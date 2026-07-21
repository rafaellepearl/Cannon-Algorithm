/**
 * Suite to generate and save matrices for further tests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * generate_and_save
 * ----------------------------
 * Generate a matrix of size N x N and save it to a file.
 * @filename: name of the file to save the matrix to
 * @N: size of the matrix
 */

void generate_and_save(const char *filename, int N) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    perror("Error in file opening");
    exit(1);
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      double val = ((double)rand() / RAND_MAX) * 10.0;
      fprintf(f, "%f ", val);
    }
    fprintf(f, "\n");
  }

  fclose(f);
  printf("Created file %s with size %dx%d\n", filename, N, N);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <size>\n", argv[0]);
    return 1;
  }
  int N = atoi(argv[1]);
  srand(time(NULL));

  generate_and_save("matrix_A.txt", N);
  generate_and_save("matrix_B.txt", N);

  return 0;
}