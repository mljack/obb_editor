/**
 * @file pv.cpp
 * @brief Physics simulation system implementation
 * 
 * This file implements a physics simulation system with various numerical integration methods
 * and physical models. It supports simulation of particle motion under different force fields,
 * collision detection and response, and trajectory tracking.
 */

#include "pv.h"
/* Physics Vector */

#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

#include <random>

/**
 * @brief Global simulation variables
 */
extern double g_sim_time;          ///< Current simulation time
extern bool g_simulating;          ///< Whether simulation is running
extern std::map<int, Marker> g_markers; ///< Markers for UI visualization
extern bool g_show_trajectories;   ///< Whether to show particle trajectories
std::vector<std::vector<vec2d>> g_env;  ///< Environment boundaries
std::vector<std::shared_ptr<Field>> g_fields;       ///< Force fields affecting particles
std::vector<Particle> g_particles;  ///< Simulated particles
std::vector<float> g_t_array, g_energy_array; ///< Energy tracking arrays

// Helper functions and GLM equivalents
// pi => glm::pi()
// angle() => glm::orientedAngle(), glm::angle()
// distance_v() => glm::distance()
// dot()
// cross()
// glm::length() 

/**
 * @brief Default acceleration function
 * @param pt Position vector
 * @param v Velocity vector
 * @return Acceleration vector (default is Earth's gravity)
 */
vec2d accel(const vec2d& pt, const vec2d& v) {
	return vec2d(0, 9.8);  // Default gravity acceleration
	//return vec2d(0, 9.8) + v * 0.1;  // Alternative with velocity-dependent term
}

/**
 * @brief Earth gravity field implementation
 * 
 * This class implements a constant gravity field similar to Earth's gravity,
 * with acceleration vector (0, 9.8) pointing downward in the coordinate system.
 */
vec2d GravityOnEarth::compute_accel(const vec2d& pos, const vec2d& vel) {
	return vec2d(0, 9.8);  // Constant downward acceleration
}

double GravityOnEarth::compute_potential(const vec2d& pos) {
	return 9.8 * (pos.y - 0.0);  // Gravitational potential energy relative to y=0
}

/**
 * @brief Central gravity field implementation (e.g., planetary gravity)
 * 
 * This class implements a central gravity field that follows the inverse-square law,
 * similar to gravity between celestial bodies.
 */
vec2d GravityInSpace::compute_accel(const vec2d& pos, const vec2d& vel) {
	vec2d r = center - pos;  // Vector from particle to center of mass
	double r2 = glm::dot(r, r);  // Squared distance
	return 200*30*30/(r2*std::sqrt(r2))*r;  // F = G*M*m/r² * r̂
}

double GravityInSpace::compute_potential(const vec2d& pos) {
	double r = glm::length(center - pos);  // Distance to center
	return -200 * 30 * 30 / r;  // Gravitational potential: -G*M*m/r
}

/**
 * @brief Air resistance force implementation
 * 
 * This class implements a drag force proportional to the velocity vector,
 * simulating air resistance effects.
 */
vec2d AirResistance::compute_accel(const vec2d& pos, const vec2d& vel) {
	double v2 = glm::dot(vel, vel);  // Squared velocity magnitude
	return -10.0 / std::sqrt(v2)*vel;  // Drag force: -k*v (normalized by velocity magnitude)
}


/**
 * @brief Starts a new physics simulation based on the selected problem
 * 
 * @param problem_idx Index of the problem to simulate:
 *        1 = Planet Orbit simulation
 *        2 = Badminton Clear Shot simulation
 *        3 = Rarefied Gas simulation
 * @param markers Pointer to the markers map for UI visualization
 */
void start_simulation(int problem_idx, std::map<int, Marker>* markers) {
	// Create the appropriate problem implementation based on the index
	if (problem_idx == 1)
		g_problem = std::make_unique<PlanetOrbit>();  // Planetary orbit simulation
	else if (problem_idx == 2)
		g_problem = std::make_unique<BadmintonClearShot>();  // Badminton trajectory simulation
	else if (problem_idx == 3)
		g_problem = std::make_unique<RarefiedGas>();  // Gas particle simulation
	else
		g_problem = nullptr;  // Invalid problem index

	// Initialize the problem with markers
	if (g_problem)
		g_problem->init(markers);

	// Start tracking trajectories if enabled
	if (g_show_trajectories) {
		for (auto& p : g_particles)
			p.traj.emplace_back(g_sim_time, p.pos);  // Record initial position
	}
}

/**
 * @brief Computes acceleration for all particles based on active force fields
 * 
 * This function iterates through all particles and calculates their acceleration
 * by summing contributions from all active force fields in the simulation.
 */
