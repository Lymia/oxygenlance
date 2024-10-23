/*
 * cranklance/gearlance bfjoust interpreter; shared parser parts.
 *
 * Copyright (c) 2011-2013 Heikki Kallasjoki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rust_callbacks.h"
#include "parser.h"

static thread_local const char* err_msg;
static thread_local jmp_buf fail_buf;

static void fail(const char *err)
{
    err_msg = err;
	longjmp(fail_buf, 1);
}

/* forced repetition count for empty loops.
   (0 and 1 are sensible values.) */
#define EMPTY_LOOP_COUNT 0

/* helper functions for reading from the input */

static int nextc(struct input_data* input)
{
    if (input->ptr == input->length)
        return -1;
    return input->data[input->ptr++];
}

static void unc(struct input_data* input)
{
    if (input->ptr == 0)
        die("attempt to reverse character at idx 0");
    input->ptr -= 1;
}

/* parsing and preprocessing, impl */

static int nextcmd(struct input_data* input)
{
	while (1)
	{
		int c = nextc(input);
		switch (c)
		{
		case -1:
		case '+': case '-': case '<': case '>': case '.': case ',': case '[': case ']':
		case '(': case ')': case '{': case '}': case '*': case '%':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return c;

		default:
			/* ignore this character */
			break;
		}
	}
}

static int readrepc(struct input_data* input)
{
	int c = 0, neg = 0, ch;

	ch = nextcmd(input);
	if (ch != '*' && ch != '%')
	{
		/* treat garbage as ()*0 in case it's inside a comment */
		unc(input);
		return 0;
	}

	ch = nextcmd(input);
	if (ch == '-')
	{
		neg = 1;
		ch = nextc(input);
	}

	while (1)
	{
		if (ch < '0' || ch > '9')
			break;

		c = c*10 + (ch - '0');
		if (c > MAXCYCLES)
		{
			c = MAXCYCLES;
			ch = 0;
			break;
		}

		ch = nextc(input);
	}

	unc(input);

	return neg ? -c : c;
}

static struct oplist *readops(struct input_data* input)
{
	/* main code to read the list of ops */

	struct oplist *ops = opl_new();
	int ch;

	while ((ch = nextcmd(input)) >= 0)
	{
		int c;

		switch (ch)
		{
		case '+': ops = opl_append(ops, OP_INC);    break;
		case '-': ops = opl_append(ops, OP_DEC);    break;
		case '<': ops = opl_append(ops, OP_LEFT);   break;
		case '>': ops = opl_append(ops, OP_RIGHT);  break;
		case '.': ops = opl_append(ops, OP_WAIT);   break;
		case '[': ops = opl_append(ops, OP_LOOP1);  break;
		case ']': ops = opl_append(ops, OP_LOOP2);  break;
		case '(': ops = opl_append(ops, OP_REP1);   break;
		case ')':
			/* need to extract the count */
			c = readrepc(input);
			if (c < 0) c = MAXCYCLES;
			ops = opl_append(ops, OP_REP2);
			ops->ops[ops->len-1].count = c;
			break;
		case '{': ops = opl_append(ops, OP_INNER1); break;
		case '}': ops = opl_append(ops, OP_INNER2); break;
		default:
			/* ignore unexpected commands */
			break;
		}
	}

	ops = opl_append(ops, OP_DONE);

	return ops;
}

static void matchrep(struct oplist *ops)
{
	/* match (..) pairs and inner {..} blocks */

	unsigned stack[MAXNEST], istack[MAXNEST], idstack[MAXNEST];
	unsigned depth = 0, idepth = 0, isdepth = 0;

	for (unsigned at = 0; at < ops->len; at++)
	{
		struct op *op = &ops->ops[at];

		switch (op->type) /* in order of occurrence */
		{
		case OP_REP1:
			if (depth == MAXNEST) fail("maximum () nesting depth exceeded");
			stack[depth] = at;
			idstack[depth] = idepth;
			op->match = -1;
			op->inner = -1;
			depth++;
			idepth = 0;
			break;

		case OP_INNER1:
			istack[isdepth++] = at;
			idepth++;
			if (idepth > depth) fail("encountered { without suitable enclosing (");
			op->match = stack[depth-idepth];
			op->inner = -1;
			if (ops->ops[op->match].match != -1) fail("encountered second { on a same level");
			ops->ops[op->match].type = OP_IREP1;
			ops->ops[op->match].match = at;
			break;

		case OP_INNER2:
			if (!idepth) fail("terminating } without a matching {");
			idepth--;
			isdepth--;
			op->match = -1;
			op->inner = istack[isdepth];
			ops->ops[op->inner].inner = at;
			break;

		case OP_REP2:
			if (!depth) fail("terminating ) without a matching (");
			if (idepth) fail("starting { without a matching }");
			depth--;
			if (ops->ops[stack[depth]].type == OP_IREP1)
			{
				op->type = OP_IREP2;
				op->inner = stack[depth];
				op->match = ops->ops[ops->ops[op->inner].match].inner;
				ops->ops[op->inner].inner = at;
				ops->ops[op->inner].count = op->count;
				ops->ops[ops->ops[op->inner].match].count = op->count;
			}
			else
			{
				op->inner = -1;
				op->match = stack[depth];
			}
			ops->ops[op->match].match = at;
			ops->ops[op->match].count = op->count;
			idepth = idstack[depth];
			break;

		default:
			/* do nothing */
			break;
		}
	}

	if (depth != 0)
		fail("starting ( without a matching )");
}

