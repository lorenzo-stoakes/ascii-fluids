#include <stdio.h>
#include <unistd.h>

#include "fluids.h"

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
