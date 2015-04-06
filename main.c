#include <complex.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define PARTICLE_BUFFER_SIZE 19538
#define OUTPUT_BUFFER_SIZE   6856
#define FRAME_SLEEP_NANO     12321
#define FILL_RANGE_SIZE      2003
#define LOOKUP               " '`-.|//,\\|\\_\\/#\n"

// See https://en.wikipedia.org/wiki/ANSI_escape_code#Sequence_elements
#define ANSI_SEQUENCE(__code)      "\x1b[" __code
#define ANSI_CLEAR                 ANSI_SEQUENCE("2J")
#define ANSI_MOVE_CURSOR(__n, __m) ANSI_SEQUENCE(#__n ";" #__m "H")
#define ANSI_BUFFER_PROLOGUE       ANSI_CLEAR ANSI_MOVE_CURSOR(1, 1)
#define ANSI_CLEAR_LENGTH          4
#define ANSI_PROLOGUE_LENGTH       10

#define FN_OUTER(__name) void (*__name)(particle_t*)
#define FN_INNER(__name) void (*__name)(cdouble_t, double, particle_t*, particle_t*)

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
	/*
	 *
	 *   *-------> x
	 *   |
	 *   |   . (x, y) encoded as y - xI.
	 *   |
	 *   v y
	 *
	 */
	cdouble_t pos;
	bool is_wall;
	cdouble_t density, force, velocity;
} particle_t;

/*
 * Reads ASCII model of particles and barriers to be simulated from stdin.
 *
 * Populates ps with particle data and end_pos with the position immediately
 * after the last character read.
 *
 * Returns the number of particles read.
 *
 * WARNING: No bounds checking is applied here so the buffer can overrun.
 */
static int read_particles(particle_t *ps)
{
	cdouble_t pos = 0;
	int i = 0;

	// Each input character corresponds to a 1x2 entry in our model.
	for (char chr = getc(stdin); chr != EOF; chr = getc(stdin)) {
		if (chr == '\n') {
			// Since input particles are of height 2, newline needs
			// to increment y by 2.
			pos += 2;
			pos = creal(pos);

			continue;
		}

		if (chr > ' ') {
			// We've detected an actual particle. We double up the
			// height to 2.
			ps[i].pos = pos;
			ps[i + 1].pos = pos + 1;

			// The # character signifies a 'wall' (solid barrier.)
			ps[i].is_wall = ps[i + 1].is_wall = (chr == '#');

			i += 2;
		}

		pos -= I;
	}

	return i;
}

static void calc(particle_t *particles, int len, FN_OUTER(update_source),
		FN_INNER(update_proximate))
{
	// Compare each pair of particles.
	for(int i = 0; i < len; i++) {
		particle_t *from = &particles[i];

		update_source(from);

		for(int j = 0; j < len; j++) {
			particle_t *to = &particles[j];

			cdouble_t delta = from->pos - to->pos;
			double distance = cabs(delta);

			if (distance <= 2)
				update_proximate(delta, distance / 2 - 1, from, to);
		}
	}
}

static void init_density(particle_t *p)
{
	p->density = p->is_wall ? 9 : 0;
}

static void update_density(cdouble_t delta, double distance, particle_t *from,
			particle_t *to)
{
	from->density += distance * distance;
}

static void apply_gravity(particle_t *p)
{
	p->force = G;
}

static void update_force(cdouble_t delta, double distance, particle_t *from,
			particle_t *to)
{
	cdouble_t velocity_delta = from->velocity - to->velocity;
	cdouble_t tmp1 = (3 - from->density - to->density);

	cdouble_t tmp2 = tmp1 * delta * P + velocity_delta * V;
	from->force += distance * tmp2 / from->density;
}

static void update_particle_dynamics(particle_t *particles, int len)
{
	calc(particles, len, init_density, update_density);
	calc(particles, len, apply_gravity, update_force);
}

static void zero_buffer_range(char *buf)
{
	memset(buf + ANSI_PROLOGUE_LENGTH, 0, FILL_RANGE_SIZE);
}

static void format_buffer_range(char *buf)
{
	for (int i = 0; i < FILL_RANGE_SIZE; i++) {
		int j = i + ANSI_PROLOGUE_LENGTH;

		int old = buf[j];

		if ((i + 1) % 80 == 0)
			buf[j] = '\n';
		else
			buf[j] = LOOKUP[old];
	}
}

static void write_to_buffer(char *buf, particle_t *particles, int len)
{
	zero_buffer_range(buf);

	for (int i = 0; i < len; i++) {
		particle_t *particle = &particles[i];

		// Intentionally truncate.
		int x = particle->pos * I; // We store -ve, I^2 = -1.
		int y = particle->pos / 2; // Particle height is 2.

		char *t = &buf[ANSI_PROLOGUE_LENGTH + x + 80 * y];

		if (0 <= x && x < 79 && 0 <= y && y < 23) {
			t[0] |= 8;
			t[1] |= 4;
			t[80] |= 2;
			t[81] = 1;
		}
	}

	format_buffer_range(buf);
}

static void update_position(particle_t *particles, int len)
{
	for (int i = 0; i < len; i++) {
		particle_t *particle = &particles[i];

		if (particle->is_wall)
			continue;

		particle->velocity += particle->force / 10;
		particle->pos += particle->velocity;
	}
}

static void output_buffer(char *buf)
{
	// Skip clear screen.
	puts(buf + ANSI_CLEAR_LENGTH);
}

int main(void)
{
	char buf[OUTPUT_BUFFER_SIZE] = ANSI_BUFFER_PROLOGUE;
	particle_t particles[PARTICLE_BUFFER_SIZE] = { { 0 } };

	int len = read_particles(particles);

	// Clear screen and reset cursor.
	puts(buf);

	// Run our simulation forever.
	while (1) {
		update_particle_dynamics(particles, len);

		write_to_buffer(buf, particles, len);
		output_buffer(buf);

		update_position(particles, len);

		usleep(FRAME_SLEEP_NANO);
	}

	return 0;
}
