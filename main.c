#include <complex.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define COMPLEX_BUFFER_SIZE  19538
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

typedef double complex cdouble_t;

typedef struct {
	cdouble_t pos;
	bool is_wall;
	cdouble_t density, force, velocity;
} particle_t;

#define FN_OUTER(__name) void (*__name)(particle_t*)
#define FN_INNER(__name) void (*__name)(cdouble_t, cdouble_t, particle_t*, particle_t*)

static int read_particles(cdouble_t *wp, particle_t *ps)
{
	cdouble_t pos = 0;
	int i = 0;

	for (char chr = getc(stdin); chr != EOF; chr = getc(stdin)) {
		if (chr <= '\n') {
			pos = (int)(pos + 2);
			continue;
		}

		if (chr > ' ') {
			ps[i].pos = pos;
			ps[i + 1].pos = pos + 1;
			ps[i].is_wall = ps[i + 1].is_wall = (chr == '#');

			i += 2;
		}

		pos -= I;
	}

	*wp = pos;
	return i;
}

static void calc1(particle_t *arr, int len, cdouble_t *wp, FN_OUTER(outer),
		FN_INNER(inner2))
{
	cdouble_t d = 0, w = *wp;

	for(int i = 0; i < len; i++) {
		particle_t *particle1 = &arr[i];

		outer(particle1);

		for(int j = 0; j < len; j++) {
			particle_t *particle2 = &arr[j];

			d = particle1->pos - particle2->pos;
			w = cabs(d) / 2 - 1;

			if ((int)(1 - w) > 0)
				inner2(d, w, particle1, particle2);
		}
	}

	*wp = w;
}

static void calc1_1_outer(particle_t *p)
{
	p->density = p->is_wall * 9;
}

static void calc1_1_inner2(cdouble_t d, cdouble_t w, particle_t *p, particle_t *q)
{
	p->density += w * w;
}

static void calc1_2_outer(particle_t *p)
{
	p->force = G;
}

static void calc1_2_inner2(cdouble_t d, cdouble_t w, particle_t *p, particle_t *q)
{
	cdouble_t tmp = (3 - p->density - q->density) * d * P +
		(p->velocity - q->velocity) * V;
	p->force += w * tmp / p->density;
}

static void zero_buffer_range(char *buf)
{
	memset(buf + FILL_RANGE_OFFSET, 0, FILL_RANGE_SIZE);
}

static void calc2(char *buf, particle_t *arr, int len)
{
	for (int i = 0; i < len; i++) {
		particle_t *particle = &arr[i];

		// Intentionally truncate.
		int x = particle->pos * I; // We store -ve, I^2 = -1.
		int y = particle->pos / 2;

		char *t = &buf[10 + x + 80 * y];

		particle->velocity += particle->force / 10 * !particle->is_wall;
		particle->pos += particle->velocity;

		if (0 <= x && x < 79 && 0 <= y && y < 23) {
			t[0] |= 8;
			t[1] |= 4;
			t[80] |= 2;
			t[81] = 1;
		}
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
	particle_t arr[COMPLEX_BUFFER_SIZE] = { { 0 } };
	cdouble_t w = 0;

	int len = read_particles(&w, arr);

	puts(buf);
	while (1) {
		puts(buf + 4);

		calc1(arr, len, &w, calc1_1_outer, calc1_1_inner2);
		calc1(arr, len, &w, calc1_2_outer, calc1_2_inner2);
		zero_buffer_range(buf);
		calc2(buf, arr, len);
		format_buffer_range(buf);

		usleep(FRAME_SLEEP_NANO);
	}

	return 0;
}
