#ifndef INTERFACE_H
#define INTERFACE_H 1

#include <stddef.h>

#include "consts.h"

struct gearlance_compile_input {
    const char* data; // borrowed
    size_t length;
};

struct gearlance_compiled_program;

struct gearlance_compile_result {
    bool error_encountered;
    const char* err_msg; // static
    struct gearlance_compiled_program *program; // owned
};

struct gearlance_execute_input {
    struct gearlance_compiled_program *program_left; // borrowed
    struct gearlance_compiled_program *program_right; // borrowed
    bool track_stats;
};

struct gearlance_execute_result {
    int scores[2][MAXTAPE+1];
    unsigned long long cycles;
    unsigned char tape_max[2][MAXTAPE];
    unsigned heat_position[2][MAXTAPE];
};

extern struct gearlance_compile_result gearlance_compile(struct gearlance_compile_input input);
extern void gearlance_execute(struct gearlance_execute_input input, struct gearlance_execute_result* output);
extern void gearlance_free_program(struct gearlance_compiled_program *program);

#endif