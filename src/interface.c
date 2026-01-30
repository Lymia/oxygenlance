#include <string.h>

#include "parser.h"
#include "gearlance_core.h"
#include "interface.h"

struct gearlance_compiled_program {
    union opcode *codeA; // owned
    union opcode *codeB; // owned
    union opcode *codeB2; // owned
    union opcode *codeACrank; // owned
    union opcode *codeBCrank; // owned
    union opcode *codeB2Crank; // owned
};

struct gearlance_compile_result gearlance_compile(struct gearlance_compile_input input) {
    struct input_data parse_input_data;
    parse_input_data.data = input.data;
    parse_input_data.length = input.length;

    struct oplist *opl = opl_parse(&parse_input_data);

    struct gearlance_compile_result result;
    if (parse_input_data.error_encountered) {
        result.error_encountered = true;
        result.err_msg = parse_input_data.err_msg;
        result.program = NULL;
    } else {
        result.error_encountered = false;
        result.err_msg = NULL;

        struct gearlance_compiled_program *program = smalloc(sizeof(struct gearlance_compiled_program));
        program->codeA = gearlance_core(core_compile_a, NULL, opl, NULL, NULL, NULL);
        program->codeB = gearlance_core(core_compile_b, NULL, opl, NULL, NULL, NULL);
        program->codeB2 = gearlance_core(core_compile_b2, NULL, opl, NULL, NULL, NULL);
        program->codeACrank = cranklance_core(core_compile_a, NULL, opl, NULL, NULL, NULL);
        program->codeBCrank = cranklance_core(core_compile_b, NULL, opl, NULL, NULL, NULL);
        program->codeB2Crank = cranklance_core(core_compile_b2, NULL, opl, NULL, NULL, NULL);
        result.program = program;
    }

    opl_free(opl);

    return result;
}

void gearlance_execute(struct gearlance_execute_input input, struct gearlance_execute_result* output) {
    struct gearlance_result results;
    if (!input.track_stats) {
        gearlance_core(
            core_run, &results, NULL,
            input.program_left->codeA,
            input.program_right->codeB,
            input.program_right->codeB2
        );
    } else {
        cranklance_core(
            core_run, &results, NULL,
            input.program_left->codeACrank,
            input.program_right->codeBCrank,
            input.program_right->codeB2Crank
        );
    }

    memcpy(output->scores, results.scores, sizeof results.scores);
    output->cycles = results.cycles;
    if (!input.track_stats) {
        memset(output->tape_max, 0, sizeof output->tape_max);
        memset(output->heat_position, 0, sizeof output->tape_max);
    } else {
        memcpy(output->tape_max, results.xstats.tape_max, sizeof results.xstats.tape_max);
        memcpy(output->heat_position, results.xstats.heat_position, sizeof results.xstats.heat_position);
    }

    for (int pol = 0; pol < 2; pol++) {
        for (int tapesize = 0; tapesize < MAXTAPE+1; tapesize++) {
            if (results.win_by_tape[pol][tapesize]) output->end_type[pol][tapesize] = gearlance_end_tape;
            else if (results.win_by_flag[pol][tapesize]) output->end_type[pol][tapesize] = gearlance_end_flag;
            else if (results.win_by_time[pol][tapesize]) output->end_type[pol][tapesize] = gearlance_end_time;
            else output->end_type[pol][tapesize] = gearlance_end_unknown;
        }
    }
}

void gearlance_free_program(struct gearlance_compiled_program *program) {
    sfree(program->codeA);
    sfree(program->codeB);
    sfree(program->codeB2);
    sfree(program->codeACrank);
    sfree(program->codeBCrank);
    sfree(program->codeB2Crank);
    sfree(program);
}
