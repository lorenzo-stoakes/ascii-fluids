#include "fluids.h"
#include <math.h>
/*
 * Uses Smoothed Particle Hydrodynamics.
 *
 * See: https://en.wikipedia.org/wiki/Smoothed-particle_hydrodynamics
 * See: http://www.ioccc.org/2012/endoh1/hint.html
 * See: https://software.intel.com/en-us/articles/fluid-simulation-for-video-games-part-15
 *
 * To calculate any quantity A at point r, sum over all particles:-
 *
 * A(r) = Sum_j(m_j * (A_j / p_j) * W(|r - r_j|, h))
 *
 * m_j is the mass of particle j (we assume to be 1.)
 * p_j is the density of particle j
 * W is a 'kernel' function.
 * |r - r_j| is the distance between the particle in question and particle j.
 * h is 'smoothing' length over which W operates (we assume h = 1.)
 *
 */

static cdouble_t kernel(double distance)
{
	// TODO: Where is this kernel calculation derived from?
	return (distance / 2 - 1) * (distance / 2 - 1);
}

static void pairwise_spline(particle_t *particles, int len,
			FN_PAIR(update_nearby))
{
	for(int i = 0; i < len; i++) {
		particle_t *from = &particles[i];

		for(int j = 0; j < len; j++) {
			particle_t *to = &particles[j];

			cdouble_t delta = from->pos - to->pos;
			double distance = cabs(delta);

			if (distance > 2)
				continue;

			update_nearby(delta, distance, from, to);
		}
	}
}

static void update_density(cdouble_t delta, double distance, particle_t *from,
			particle_t *to)
{
	// p(r) = Sum_j(m_j * (p_j / p_j) * W(|r - r_j|, h))
	// With m_j assumed to be 1, p_j cancelled out and W == kernel, we
	// have:-
	from->density += kernel(distance);
}

static void update_force(cdouble_t delta, double distance, particle_t *from,
			particle_t *to)
{
	// TODO: Where does this equation originate from?
	cdouble_t force = (from->density + to->density - 3) * delta * P -
		(from->velocity - to->velocity) * V;

	// F(r) = Sum_j(m_j * (F_j / p_j) * W(|r - r_j|, h))
	// With m_j assumed to be 1 and W == kernel, we have:-
	from->force += (force / from->density) * kernel(distance);
}

static void init_particles(particle_t *particles, int len)
{
	for(int i = 0; i < len; i++) {
		particle_t *particle = &particles[i];

		particle->force = G;
		particle->density = particle->is_wall ? 9 : 0;
	}
}

// Use the magic of Smoothed-particle hydrodynamics.
void update_particle_dynamics(particle_t *particles, int len)
{
	init_particles(particles, len);

	pairwise_spline(particles, len, update_density);
	pairwise_spline(particles, len, update_force);
}

// Update velocity of all non-wall particles from force and subsequently, their
// positions.
void update_position(particle_t *particles, int len)
{
	for (int i = 0; i < len; i++) {
		particle_t *particle = &particles[i];

		if (particle->is_wall)
			continue;

		particle->velocity += particle->force / 10;
		particle->pos += particle->velocity;
	}
}
