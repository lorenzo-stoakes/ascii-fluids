#include "fluids.h"

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

// Use the magic of Smoothed-particle hydrodynamics.
void update_particle_dynamics(particle_t *particles, int len)
{
	calc(particles, len, init_density, update_density);
	calc(particles, len, apply_gravity, update_force);
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
