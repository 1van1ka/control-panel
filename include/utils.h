#ifndef UTILS_H
#define UTILS_H

#include "config.h"

void die(const char *fmt, ...);
void execute_command_args(char *const argv[]);
int read_int_from_file(const char *path);
void set_value(struct Widget *w, int (*state)());

#endif
