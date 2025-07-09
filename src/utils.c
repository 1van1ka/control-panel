#include "../config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void die(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);
  } else {
    fputc('\n', stderr);
  }

  exit(1);
}

void execute_command_args(char *const argv[]) {
  pid_t pid = fork();
  if (pid == 0) {
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    execvp(argv[0], argv);
    perror("execvp failed");
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    waitpid(pid, NULL, 0);
  }
}

int read_int_from_file(const char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    return -1;
  }
  int value;
  if (fscanf(fp, "%d", &value) != 1) {
    fclose(fp);
    fprintf(stderr, "Failed to read integer from %s\n", path);
    return -1;
  }
  fclose(fp);
  return value;
}

void set_value(struct Widget *w, int (*state)()) {
  free(w->text);
  w->text = strdup((state ? state() : 1) ? w->valid_label : w->invalid_label);
}
