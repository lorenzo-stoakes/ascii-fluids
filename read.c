#include "fluids.h"

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
int read_particles(particle_t *ps)
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
