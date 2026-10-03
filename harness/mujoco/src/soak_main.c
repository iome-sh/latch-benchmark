/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 iome-sh contributors
 *
 * BM-10 soak. Two sequential child execs of this binary replay the same
 * fixture sense stream. Exit 0 only when both emit the same mode name,
 * plane, and evidence for each row. Public results are mock-oracle.
 */
#define _POSIX_C_SOURCE 200809L

#include "oracle_runner.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef LBS_FIXTURE_DIR
#define LBS_FIXTURE_DIR "fixtures"
#endif

enum { kCaptureCap = 65536 };

static void emit_row(const char *id, const char *name, const char *plane, const char *evidence, void *user) {
  (void)user;
  printf("ROW %s %s %s %s\n", id, name, plane, evidence);
}

static int emit_main(const char *path) {
  OracleRun run;
  int match = 0;
#if LBS_MOCK_ORACLE
  match = 1;
#endif
  if (lbs_replay_oracle_jsonl(path, "lbs-mock-oracle", match, match, emit_row, NULL, &run) != 0 || run.fails != 0 ||
      run.rows < 1) {
    fprintf(stderr, "FAIL BM-10 emit rows=%d fails=%d\n", run.rows, run.fails);
    return 1;
  }
  if (fflush(stdout) != 0) return 1;
  return 0;
}

static int capture_child(char *const argv[], char *buf, int cap, int *status) {
  int fds[2];
  pid_t pid = 0;
  int n = 0;
  if (pipe(fds) != 0) return -1;
  pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    return -1;
  }
  if (pid == 0) {
    close(fds[0]);
    if (dup2(fds[1], STDOUT_FILENO) < 0) _exit(127);
    close(fds[1]);
    execv(argv[0], argv);
    _exit(127);
  }
  close(fds[1]);
  while (n < cap) {
    ssize_t got = read(fds[0], buf + n, (size_t)(cap - n));
    if (got == 0) break;
    if (got < 0) {
      if (errno == EINTR) continue;
      close(fds[0]);
      waitpid(pid, status, 0);
      return -1;
    }
    n += (int)got;
  }
  if (n == cap) {
    char extra = 0;
    ssize_t got = 0;
    do {
      got = read(fds[0], &extra, 1);
    } while (got < 0 && errno == EINTR);
    if (got != 0) {
      close(fds[0]);
      waitpid(pid, status, 0);
      return -1;
    }
  }
  close(fds[0]);
  if (waitpid(pid, status, 0) < 0) return -1;
  buf[n] = '\0';
  return n;
}

static int child_ok(int status) { return WIFEXITED(status) && WEXITSTATUS(status) == 0; }

static int count_rows(const char *buf, int n) {
  int i = 0;
  int rows = 0;
  for (i = 0; i < n; ++i) {
    if (buf[i] == '\n') rows += 1;
  }
  return rows;
}

static int fixture_path(char *path, int cap) {
  int n = snprintf(path, (size_t)cap, "%s/mock-oracle.jsonl.example", LBS_FIXTURE_DIR);
  return n > 0 && n < cap;
}

int main(int argc, char **argv) {
  char path[512];
  char self[4096];
  char flag[] = "--emit";
  char *child_argv[4];
  char *left = NULL;
  char *right = NULL;
  int left_n = 0;
  int right_n = 0;
  int left_status = 0;
  int right_status = 0;
  int rows = 0;
  int self_n = 0;

  if (argc >= 2 && strcmp(argv[1], "--emit") == 0) {
    if (argc != 3) return 2;
    return emit_main(argv[2]);
  }
  if (!argv[0] || !fixture_path(path, (int)sizeof path)) {
    fprintf(stderr, "FAIL BM-10 fixture path\n");
    return 1;
  }
  self_n = snprintf(self, sizeof self, "%s", argv[0]);
  if (self_n <= 0 || (size_t)self_n >= sizeof self) {
    fprintf(stderr, "FAIL BM-10 exec path\n");
    return 1;
  }
  child_argv[0] = self;
  child_argv[1] = flag;
  child_argv[2] = path;
  child_argv[3] = NULL;

  left = (char *)malloc((size_t)kCaptureCap + 1u);
  right = (char *)malloc((size_t)kCaptureCap + 1u);
  if (!left || !right) {
    fprintf(stderr, "FAIL BM-10 capture buffer\n");
    free(left);
    free(right);
    return 1;
  }
  left_n = capture_child(child_argv, left, kCaptureCap, &left_status);
  right_n = capture_child(child_argv, right, kCaptureCap, &right_status);
  if (left_n < 1 || right_n < 1 || !child_ok(left_status) || !child_ok(right_status)) {
    fprintf(stderr, "FAIL BM-10 child status left=%d right=%d bytes=%d/%d\n", left_status, right_status, left_n,
            right_n);
    free(left);
    free(right);
    return 1;
  }
  rows = count_rows(left, left_n);
  if (rows < 1 || left_n != right_n || memcmp(left, right, (size_t)left_n) != 0) {
    fprintf(stderr, "FAIL BM-10 process mismatch rows=%d bytes=%d/%d\n", rows, left_n, right_n);
    free(left);
    free(right);
    return 1;
  }
  free(left);
  free(right);
#if LBS_MOCK_ORACLE
  printf("BM-10 mock-oracle %d rows two-process match (not a Latch evidence-v2 certificate)\n", rows);
#else
  printf("BM-10 two-process match %d rows; in-tree fixture is not a Latch certificate\n", rows);
#endif
  return 0;
}