void compute_accel() {
	for (auto& p : g_particles) {
		p.accel = vec2d(0.0, 0.0);  // Reset acceleration
		for (auto& field : g_fields) {
			// Add acceleration contribution from each force field
			p.accel += field->compute_accel(p.pos, p.vel);
		}
	}
}

/**
 * @brief Runs a single step of the physics simulation using the specified integration method
 * 
 * @param timestep Time step size for the simulation
 * @param method_idx Integration method to use:
 *        0 = Forward Euler
 *        1 = Backward Euler
 *        2 = Implicit Trapezoid
 *        3 = Explicit Trapezoid
 *        4 = Taylor (2nd order)
 *        5 = Taylor (2nd order) + Explicit Trapezoid
 *        6 = Velocity Verlet
 */
void run_one_simulation_step(double timestep, int method_idx) {
	if (method_idx == 0) { 	// Forward Euler method (explicit)
		//printf("Forward Euler \n");
		compute_accel();  // Calculate accelerations at current state
		for (auto& p : g_particles) {
			p.pos += timestep * p.vel;  // Update position
			p.vel += timestep * p.accel;  // Update velocity
		}
	} else if (method_idx == 1) { 	// Backward Euler method (implicit)
		//printf("Backward Euler \n");
		compute_accel();  // Initial acceleration
		
		// Initial prediction
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		
		// Newton-Raphson iteration to solve implicit equations
		for (int i = 0; i < 10; ++i) {
			// Calculate accelerations at predicted state
			for (auto& p : g_particles) {
				p.accel_predicted = vec2d(0.0, 0.0);
				for (auto& field : g_fields) {
					p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
				}
			}
			
			// Refine predictions
			for (auto& p : g_particles) {
				p.pos_predicted = p.pos + timestep * p.vel_predicted;
				p.vel_predicted = p.vel + timestep * p.accel_predicted;
			}
		}
		
		// Apply the final predictions
		for (auto& p : g_particles) {
			p.pos += timestep * p.vel_predicted;
			p.vel += timestep * p.accel_predicted;
		}
	}
	else if (method_idx == 2) {	// Implicit Trapezoid method
		//printf("Implicit Trapezoid \n");
		compute_accel();
		
		// Initial prediction
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		
		// Iterative solution for implicit equations
		for (int i = 0; i < 10; ++i) {
			// Calculate accelerations at predicted state
			for (auto& p : g_particles) {
				p.accel_predicted = vec2d(0.0, 0.0);
				for (auto& field : g_fields) {
					p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
				}
			}
			
			// Refine predictions
			for (auto& p : g_particles) {
				p.pos_predicted = p.pos + timestep * p.vel_predicted;
				p.vel_predicted = p.vel + timestep * p.accel_predicted;
			}
		}
		
		// Apply trapezoidal update using average of current and predicted
		for (auto& p : g_particles) {
			p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	} else if (method_idx == 3) {	// Explicit Trapezoid method
	 //printf("Explicit Trapezoid \n");
		compute_accel();
		
		// Predict new state using Euler
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		
		// Calculate acceleration at predicted state
		for (auto& p : g_particles) {
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
			}
		}
		
		// Apply trapezoidal update (no iteration)
		for (auto& p : g_particles) {
			p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	} else if (method_idx == 4) {	// Taylor series method (2nd order)
		//printf("Taylor (2nd order) \n");
		compute_accel();
		
		// Update using Taylor expansion to 2nd order
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel += timestep * p.accel;
		}
	} else if (method_idx == 5) {	// Combined Taylor + Explicit Trapezoid method
		//printf("Taylor (2nd order) + Explicit Trapezoid \n");
		compute_accel();
		
		// Initial prediction using Taylor method
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		
		// Calculate acceleration at predicted state
		for (auto& p : g_particles) {
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
			}
		}
		
		// Apply combined update
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.25 * timestep * (p.accel + p.accel_predicted));
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	} else if (method_idx == 6) {	// Velocity Verlet method
		//printf("Velocity Verlet \n");
		compute_accel();
		
		// Update position and predict velocity
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		
		// Calculate new acceleration at updated position
		for (auto& p : g_particles) {
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos, p.vel_predicted);
			}
		}
		
		// Correct velocity using average acceleration
		for (auto& p : g_particles) {
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	}

	if (g_problem) {
		g_problem->handle_collision();
		g_problem->handle_boundary();
	}

	if (g_show_trajectories) {
		for (auto& p : g_particles)
			p.traj.emplace_back(g_sim_time, p.pos);
	}

	double E = 0.0;
	for (auto& p : g_particles) {
		if (p.solution) {
			p.solution(g_sim_time + timestep, &p.pos, &p.vel, &p.accel);
		} else {
			E += 0.5 * glm::dot(p.vel, p.vel);
			for (auto& field : g_fields) {
				E += field->compute_potential(p.pos);
			}
		}
	}
	if (isnan(E)) {
		g_simulating = false;
	} else {
		g_sim_time += timestep;
		g_t_array.push_back(g_sim_time);
		g_energy_array.push_back(E);
	}
}

