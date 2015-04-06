#include <string.h>
#include "fluids.h"

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
void write_to_buffer(char *buf, particle_t *particles, int len)
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

// Output buffer, don't clear the screen to avoid flicker.
void output_buffer(char *buf)
{
	puts(buf + ANSI_CLEAR_LENGTH);
}
