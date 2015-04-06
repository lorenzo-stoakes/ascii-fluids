#ifndef __fluids
#define __fluids

#include <stdbool.h>
#include <complex.h>
#include <stdio.h>


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

// read.c
int read_particles(particle_t *);

// write.c
void write_to_buffer(char *, particle_t *, int);
void output_buffer(char *);

// calc.c
void update_particle_dynamics(particle_t *, int);
void update_position(particle_t *, int);

#endif