/**
 * @brief Stops the simulation and updates markers with final particle states
 * 
 * This function updates all markers with the current position, velocity,
 * and acceleration of their corresponding particles.
 * 
 * @param markers Pointer to map of markers to be updated
 */
void stop_simulation(std::map<int, Marker>* markers) {
	// Update each marker with the corresponding particle's state
	for (auto& p : g_particles) {
		auto& m = markers->at(p.id);
		m.x = p.pos.x; m.y = p.pos.y;
		m.vx = p.vel.x; m.vy = p.vel.y;
		m.ax = p.accel.x; m.ay = p.accel.y;
	}
}

/**
 * @brief Jumps to a specific time point in the simulation history
 * 
 * This function updates markers to show the state of particles at the specified
 * time by searching through the recorded trajectory data.
 * 
 * @param t Target time to jump to (normalized between 0 and 1)
 * @param markers Pointer to map of markers to be updated
 */
void seek_to_sim_time_moment(double t, std::map<int, Marker>* markers) {
	// Scale time slightly to avoid boundary issues
	t *= 0.9999;
	int count = 0;
	// Update each marker based on trajectory data
	for (auto&[idx, m] : *markers) {
		if (count >= g_particles.size())
			continue;
		// Find the first trajectory point after the target time
		for (auto& traj_pt : g_particles[count++].traj) {
			if (traj_pt.t > t) {
				m.x = traj_pt.pos.x;
				m.y = traj_pt.pos.y;

				// Update the simulation time
				g_sim_time = t;
				break;
			}
		}
	}
}

/**
 * @brief Initializes the planet orbit simulation
 * 
 * Sets up a central gravitational field and places a single planet
 * in orbit around the center point.
 * 
 * @param markers Pointer to map of markers to be initialized
 */
void PlanetOrbit::init(std::map<int, Marker>* markers) {
	// Clear existing force fields
	g_fields.clear();
	// Create a central gravitational field at (800, -600)
	g_fields.push_back(std::make_shared<GravityInSpace>(vec2d(800.0, -600.0)));
	
	// Create a marker for the planet
	Marker m1;
	m1.x = 700.0f;    // Position 100 units left of center
	m1.y = -600.0f;   // Same y-coordinate as center
	m1.vy = -30.0f;   // Initial velocity to create circular orbit

	// Clear and initialize environment
	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(800.0, -600.0));  // Center point

	// Initialize markers and particles
	markers->clear();
	markers->emplace(0, m1);
	g_particles.resize(markers->size());
	int count = 0;
	for (auto&[idx, m] : *markers) {
		g_particles[count++].set(g_sim_time, idx, /*radius=*/4.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}

	// The commented code below would add an analytical solution for comparison
	//if (!g_particles.empty()) {
	//	Particle solution = g_particles[0];
	//	solution.traj.clear();
	//	solution.set_solution([](double t, vec2d* pos, vec2d* vel, vec2d* accel) {
	//		double r = 100.0;
	//		*pos = vec2d(800.0, -600.0) + r * vec2d(std::cos(t), std::sin(t));
	//	});
	//	g_particles.push_back(solution);
	//}
}

/**
 * @brief Initializes the badminton clear shot simulation
 * 
 * Sets up gravity and air resistance fields, places two shuttlecocks
 * with different initial velocities, and defines a ground boundary.
 * 
 * @param markers Pointer to map of markers to be initialized
 */
void BadmintonClearShot::init(std::map<int, Marker>* markers) {
	// Clear existing force fields
	g_fields.clear();
	// Add Earth's gravity
	g_fields.push_back(std::make_shared<GravityOnEarth>());
	// Add air resistance
	g_fields.push_back(std::make_shared<AirResistance>());

	// Create two markers for shuttlecocks with different initial velocities
	Marker m1, m2;
	// First shuttlecock (lower velocity)
	m1.x = 700.0f;    // Starting x-position
	m1.y = -700.0f;   // Starting y-position (above ground)
	m1.vx = 80.0f;    // Horizontal velocity
	m1.vy = -80.0f;   // Vertical velocity (upwards)

	// Second shuttlecock (higher velocity)
	m2.x = 700.0f;
	m2.y = -700.0f;
	m2.vx = 80.0f;
	m2.vy = -90.0f;   // Higher upward velocity

	// Define ground boundary
	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(0.0, -600.0));    // Left ground point
	g_env.back().push_back(vec2d(1500.0, -600.0));  // Right ground point

	// Initialize markers and particles
	markers->clear();
	markers->emplace(0, m1);
	markers->emplace(1, m2);
	g_particles.resize(markers->size());
	int count = 0;
	for (auto&[idx, m] : *markers) {
		g_particles[count++].set(g_sim_time, idx, /*radius=*/4.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}
}

