#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Scans first line of input to determine the size of the matrix (N x N)
int detect_matrix_size(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("Unable to open file to calculate N");
    exit(1);
  }

  int N = 0;
  double val;
  int ch;

  // Read characters until the end of the first line, counting numbers
  while ((ch = fgetc(f)) != EOF) {
    if (ch == '\n' || ch == '\r')
      break;
    if (ch == ' ' || ch == '\t')
      continue;
    // Put the non-whitespace character back so fscanf can read the full number
    ungetc(ch, f);
    if (fscanf(f, "%lf", &val) == 1) {
      N++;
    } else {
      break;
    }
  }
  fclose(f);
  return N;
}

// Function to collect the NxN matrix and returns an array of NxN doubles.
double *load_matrix(const char *filename, int N) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("Unable to open input file");
    exit(1);
  }

  // Allocating the block
  double *matrix = (double *)malloc(N * N * sizeof(double));
  for (int i = 0; i < N * N; i++) {
    if (fscanf(f, "%lf", &matrix[i]) != 1) {
      fprintf(stderr, "Read error in file %s\n", filename);
      exit(1);
    }
  }
  fclose(f);
  return matrix;
}

// C += A * B for three square submatrices of side length `block_dim`
void local_matrix_multiply(double *A, double *B, double *C, int block_dim) {
  for (int i = 0; i < block_dim; i++) {
    for (int j = 0; j < block_dim; j++) {
      for (int k = 0; k < block_dim; k++) {
        C[i * block_dim + j] += A[i * block_dim + k] * B[k * block_dim + j];
      }
    }
  }
}

