/*
 * cranklance/gearlance bfjoust interpreter; shared helpers.
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

#ifndef CRANKLANCE_COMMON_H
#define CRANKLANCE_COMMON_H 1

#include <stddef.h>

/* generic helpers */

void oxygenlance_die(const char *str);
void *oxygenlance_smalloc(size_t size);
void *oxygenlance_srealloc(void *ptr, size_t size);
void oxygenlance_sfree(void* ptr);

#define die(...) oxygenlance_die(__VA_ARGS__)
#define smalloc(...) oxygenlance_smalloc(__VA_ARGS__)
#define srealloc(...) oxygenlance_srealloc(__VA_ARGS__)
#define sfree(...) oxygenlance_sfree(__VA_ARGS__)

#endif /* !CRANKLANCE_COMMON_H */
