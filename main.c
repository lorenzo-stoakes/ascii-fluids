#include <complex.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
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

#define FN_OUTER(__name) void (*__name)(cdouble*)
#define FN_INNER(__name) void (*__name)(cdouble, cdouble, cdouble*, cdouble*)

static cdouble read_chars(cdouble **rp)
{
	cdouble pos = 0;
	cdouble *r = *rp;

	for (char chr = getc(stdin); chr != EOF; chr = getc(stdin)) {
		if (chr <= '\n') {
			pos = (int)(pos + 2);
			continue;
		}

		if (chr > ' ') {
			r[0] = pos;
			r[5] = pos + 1;
			r[1] = r[6] = (chr == '#');

			r += 10;
		}

		pos -= I;
	}

	*rp = r;

	return pos;
}

static void calc1(cdouble *arr, cdouble *r, cdouble *wp, FN_OUTER(outer),
		FN_INNER(inner2))
{
	cdouble d = 0, w = *wp;
	cdouble *p, *q;

	for (p = arr; p < r; p += 5) {
		outer(p);

		for (q = arr; q < r; q += 5) {
			d = p[0] - q[0];
			w = cabs(d) / 2 - 1;

			if ((int)(1 - w) > 0)
				inner2(d, w, p, q);
		}
	}

	*wp = w;
}

static void calc1_1_outer(cdouble *p)
{
	p[2] = p[1] * 9;
}

static void calc1_1_inner2(cdouble d, cdouble w, cdouble *p, cdouble *q)
{
	p[2] += w * w;
}

static void calc1_2_outer(cdouble *p)
{
	p[3] = G;
}

static void calc1_2_inner2(cdouble d, cdouble w, cdouble *p, cdouble *q)
{
	cdouble tmp = (3 - p[2] - q[2]) * d * P + (p[4] - q[4]) * V;
	p[3] += w * tmp / p[2];
}


static void zero_buffer_range(char *buf)
{
	memset(buf + FILL_RANGE_OFFSET, 0, FILL_RANGE_SIZE);
}

static void calc2(char *buf, cdouble *arr, cdouble *r)
{
	cdouble *p;
	cdouble val;

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
			t[80] |= 2;
			t[81] = 1;
		}

		*p = val;
	}
}

static void format_buffer_range(char *buf)
{
	for (int i = 0; i < FILL_RANGE_SIZE; i++) {
		int j = i + FILL_RANGE_OFFSET;

		int old = buf[j];

		if ((i + 1) % 80 == 0)
			buf[j] = '\n';
		else
			buf[j] = LOOKUP[old];
	}
}

int main(void)
{
	char buf[OUTPUT_BUFFER_SIZE] = ANSI_BUFFER_PROLOGUE;
	cdouble arr[COMPLEX_BUFFER_SIZE];
	cdouble *r = arr;
	cdouble w = 0;

	w = read_chars(&r);

	puts(buf);
	while (1) {
		puts(buf + 4);

		calc1(arr, r, &w, calc1_1_outer, calc1_1_inner2);
		calc1(arr, r, &w, calc1_2_outer, calc1_2_inner2);
		zero_buffer_range(buf);
		calc2(buf, arr, r);
		format_buffer_range(buf);

		usleep(FRAME_SLEEP_NANO);
	}

	return 0;
}
