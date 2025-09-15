#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define N 10
#define WORKERS 5

static void fill_input(int A[N][N], int B[N][N]) {
  int i, j;
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
      A[i][j] = (i + 1) + (j % 3);
      B[i][j] = (i == j) ? 1 : ((i + j) % 5);
    }
  }
}

static void multiply_rows(int A[N][N], int B[N][N], int r0, int r1, int out_rows, int wfd) {
  int i, j, k;
  int Crows[2][N];
  for (i = r0; i <= r1; i++) {
    for (j = 0; j < N; j++) {
      int sum = 0;
      for (k = 0; k < N; k++) sum += A[i][k] * B[k][j];
      Crows[i - r0][j] = sum;
    }
  }
  write(wfd, &r0, sizeof(r0));
  write(wfd, &out_rows, sizeof(out_rows));
  write(wfd, Crows, sizeof(int) * out_rows * N);
}

int
main(void) {
  int A[N][N], B[N][N], C[N][N];
  int i;

  fill_input(A, B);

  int pipes[WORKERS][2];
  for (i = 0; i < WORKERS; i++) {
    if (pipe(pipes[i]) < 0) {
      printf(2, "mmul: pipe failed\n");
      exit();
    }
  }

  for (i = 0; i < WORKERS; i++) {
    int r0 = i * (N/WORKERS);
    int r1 = r0 + (N/WORKERS) - 1;
    int pid = fork();
    if (pid < 0) {
      printf(2, "mmul: fork failed\n");
      exit();
    }
    if (pid == 0) {
      close(pipes[i][0]);
      multiply_rows(A, B, r0, r1, r1 - r0 + 1, pipes[i][1]);
      close(pipes[i][1]);
      exit();
    } else {
      close(pipes[i][1]);
    }
  }

  for (i = 0; i < WORKERS; i++) {
    int rstart=0, rows=0;
    if (read(pipes[i][0], &rstart, sizeof(rstart)) != sizeof(rstart)) {
      printf(2, "mmul: read rstart failed\n");
      exit();
    }
    if (read(pipes[i][0], &rows, sizeof(rows)) != sizeof(rows)) {
      printf(2, "mmul: read rows failed\n");
      exit();
    }
    int buf[2][N];
    int need = sizeof(int) * rows * N;
    if (read(pipes[i][0], buf, need) != need) {
      printf(2, "mmul: read data failed\n");
      exit();
    }
    close(pipes[i][0]);
    int r,c;
    for (r = 0; r < rows; r++) {
      for (c = 0; c < N; c++) {
        C[rstart + r][c] = buf[r][c];
      }
    }
  }

  for (i = 0; i < WORKERS; i++) wait();

  printf(1, "Result C = A x B (10x10):\n");
  for (i = 0; i < N; i++) {
    int j;
    for (j = 0; j < N; j++) {
      printf(1, "%d ", C[i][j]);
    }
    printf(1, "\n");
  }
  exit();
}
