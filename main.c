#include <complex.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define COMPLEX_BUFFER_SIZE  97687
#define OUTPUT_BUFFER_SIZE   6856
#define FRAME_SLEEP_NANO     12321
#define FILL_RANGE_OFFSET    10
#define FILL_RANGE_SIZE      2003
#define ANSI_BUFFER_PROLOGUE "\x1b[2J\x1b[1;1H     "
#define LOOKUP               " '`-.|//,\\|\\_\\/#\n"

// User-definable constants.

// Gravity
#ifndef G
#define G 1
#endif

// Pressure
#ifndef P
#define P 4
#endif

// Viscosity
#ifndef V
#define V 8
#endif

typedef double complex cdouble;

cdouble arr[COMPLEX_BUFFER_SIZE];
cdouble *r = arr;
cdouble w = 0, d;

char buf[OUTPUT_BUFFER_SIZE] = ANSI_BUFFER_PROLOGUE;

static void zero_buffer_range(void)
{
	memset(buf + FILL_RANGE_OFFSET, 0, FILL_RANGE_SIZE);
}

static void format_buffer_range(void)
{
	for (int i = 0; i < FILL_RANGE_SIZE; i++) {
		int j = i + FILL_RANGE_OFFSET;

		char old = buf[j];

		if ((i + 1) % 80 == 0)
			buf[j] = '\n';
		else
			buf[j] = LOOKUP[old];
	}
}

static void init(void)
{
	for (char chr = getc(stdin); chr != EOF; chr = getc(stdin)) {
		if (chr <= '\n') {
			w = (int)(w + 2);
			continue;
		}

		if (chr > ' ') {
			r[0] = w;
			r[5] = w + 1;
			r[1] = r[6] = (chr == '#');

			r += 10;
		}

		w -= I;
	}
}

static void step12(void (*outer)(cdouble*, cdouble*),
	void (*inner)(cdouble*, cdouble*), void (*inner2)(cdouble*, cdouble*))
{
	cdouble *p, *q;

	for (p = arr; p < r; p += 5) {
		outer(p, q);

		for (q = arr; q < r; q += 5) {
			inner(p, q);

			if ((int)(1 - w) > 0)
				inner2(p, q);
		}
		// TODO: Do we need to repeat this?
		inner(p, q);
	}
	// TODO: Do we need to repeat this?
	outer(p, q);
}

static void step1_outer(cdouble *p, cdouble *q)
{
	p[2] = p[1] * 9;
}

static void step1_inner(cdouble *p, cdouble *q)
{
	d = *p - *q;
	w = cabs(d) / 2 - 1;
}

static void step1_inner2(cdouble *p, cdouble *q)
{
	p[2] += w * w;
}

static void step2_outer(cdouble *p, cdouble *q)
{
	p[3] = G;
}

static void step2_inner(cdouble *p, cdouble *q)
{
	d = *p - *q;
	w = cabs(d) / 2 - 1;
}

static void step2_inner2(cdouble *p, cdouble *q)
{
	cdouble tmp = (3 - p[2] - q[2]) * d * P + (p[4] - q[4]) * V;
	p[3] += w * tmp / p[2];
}

static void step4(void)
{
	cdouble *p, val;

	for (p = arr; p < r; p += 5) {
		val = *p;

		// Truncate to ints.
		int i = val * I;
		int j = val / 2;

		char *t = &buf[10 + i + 80 * j];

		p[4] += p[3] / 10 * !p[1];
		val += p[4];

		if (0 <= i && i < 79 && 0 <= j && j < 23) {
			t[0] |= 8;
			t[1] |= 4;
			i = t[80] |= 2;
			t[81] = 1;
		} else {
			i = 0;
		}

		*p = val;
	}
	// TODO: Do we need to repeat this?
	p[4] += p[3] / 10 * !p[1];
	val += p[4];
}

int main(void)
{
	init();

	for(char *out = buf; ; out = buf + 4) {
		puts(out);

		step12(step1_outer, step1_inner, step1_inner2);
		step12(step2_outer, step2_inner, step2_inner2);
		zero_buffer_range();
		step4();
		format_buffer_range();

		usleep(FRAME_SLEEP_NANO);
	}

	return 0;
}
