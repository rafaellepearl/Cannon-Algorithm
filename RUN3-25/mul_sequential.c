/**
 * Suite to perform sequential matrix multiplication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * Detects the size N of a square matrix by counting elements in the first line.
 * @filename: path to the source file
 * Returns: matrix dimension N
 */
int detect_matrix_size(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("Could not open file to detect size");
    exit(1);
  }

  int N = 0;
  double val;
  int ch;

  // Count numbers in the first line, skipping spaces and stopping at newline
  while ((ch = fgetc(f)) != EOF) {
    if (ch == '\n' || ch == '\r') {
      break;
    }
    if (ch == ' ' || ch == '\t') {
      continue;
    }
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
 * Loads a square matrix of size N x N from a text file.
 * @filename: path to the source file
 * @N: matrix size (rows/columns)
 * Returns: pointer to the dynamically allocated matrix array
 */

double *load_matrix(const char *filename, int N) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("Could not open input file");
    exit(1);
  }

  double *matrix = (double *)malloc(N * N * sizeof(double));
  for (int i = 0; i < N * N; i++) {
    if (fscanf(f, "%lf", &matrix[i]) != 1) {
      fprintf(stderr, "Read failure in file %s\n", filename);
      exit(1);
    }
  }
  fclose(f);
  return matrix;
}

/**
 * Performs standard sequential matrix multiplication (C = A x B).
 * @A: pointer to the first input matrix
 * @B: pointer to the second input matrix
 * @C: pointer to the output matrix where results are stored
 * @N: matrix dimension
 */

void matrix_sequential(double *A, double *B, double *C, int N) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      C[i * N + j] = 0.0;
      for (int k = 0; k < N; k++) {
        C[i * N + j] += A[i * N + k] * B[k * N + j];
      }
    }
  }
}

int main(int argc, char *argv[]) {

  int N = detect_matrix_size("matrix_A.txt");

  // Allocate and load datasets from txt files
  double *A = load_matrix("matrix_A.txt", N);
  double *B = load_matrix("matrix_B.txt", N);
  double *C = (double *)calloc(N * N, sizeof(double));

  // High-resolution timing of the actual computation
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  matrix_sequential(A, B, C, N);

  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_taken =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("Execution time: %f seconds\n", time_taken);

  // Export the benchmark result to a tracking file
  FILE *f_time = fopen("time_sequential.txt", "w");
  if (f_time) {
    fprintf(f_time, "%f\n", time_taken);
    fclose(f_time);
    printf("Time saved to time_sequential.txt\n");
  } else {
    perror("Error saving the execution time file");
  }

  // Export the resulting matrix C to mul_sequential.txt
  FILE *f_matrix = fopen("mul_sequential.txt", "w");
  if (f_matrix) {
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) {
        fprintf(f_matrix, "%f ", C[i * N + j]);
      }
      fprintf(f_matrix, "\n");
    }
    fclose(f_matrix);
    printf("Matrix C successfully saved to mul_sequential.txt\n");
  } else {
    perror("Error saving the matrix file");
  }

  // Resource cleanup
  free(A);
  free(B);
  free(C);

  return 0;
}