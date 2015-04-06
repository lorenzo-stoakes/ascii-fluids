#include <complex.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define WIDTH 80
#define HEIGHT 25
#define PARTICLE_BUFFER_SIZE (WIDTH * HEIGHT)
#define FRAME_SLEEP_NANO     12321
#define LOOKUP               " '`-.|//,\\|\\_\\/#"

// See https://en.wikipedia.org/wiki/ANSI_escape_code#Sequence_elements
#define ANSI_SEQUENCE(__code)      "\x1b[" __code
#define ANSI_CLEAR                 ANSI_SEQUENCE("2J")
#define ANSI_MOVE_CURSOR(__n, __m) ANSI_SEQUENCE(#__n ";" #__m "H")
#define ANSI_BUFFER_PROLOGUE       ANSI_CLEAR ANSI_MOVE_CURSOR(1, 1)
#define ANSI_CLEAR_LENGTH          4
#define ANSI_PROLOGUE_LENGTH       10

// Add extra column to account for newlines.
#define OUTPUT_BUFFER_SIZE   (ANSI_PROLOGUE_LENGTH + PARTICLE_BUFFER_SIZE + HEIGHT )

#define FN_OUTER(__name) void (*__name)(particle_t*)
#define FN_INNER(__name) void (*__name)(cdouble_t, double, particle_t*, particle_t*)

// User-definable constants.

// Gravity - multiple of 10 ms^-2.
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

// Convert buffer particle flags to ASCII characters and insert newlines in the
// right places.
static void buffer_to_ascii(char *buf)
{
	for (int i = 0; i < OUTPUT_BUFFER_SIZE - ANSI_PROLOGUE_LENGTH - 1; i++) {
		int j = i + ANSI_PROLOGUE_LENGTH;

		if ((i + 1) % WIDTH == 0) {
			buf[j] = '\n';
			continue;
		}

		int offset = buf[j];
		buf[j] = LOOKUP[offset];
	}
}

// Read len particles and write them to the supplied buffer as ASCII.
static void write_to_buffer(char *buf, particle_t *particles, int len)
{
	// Zero the output buffer other than ANSI prologue so we can set flags
	// to denote particles and adjacent.
	memset(buf + ANSI_PROLOGUE_LENGTH, 0, OUTPUT_BUFFER_SIZE -
		ANSI_PROLOGUE_LENGTH);

	for (int i = 0; i < len; i++) {
		particle_t *particle = &particles[i];

		int x = -cimag(particle->pos);
		int y = creal(particle->pos) / 2; // Particle height is 2.

		// Particles are of height 2, need to be able to write to bottom
		// right.
		if (x < 0 || x >= WIDTH - 1 || y < 0 || y >= HEIGHT - 2)
			continue;

		int curr = ANSI_PROLOGUE_LENGTH + x + WIDTH * y;
		int right = curr + 1, below = curr + WIDTH,
			below_right = below + 1;

		/*
		 * This code cleverly provides the core of the ASCII animation
		 * by flipping bits to denote proximity to other particles. The
		 * carefully chosen different possibilities make for
		 * graphical-like output:-
		 *
		 * Empty Space
		 *
		 * 0:  ' '
		 *
		 * Adjacent To Particle(s)
		 *
		 * 1:  "'" Below Right       ,
		 *                           '
		 *
		 * 2:  '`' Below             ,
		 *                           .
		 *
		 * 3:  '-' Below 2 Particles ,_
		 *                            -
		 *
		 * 4:  '.' Right             ,.
		 *
		 * 5:  '|' Right 2 Particles ,
		 *                           ||
		 *
		 * 6:  '/' Right + Below      ,
		 *                           ,/
		 *
		 * 7:  '/' Corner            ,_
		 *                           |/
		 * Particle
		 *
		 * 8:  ',' Isolated          ,
		 *
		 * 9:  '\' Below Right       ,
		 *                            \
		 *
		 * 10: '|' Below             ,
		 *                           |
		 *
		 * 11: '\' Below 2 Particles ,_
		 *                            \
		 *
		 * 12: '_' Right             ,_
		 *
		 * 13: '\' Right 2 Particles ,
		 *                           |\
		 *
		 * 14: '/' Right + Below      ,
		 *                           |/
		 *
		 * 15: '#' Corner            ,_
		 *                           |#
		 */

		buf[curr] |= 8;
		buf[right] |= 4;
		buf[below] |= 2;
		buf[below_right] = 1;
	}

	buffer_to_ascii(buf);
}

// Update velocity of all non-wall particles from force and subsequently, their
// positions.
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

// Output buffer, don't clear the screen to avoid flicker.
static void output_buffer(char *buf)
{
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
	do {
		update_particle_dynamics(particles, len);

		write_to_buffer(buf, particles, len);
		output_buffer(buf);

		update_position(particles, len);
	} while (usleep(FRAME_SLEEP_NANO) == 0);

	return 0;
}