static void cleanrep(struct oplist *ops)
{
	/* turn contentless loops into *0's.
	   transform ({a}b)%N to ()*0a(b)*N.
	   transform (a{b})%N to (a)*Nb()*0. */

	int last_real = -1;

	for (unsigned at = 0; at < ops->len; at++)
	{
		struct op *op = &ops->ops[at];
		switch (op->type)
		{
		case OP_INC: case OP_DEC: case OP_LEFT: case OP_RIGHT:
		case OP_LOOP1: case OP_LOOP2: case OP_WAIT: case OP_DONE:
			last_real = at;
			break;
		case OP_REP1: case OP_IREP1: case OP_INNER2:
			/* no action */
			break;
		case OP_REP2:
			if (last_real < op->match)
			{
				/* empty () loop */
				op->count = 0;
				ops->ops[op->match].count = 0;
			}
			break;
		case OP_INNER1:
			if (last_real < op->match)
			{
				/* empty ({ part */
				int rep1 = op->match, inner1 = at, inner2 = op->inner, rep2 = ops->ops[op->match].inner;
				ops->ops[rep1].type = OP_REP1;
				ops->ops[rep1].count = EMPTY_LOOP_COUNT;
				ops->ops[rep1].inner = -1;
				ops->ops[inner1].type = OP_REP2;
				ops->ops[inner1].count = EMPTY_LOOP_COUNT;
				ops->ops[inner1].inner = -1;
				ops->ops[inner2].type = OP_REP1;
				ops->ops[inner2].inner = -1;
				ops->ops[rep2].type = OP_REP2;
				ops->ops[rep2].inner = -1;
			}
			break;
		case OP_IREP2:
			if (last_real < op->match)
			{
				/* empty }) part */
				int rep1 = op->inner, inner1 = ops->ops[op->match].inner, inner2 = op->match, rep2 = at;
				ops->ops[rep1].type = OP_REP1;
				ops->ops[rep1].inner = -1;
				ops->ops[inner1].type = OP_REP2;
				ops->ops[inner1].inner = -1;
				ops->ops[inner2].type = OP_REP1;
				ops->ops[inner2].count = EMPTY_LOOP_COUNT;
				ops->ops[inner2].inner = -1;
				ops->ops[rep2].type = OP_REP2;
				ops->ops[rep2].count = EMPTY_LOOP_COUNT;
				ops->ops[rep2].inner = -1;
			}
			break;
		default:
			fail("impossible: unknown op->type");
		}
	}

	/* clean out (...)*0 loops */

	unsigned orig_len = ops->len;
	for (unsigned at = 0, to = 0; at < orig_len; at++, to++)
	{
		struct op *op = &ops->ops[at];

		if ((op->type == OP_REP1 || op->type == OP_IREP1 || op->type == OP_INNER2) && op->count == 0)
		{
			unsigned del_to = op->match; /* delete this far */
			ops->len -= del_to - at + 1; /* fixup length */
			at = del_to;                 /* skip the loop */
			to--;                        /* don't copy anything */
		}
		else if (to < at)
		{
			if (op->match != -1)
				ops->ops[op->match].match = to;
			if (op->inner != -1)
				ops->ops[op->inner].inner = to;
			ops->ops[to] = *op;
		}
	}
}

static void matchloop(struct oplist *ops)
{
	/* match [..] pairs */

	unsigned stack[MAXNEST], idstack[MAXNEST];
	unsigned depth = 0, idepth = 0;

	for (unsigned at = 0; at < ops->len; at++)
	{
		struct op *op = &ops->ops[at];

		switch (op->type)
		{
		case OP_LOOP1:
			if (depth == MAXNEST) fail("maximum [] nesting depth exceeded");
			stack[depth] = at;
			idstack[depth] = idepth;
			op->match = -1;
			depth++;
			idepth = 0;
			break;

		case OP_REP1:
		case OP_IREP1:
		case OP_INNER1:
			idepth++;
			break;

		case OP_INNER2:
		case OP_IREP2:
		case OP_REP2:
			if (!idepth) fail("[..] crossing out of a ({..}) level");
			idepth--;
			break;

		case OP_LOOP2:
			if (!depth) fail("terminating ] without a matching [");
			if (idepth) fail("[..] crossing into a ({..}) level");
			depth--;
			op->match = stack[depth];
			ops->ops[op->match].match = at;
			idepth = idstack[depth];
			break;

		default:
			/* do nothing */
			break;
		}
	}

	if (depth != 0)
		fail("starting [ without a matching ]");
}

struct oplist *opl_parse(struct input_data* input)
{
    input->ptr = 0;
    input->error_encountered = false;
    input->err_msg = NULL;

    if (setjmp(fail_buf))
    {
        input->error_encountered = true;
        input->err_msg = err_msg;
        return 0;
    }


	struct oplist *ops = readops(input);

	/* handle (...) constructions first */

	matchrep(ops);
	cleanrep(ops);

	/* handle [...] constructions now that rep/inner levels are known */

	matchloop(ops);

	return ops;
}

/* oplist handling, impl */

struct oplist *opl_new(void)
{
	const unsigned size = 32;

	struct oplist *o = smalloc(sizeof *o + size * sizeof *o->ops);

	o->len = 0;
	o->size = size;

	return o;
}

void opl_free(struct oplist *o)
{
	sfree(o);
}

struct oplist *opl_append(struct oplist *o, enum optype type)
{
	if (o->len == o->size)
	{
		o->size += o->size >> 1;
		o = srealloc(o, sizeof *o + o->size * sizeof *o->ops);
	}

	o->ops[o->len].type = type;
	o->ops[o->len].match = -1;
	o->ops[o->len].inner = -1;
	o->ops[o->len].count = -1;
	o->len++;

	return o;
}
