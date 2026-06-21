#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Rileva la dimensione N di una matrice quadrata contando gli elementi nella
 * prima riga.
 */
int detect_matrix_size(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("Impossibile aprire il file per calcolare N");
    exit(1);
  }

  int N = 0;
  double val;
  int ch;

  while ((ch = fgetc(f)) != EOF) {
    if (ch == '\n' || ch == '\r')
      break;
    if (ch == ' ' || ch == '\t')
      continue;
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

/**
 * Carica una matrice quadrata NxN da un file di testo.
 */
double *load_matrix(const char *filename, int N) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("Impossibile aprire il file di input");
    exit(1);
  }

  double *matrix = (double *)malloc(N * N * sizeof(double));
  for (int i = 0; i < N * N; i++) {
    if (fscanf(f, "%lf", &matrix[i]) != 1) {
      fprintf(stderr, "Errore di lettura nel file %s\n", filename);
      exit(1);
    }
  }
  fclose(f);
  return matrix;
}

// Moltiplicazione locale dei sottomatrici
void local_matrix_multiply(double *A, double *B, double *C, int size) {
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      for (int k = 0; k < size; k++) {
        C[i * size + j] += A[i * size + k] * B[k * size + j];
      }
    }
  }
}

int main(int argc, char **argv) {
  int rank, num_procs;
  int global_N = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int sqrt_P = (int)sqrt(num_procs);
  if (sqrt_P * sqrt_P != num_procs) {
    if (rank == 0)
      printf(
          "Errore: Il numero di processi deve essere un quadrato perfetto.\n");
    MPI_Finalize();
    return 1;
  }

  double *global_A = NULL;
  double *global_B = NULL;
  double *global_C = NULL;

  // 1. LETTURA FILE (Solo Processo 0)
  if (rank == 0) {
    global_N = detect_matrix_size("matrix_A.txt");
    if (global_N <= 0) {
      fprintf(stderr, "Errore: Dimensione non valida (%d).\n", global_N);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    global_A = load_matrix("matrix_A.txt", global_N);
    global_B = load_matrix("matrix_B.txt", global_N);
    global_C = (double *)calloc(global_N * global_N, sizeof(double));
  }

  // Broadcast dimensione
  MPI_Bcast(&global_N, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (global_N % sqrt_P != 0) {
    if (rank == 0)
      printf("Errore: N (%d) deve essere divisibile per sqrt_P (%d)\n",
             global_N, sqrt_P);
    if (rank == 0) {
      free(global_A);
      free(global_B);
      free(global_C);
    }
    MPI_Finalize();
    return 1;
  }

  int block_N = global_N / sqrt_P;
  int block_size = block_N * block_N;

  double *local_A = (double *)malloc(block_size * sizeof(double));
  double *local_B = (double *)malloc(block_size * sizeof(double));
  double *local_C = (double *)calloc(block_size, sizeof(double));
  double *buf_A = (double *)malloc(block_size * sizeof(double));
  double *buf_B = (double *)malloc(block_size * sizeof(double));

  // 2. DISTRIBUZIONE DEI DATI
  if (rank == 0) {
    for (int p = 0; p < num_procs; p++) {
      int p_row = p / sqrt_P;
      int p_col = p % sqrt_P;

      double *send_ptr_A = (double *)malloc(block_size * sizeof(double));
      double *send_ptr_B = (double *)malloc(block_size * sizeof(double));

      for (int i = 0; i < block_N; i++) {
        int global_row = p_row * block_N + i;
        int global_col_start = p_col * block_N;
        memcpy(&send_ptr_A[i * block_N],
               &global_A[global_row * global_N + global_col_start],
               block_N * sizeof(double));
        memcpy(&send_ptr_B[i * block_N],
               &global_B[global_row * global_N + global_col_start],
               block_N * sizeof(double));
      }

      if (p == 0) {
        memcpy(local_A, send_ptr_A, block_size * sizeof(double));
        memcpy(local_B, send_ptr_B, block_size * sizeof(double));
      } else {
        MPI_Send(send_ptr_A, block_size, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
        MPI_Send(send_ptr_B, block_size, MPI_DOUBLE, p, 1, MPI_COMM_WORLD);
      }
      free(send_ptr_A);
      free(send_ptr_B);
    }
  } else {
    MPI_Recv(local_A, block_size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    MPI_Recv(local_B, block_size, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double start_time = MPI_Wtime();

  // 3. TOPOLOGIA CARTESIANA
  int dims[2] = {sqrt_P, sqrt_P};
  int periods[2] = {1, 1};
  MPI_Comm cart_comm;
  // REORDER = 0 è fondamentale per non rompere la mappatura del nostro Scatter
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

  int cart_rank;
  MPI_Comm_rank(cart_comm, &cart_rank);

  int coords[2];
  MPI_Cart_coords(cart_comm, cart_rank, 2, coords);
  int my_row = coords[0];
  int my_col = coords[1];

  int left, right, up, down;
  MPI_Cart_shift(cart_comm, 1, 1, &left, &right);
  MPI_Cart_shift(cart_comm, 0, 1, &up, &down);

  // 4. SKEWING INIZIALE (con controlli per evitare self-sending letali)
  if (my_row > 0) {
    int init_left, init_right;
    MPI_Cart_shift(cart_comm, 1, my_row, &init_left, &init_right);
    MPI_Sendrecv(local_A, block_size, MPI_DOUBLE, init_left, 0, buf_A,
                 block_size, MPI_DOUBLE, init_right, 0, cart_comm,
                 MPI_STATUS_IGNORE);
    memcpy(local_A, buf_A, block_size * sizeof(double));
  }

  if (my_col > 0) {
    int init_up, init_down;
    MPI_Cart_shift(cart_comm, 0, my_col, &init_up, &init_down);
    MPI_Sendrecv(local_B, block_size, MPI_DOUBLE, init_up, 1, buf_B, block_size,
                 MPI_DOUBLE, init_down, 1, cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_B, buf_B, block_size * sizeof(double));
  }

  // 5. LOOP DI CANNON
  for (int step = 0; step < sqrt_P; step++) {
    local_matrix_multiply(local_A, local_B, local_C, block_N);

    // Shift circolare (A sinistra, B in alto)
    MPI_Sendrecv(local_A, block_size, MPI_DOUBLE, left, 0, buf_A, block_size,
                 MPI_DOUBLE, right, 0, cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_A, buf_A, block_size * sizeof(double));

    MPI_Sendrecv(local_B, block_size, MPI_DOUBLE, up, 1, buf_B, block_size,
                 MPI_DOUBLE, down, 1, cart_comm, MPI_STATUS_IGNORE);
    memcpy(local_B, buf_B, block_size * sizeof(double));
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double end_time = MPI_Wtime();
  double total_time = end_time - start_time;

  // 6. RACCOLTA RISULTATI
  if (rank == 0) {
    for (int p = 0; p < num_procs; p++) {
      int p_row = p / sqrt_P;
      int p_col = p % sqrt_P;
      double *recv_ptr = (double *)malloc(block_size * sizeof(double));

      if (p == 0) {
        memcpy(recv_ptr, local_C, block_size * sizeof(double));
      } else {
        MPI_Recv(recv_ptr, block_size, MPI_DOUBLE, p, 2, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
      }

      for (int i = 0; i < block_N; i++) {
        int global_row = p_row * block_N + i;
        int global_col_start = p_col * block_N;
        memcpy(&global_C[global_row * global_N + global_col_start],
               &recv_ptr[i * block_N], block_N * sizeof(double));
      }
      free(recv_ptr);
    }

    FILE *file_time = fopen("time_parallel.txt", "w");
    fprintf(file_time, "%f\n", total_time);
    fclose(file_time);

    printf("Execution time: %f sec.\n", total_time);
    printf("Time saved to time_parallel.txt\n");

    FILE *f_matrix = fopen("mul_parallel.txt", "w");
    if (f_matrix) {
      for (int i = 0; i < global_N; i++) {
        for (int j = 0; j < global_N; j++) {
          fprintf(f_matrix, "%f ",
                  global_C[i * global_N +
                           j]); // Arrotondato a 2 decimali per leggibilità
        }
        fprintf(f_matrix, "\n");
      }
      fclose(f_matrix);
      printf("Matrix C successfully saved to mul_parallel.txt\n");
    } else {
      perror("Error saving the matrix file");
    }
  } else {
    MPI_Send(local_C, block_size, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
  }

  // 7. CLEANUP
  free(local_A);
  free(local_B);
  free(local_C);
  free(buf_A);
  free(buf_B);
  if (rank == 0) {
    free(global_A);
    free(global_B);
    free(global_C);
  }
  MPI_Comm_free(&cart_comm);
  MPI_Finalize();
  return 0;
}