int main(int argc, char **argv) {
  int rank, num_procs;
  int global_N = 0; // The dimension of the matrix

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs); // How many processes are running in total
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // The rank of this process (0 to num_procs-1)

  // Making sure the processes are arranged in a square grid for Cannon's Algorithm
  int grid_size = (int)sqrt(num_procs); // Measures side length of the grid

  // Shuts down the process if the number of processes is not a perfect square
  if (grid_size * grid_size != num_procs) {
    if (rank == 0)
      printf("Error: The number of processes must be a perfect square.\n");
    MPI_Finalize();
    return 1;
  }

  // These pointers are only used by rank 0 to read the full matrices from disk and scatter them to other processes
  double *global_A = NULL;
  double *global_B = NULL;
  double *global_C = NULL; // Will hold the final result matrix after gathering from all processes

  // Rank 0 will read the file
  if (rank == 0) {
    global_N = detect_matrix_size("matrix_A.txt");
    if (global_N <= 0) {
      fprintf(stderr, "Error: Invalid dimension (%d).\n", global_N);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    global_A = load_matrix("matrix_A.txt", global_N);
    global_B = load_matrix("matrix_B.txt", global_N);
    // Use calloc to zero init the matrix
    global_C = (double *)calloc(global_N * global_N, sizeof(double));
  }

  // Broadcast matrix size to all processes
  MPI_Bcast(&global_N, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // N must divide evenly into the grid size for Cannon's algorithm to work correctly
  if (global_N % grid_size != 0) {
    if (rank == 0)
      printf("Error: N (%d) must be divisible by grid_size (%d)\n",
             global_N, grid_size);
    if (rank == 0) {
      free(global_A);
      free(global_B);
      free(global_C);
    }
    MPI_Finalize();
    return 1;
  }

  int block_dim   = global_N / grid_size;
  int block_elems = block_dim * block_dim;

  // Local submatrices for each process, plus temporary buffers for communication
  double *local_A    = (double *)malloc(block_elems * sizeof(double));
  double *local_B    = (double *)malloc(block_elems * sizeof(double));
  double *local_C    = (double *)calloc(block_elems, sizeof(double)); // calloc because we will accumulate into it
  double *recv_buf_A = (double *)malloc(block_elems * sizeof(double));
  double *recv_buf_B = (double *)malloc(block_elems * sizeof(double)); //Temp for MPI_Sendrecv

  // Rank 0 sends each process its A and B sub-blocks, while all other processes receive their blocks from rank 0
  if (rank == 0) {
    for (int proc_id = 0; proc_id < num_procs; proc_id++) {
      int proc_row = proc_id / grid_size;
      int proc_col = proc_id % grid_size;

      double *block_send_A = (double *)malloc(block_elems * sizeof(double));
      double *block_send_B = (double *)malloc(block_elems * sizeof(double));

      // Copy row by row from the global matrix into the buffer
      for (int i = 0; i < block_dim; i++) {
        int global_row       = proc_row * block_dim + i; // Which row of the full matrix this block row maps to
        int global_col_start = proc_col * block_dim; // First column index belonging to this block
        memcpy(&block_send_A[i * block_dim],
               &global_A[global_row * global_N + global_col_start],
               block_dim * sizeof(double));
        memcpy(&block_send_B[i * block_dim],
               &global_B[global_row * global_N + global_col_start],
               block_dim * sizeof(double));
      }

      if (proc_id == 0) {
        // Rank 0 keeps its own block; no MPI call needed
        memcpy(local_A, block_send_A, block_elems * sizeof(double));
        memcpy(local_B, block_send_B, block_elems * sizeof(double));
      } else {
        // Tags 0 and 1 distinguish A blocks from B blocks in the inbox
        MPI_Send(block_send_A, block_elems, MPI_DOUBLE, proc_id, 0, MPI_COMM_WORLD);
        MPI_Send(block_send_B, block_elems, MPI_DOUBLE, proc_id, 1, MPI_COMM_WORLD);
      }
      free(block_send_A);
      free(block_send_B);
    }
  } else {
    // Every non-root process receives its A and B sub-blocks from rank 0
    MPI_Recv(local_A, block_elems, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    MPI_Recv(local_B, block_elems, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }

  // Barrier ensures all processes have their data before timing begins
  MPI_Barrier(MPI_COMM_WORLD);
  double start_time = MPI_Wtime();

  // Wrap all processes into a 2-D torus so that MPI_Cart_shift can compute left/right/up/down neighbours with automatic wrap-around.
  int dims[2]    = {grid_size, grid_size};
  int periods[2] = {1, 1}; // 1 = periodic (torus) in both dimensions
  MPI_Comm cart_comm;
  // We set the reorder property to 0 to keep the same rank numbering so it's consistent
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

  int cart_rank;
  MPI_Comm_rank(cart_comm, &cart_rank);

  // Retrieve this process's (row, col) position in the grid
  int coords[2];
  MPI_Cart_coords(cart_comm, cart_rank, 2, coords);
  int my_row = coords[0];
  int my_col = coords[1];

  // Pre-compute the four neighbours used in every Cannon shift step
  int left, right, up, down;
  MPI_Cart_shift(cart_comm, 1, 1, &left, &right); // shift axis 1 (columns) -> left/right neighbours for A
  MPI_Cart_shift(cart_comm, 0, 1, &up, &down); // shift axis 0 (rows) -> up/down neighbours for B

  // Skew A: row i shifts its block left by i steps, one step at a time
  for (int s = 0; s < my_row; s++) {
    MPI_Sendrecv(local_A, block_elems, MPI_DOUBLE, left,  0,
                 recv_buf_A, block_elems, MPI_DOUBLE, right, 0,
                 cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_A, recv_buf_A, block_elems * sizeof(double));
  }

  // Skew B: col j shifts its block up by j steps, one step at a time
  for (int s = 0; s < my_col; s++) {
    MPI_Sendrecv(local_B, block_elems, MPI_DOUBLE, up,   1,
                 recv_buf_B, block_elems, MPI_DOUBLE, down, 1,
                 cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_B, recv_buf_B, block_elems * sizeof(double));
  }

  // Initialize timing variables for computation and communication
  double calc_time = 0.0;
  double comm_time = 0.0;
  double step_start, step_end;

  // Each process performs grid_size iterations of local multiplication and shifting
  for (int step = 0; step < grid_size; step++) {
    
    // TIMING COMPUTATION
    step_start = MPI_Wtime();
    // Multiply local blocks and accumulate into local_C
    local_matrix_multiply(local_A, local_B, local_C, block_dim);
    step_end = MPI_Wtime();
    calc_time += (step_end - step_start);

    // TIMING COMMUNICATION
    step_start = MPI_Wtime();
    // Circular shift A one step to the left; receive from the right
    MPI_Sendrecv(local_A, block_elems, MPI_DOUBLE, left,  0,
                 recv_buf_A, block_elems, MPI_DOUBLE, right, 0,
                 cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_A, recv_buf_A, block_elems * sizeof(double));

    // Circular shift B one step upward; receive from below
    MPI_Sendrecv(local_B, block_elems, MPI_DOUBLE, up,   1,
                 recv_buf_B, block_elems, MPI_DOUBLE, down, 1,
                 cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_B, recv_buf_B, block_elems * sizeof(double));
    step_end = MPI_Wtime();
    comm_time += (step_end - step_start);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double end_time   = MPI_Wtime();
  double total_time = end_time - start_time;

  // Collect the results from all processes and assemble them into the global_C matrix on rank 0
  if (rank == 0) {
    for (int proc_id = 0; proc_id < num_procs; proc_id++) {
      int proc_row = proc_id / grid_size;
      int proc_col = proc_id % grid_size;
      double *block_recv_buf = (double *)malloc(block_elems * sizeof(double));

      if (proc_id == 0) {
        // Rank 0 already has its own results, so just needs to copy
        memcpy(block_recv_buf, local_C, block_elems * sizeof(double));
      } else {
        // If tag is 2, it means this is a result block being sent back to rank 0
        MPI_Recv(block_recv_buf, block_elems, MPI_DOUBLE, proc_id, 2,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }

      // Copy all received blocks into the correct position
      for (int i = 0; i < block_dim; i++) {
        int global_row       = proc_row * block_dim + i;
        int global_col_start = proc_col * block_dim;
        memcpy(&global_C[global_row * global_N + global_col_start],
               &block_recv_buf[i * block_dim], block_dim * sizeof(double));
      }
      free(block_recv_buf);
    }

    double calc_ratio = (calc_time / total_time) * 100.0;
    
    // Print to standard output (shows up in your SLURM .out file)
    printf("--- BENCHMARK RESULTS ---\n");
    printf("Matrix Size (N) : %d\n", global_N);
    printf("Processes (P)   : %d\n", num_procs);
    printf("Total Time      : %f sec\n", total_time);
    printf("Compute Time    : %f sec (%.2f%%)\n", calc_time, calc_ratio);
    printf("Comm Time       : %f sec\n", comm_time);
    printf("-------------------------\n");

    // Execution time for original benchmarking purposes
    FILE *file_time = fopen("time_parallel.txt", "w");
    fprintf(file_time, "%f\n", total_time);
    fclose(file_time);
    printf("Time saved to time_parallel.txt\n");

    // Open a CSV file in APPEND mode ("a") to aggregate multiple runs
    FILE *file_bench = fopen("benchmark_results.csv", "a");
    if (file_bench) {
      fprintf(file_bench, "%d,%d,%f,%f,%f,%.2f\n", 
              global_N, num_procs, total_time, calc_time, comm_time, calc_ratio);
      fclose(file_bench);
      printf("Metrics appended to benchmark_results.csv\n");
    } else {
      perror("Error saving the benchmark file");
    }

    // Save the result matrix row by row
    FILE *output_file = fopen("mul_parallel.txt", "w");
    if (output_file) {
      for (int i = 0; i < global_N; i++) {
        for (int j = 0; j < global_N; j++) {
          fprintf(output_file, "%f ", global_C[i * global_N + j]);
        }
        fprintf(output_file, "\n");
      }
      fclose(output_file);
      printf("Matrix C successfully saved to mul_parallel.txt\n");
    } else {
      perror("Error saving the matrix file");
    }
  } else {
    // Everyone sends their completed block to rank 0
    MPI_Send(local_C, block_elems, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
  }

  // Cleanup
  free(local_A);
  free(local_B);
  free(local_C);
  free(recv_buf_A);
  free(recv_buf_B);
  if (rank == 0) {
    free(global_A);
    free(global_B);
    free(global_C);
  }
  MPI_Comm_free(&cart_comm);
  MPI_Finalize();
  return 0;
}
