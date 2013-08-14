/*
 * Copyright (c) 2013 Ahmed Samy  <f.fallen45@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

typedef struct buffer buffer_t;

extern buffer_t *balloc(const char *f);
extern void      bfree(buffer_t *b);

extern int      btell(const buffer_t *bp);
extern uint8_t  bgetc(buffer_t *bp);
extern uint16_t bget16(buffer_t *bp);
extern uint32_t bget32(buffer_t *bp);

extern void     bskip(buffer_t *bp, int size);
extern void	bseek(buffer_t *bp, int pos);

#endif