/**
 * @brief Initializes the rarefied gas simulation
 * 
 * Creates a rectangular container and fills it with many gas particles
 * with randomly distributed positions and velocities.
 * 
 * @param markers Pointer to map of markers to be initialized
 */
void RarefiedGas::init(std::map<int, Marker>* markers) {
	// Clear existing force fields (no gravity in this simulation)
	g_fields.clear();
	//g_fields.push_back(std::make_shared<GravityOnEarth>());

	// Define a rectangular container boundary
	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(600.0, -600.0));    // Top-left
	g_env.back().push_back(vec2d(1000.0, -600.0));   // Top-right
	g_env.back().push_back(vec2d(1000.0, -1000.0));  // Bottom-right
	g_env.back().push_back(vec2d(600.0, -1000.0));   // Bottom-left
	g_env.back().push_back(vec2d(600.0, -600.0));    // Close the rectangle

	// Set up random number generators for particle positions and velocities
	std::default_random_engine generator(1234);       // Fixed seed for reproducibility
	std::uniform_real_distribution<double> xy_dist(0.0 + 0.1, 200.0 - 0.1);  // Position distribution within container
	std::uniform_real_distribution<double> vel_dist(-5.0, 5.0);            // Random velocity components

	// Create 1000 gas particles with random positions and velocities
	markers->clear();
	Marker m;
	for (int i = 0; i < 1000; ++i) {
		// Position within container (offset by container coordinates)
		m.x = xy_dist(generator) + 600.0;
		m.y = xy_dist(generator) - 1000.0;
		// Random velocities
		m.vx = vel_dist(generator);
		m.vy = vel_dist(generator);
		markers->emplace(i, m);
	}

	// Initialize particles from markers
	g_particles.resize(markers->size());
	int count = 0;
	for (auto&[idx, m] : *markers) {
		// Small radius for gas particles
		g_particles[count++].set(g_sim_time, idx, /*radius=*/1.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}
}

/**
 * @brief Handles particle collisions with container boundaries
 * 
 * Implements perfect elastic reflections when particles hit the container walls.
 * Reverses the appropriate velocity component when a particle is heading toward a boundary.
 */
void RarefiedGas::handle_boundary() {
	// Check each particle against container walls
	for (auto& p : g_particles) {
		// Left and right walls - reflect x-velocity
		if ((p.pos.x < 600.0 && p.vel.x < 0.0) || (p.pos.x > 1000.0 && p.vel.x > 0.0))
			p.vel.x *= -1;
		// Bottom and top walls - reflect y-velocity
		if ((p.pos.y < -1000.0 && p.vel.y < 0.0) || (p.pos.y > -600.0 && p.vel.y > 0.0))
			p.vel.y *= -1;
	}
}

/**
 * @brief Handles collisions between gas particles
 * 
 * Implements elastic collisions between particles, ensuring momentum and kinetic energy
 * are conserved during interactions.
 */
void RarefiedGas::handle_collision() {
	// Reset collision flags for all particles
	for (auto& p : g_particles)
		p.is_colliding = false;

	// Check all pairs of particles for collisions
	for (int i = 0; i < g_particles.size(); ++i) {
		for (int j = i + 1; j < g_particles.size(); ++j) {
			vec2d diff = g_particles[i].pos - g_particles[j].pos;
			double dist2 = glm::dot(diff, diff);
			auto pair = std::make_pair(i, j);
			bool already_in_colliding = (collision_pairs.count(pair) != 0);
			double R = g_particles[i].radius + g_particles[j].radius;
			if (dist2 < R * R) {
				vec2d dir = 1 / std::sqrt(dist2) * diff;
				double v1 = glm::dot(dir, g_particles[i].vel);
				double v2 = glm::dot(dir, g_particles[j].vel);
				//if (v2 - v1 < 0.0) {
				//	collision_pairs.erase(pair);
				//	continue;
				//}
				if (already_in_colliding)
					continue;
				collision_pairs.insert(pair);
				g_particles[i].is_colliding = true;
				g_particles[j].is_colliding = true;
				g_particles[i].vel += v2 * dir - v1 * dir;
				g_particles[j].vel += v1 * dir - v2 * dir;
				continue;
			}
			else {
				collision_pairs.erase(pair);
			}
		}
	}
}

