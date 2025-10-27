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
#include <chrono>
#include <thread>
#include <mutex>

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
double g_max_particle_radius = 0.00001;

// Speed distribution statistics global variables
std::vector<int> g_speed_hist;
const int SPEED_BINS = 200;
double g_max_speed = 20.0; // Dynamic maximum speed

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
		for (auto& p : g_particles)
			if (g_show_trajectories || p.show_trajectory)
				p.traj.emplace_back(g_sim_time, p.pos);  // Record initial position
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
	auto t0 = std::chrono::high_resolution_clock::now();
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
	auto t1 = std::chrono::high_resolution_clock::now();

	if (g_problem)
		g_problem->handle_collision();

	auto t2 = std::chrono::high_resolution_clock::now();

	if (g_problem)
		g_problem->handle_boundary();

	auto t3 = std::chrono::high_resolution_clock::now();

	for (auto& p : g_particles)
		if (g_show_trajectories || p.show_trajectory)
			p.traj.emplace_back(g_sim_time, p.pos);

	double E = 0.0;
	for (auto& p : g_particles) {
		if (p.solution) {
			p.solution(g_sim_time + timestep, &p.pos, &p.vel, &p.accel);
		} else {
			E += 0.5 * p.mass * glm::dot(p.vel, p.vel);
			for (auto& field : g_fields) {
				E += field->compute_potential(p.pos);
			}
		}
		if (isnan(E))
			printf("Found NaN for the particle %d\n", p.id);
	}

	auto t4 = std::chrono::high_resolution_clock::now();
	double time_integrator = std::chrono::duration<double, std::milli>(t1 - t0).count();
	double time_collision = std::chrono::duration<double, std::milli>(t2 - t1).count();
	double time_boundary = std::chrono::duration<double, std::milli>(t3 - t2).count();
	double time_energy = std::chrono::duration<double, std::milli>(t4 - t3).count();
	printf("integrator: %.2f, collision: %.2f, boundary: %.2f, energy: %.2f\n", time_integrator, time_collision, time_boundary, time_energy);

	if (isnan(E)) {
		g_simulating = false;
	} else {
		g_sim_time += timestep;
		g_t_array.push_back(g_sim_time);
		g_energy_array.push_back(E);

		// Update maximum speed of all particles
		double max_speed = 0.0;
		for (const auto& p : g_particles)
			max_speed = std::max(max_speed, glm::length(p.vel));

		// Add a small buffer (20%) to ensure all particles are visible
		g_max_speed = max_speed * 1.2;
		// Ensure minimum value to avoid empty plots
		if (g_max_speed < 0.1) {
			g_max_speed = 0.1;
		}
		
		// Calculate speed distribution
		g_speed_hist.assign(SPEED_BINS, 0);
		for (const auto& p : g_particles) {
			// Calculate speed (velocity magnitude)
			double speed = glm::length(p.vel);
			
			// Map speed to histogram bin
			int bin = static_cast<int>((speed / g_max_speed) * SPEED_BINS);
			bin = std::max(0, std::min(bin, SPEED_BINS - 1));
			g_speed_hist[bin]++;
		}
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
		g_particles[count++].set(g_sim_time, idx, /*color_idx=*/0, /*radius=*/4.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
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
		g_particles[count++].set(g_sim_time, idx, /*color_idx=*/0, /*radius=*/4.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
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
	// Initialize boundary variables
	container_min_x = 600.0;
	container_max_x = 1000.0;
	container_min_y = -1000.0;
	container_max_y = -600.0;
	center_x = (container_min_x + container_max_x) / 2;
	center_y = (container_min_y + container_max_y) / 2;
	wall_width = 40.0;
	wall_min_x = center_x - wall_width / 2;
	wall_max_x = center_x + wall_width / 2;
	hole_min_y = center_y - wall_width / 2;
	hole_max_y = center_y + wall_width / 2;

	num_of_particles = 100000;
	double particle_radius = 0.1;

	// Clear existing force fields (no gravity in this simulation)
	g_fields.clear();
	g_fields.push_back(std::make_shared<GravityOnEarth>());

	// Define a rectangular container boundary
	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(container_min_x, container_max_y));    // Top-left
	g_env.back().push_back(vec2d(container_max_x, container_max_y));   // Top-right
	g_env.back().push_back(vec2d(container_max_x, container_min_y));  // Bottom-right
	g_env.back().push_back(vec2d(container_min_x, container_min_y));   // Bottom-left
	g_env.back().push_back(vec2d(container_min_x, container_max_y));    // Close the rectangle

	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(wall_min_x, container_min_y));
	g_env.back().push_back(vec2d(wall_min_x, hole_min_y));
	g_env.back().push_back(vec2d(wall_max_x, hole_min_y));
	g_env.back().push_back(vec2d(wall_max_x, container_min_y));

	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(wall_max_x, container_max_y));
	g_env.back().push_back(vec2d(wall_max_x, hole_max_y));
	g_env.back().push_back(vec2d(wall_min_x, hole_max_y));
	g_env.back().push_back(vec2d(wall_min_x, container_max_y));

	// Set up random number generators for particle positions and velocities
	std::default_random_engine generator(1234);       // Fixed seed for reproducibility
	std::uniform_real_distribution<double> x_dist(container_min_x + 0.1, (container_min_x + container_max_x) / 2 - wall_width / 2 - 0.1);
	std::uniform_real_distribution<double> x_dist2((container_min_x + container_max_x) / 2 + wall_width / 2 + 0.1, container_max_x - 0.1);
	std::uniform_real_distribution<double> y_dist(center_y + 0.1, container_max_y - 0.1);
	std::uniform_real_distribution<double> vel_dist(-5.0, 5.0);

	markers->clear();

	// Create gas particles with random positions and velocities
	g_particles.resize(num_of_particles);
	int count = 0;
	for (int idx = 0; idx < g_particles.size(); ++idx) {
		double x = x_dist(generator);
		double x2 = x_dist2(generator);
		double y = y_dist(generator);
		double vx = vel_dist(generator);
		double vy = vel_dist(generator);
		if (idx % 2 == 0)
			g_particles[idx].set(g_sim_time, idx, /*color_idx=*/0, particle_radius, /*mass=*/1.0, vec2d(x,  y), vec2d(vx, vy), vec2d(0.0, 0.0));
		else
			g_particles[idx].set(g_sim_time, idx, /*color_idx=*/2, particle_radius, /*mass=*/1.0, vec2d(x2, y), vec2d(vx, vy), vec2d(0.0, 0.0));
	}

	g_particles.back().show_trajectory = true;
	Particle p;
	p.show_trajectory = true;
	p.set(g_sim_time, g_particles.size(), /*color_idx=*/1, particle_radius * 10, /*mass=*/30.0, vec2d(center_x, center_y), vec2d(0.0, 0.0), vec2d(0.0, 0.0));

	p.pos.x = center_x - 150.0;
	p.pos.y = center_y - 100.0;
	p.set_radius(20.0);
	p.mass = p.radius * p.radius * glm::pi<double>() * 0.01;
	g_particles.push_back(p);

	p.pos.x = center_x - 70.0;
	p.pos.y = center_y - 100.0;
	p.set_radius(20.0);
	p.mass = p.radius * p.radius * glm::pi<double>() * 0.1;
	g_particles.push_back(p);

	p.pos.x = center_x + 70.0;
	p.pos.y = center_y - 100.0;
	p.set_radius(20.0);
	p.mass = p.radius * p.radius * glm::pi<double>() * 0.5;
	g_particles.push_back(p);

	p.pos.x = center_x + 150.0;
	p.pos.y = center_y - 100.0;
	p.set_radius(20.0);
	p.mass = p.radius * p.radius * glm::pi<double>() * 0.9;
	g_particles.push_back(p);
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
		if ((p.pos.x < container_min_x + p.radius && p.vel.x < 0.0) || (p.pos.x > container_max_x - p.radius && p.vel.x > 0.0)) {
			p.vel.x *= -1;
			p.is_colliding = true;
		}
		// Bottom and top walls - reflect y-velocity
		if ((p.pos.y < container_min_y + p.radius && p.vel.y < 0.0) || (p.pos.y > container_max_y - p.radius && p.vel.y > 0.0)) {
			p.vel.y *= -1;
			p.is_colliding = true;
		}

		if (p.pos.x > wall_min_x - p.radius && p.pos.x < wall_max_x + p.radius && (p.pos.y < hole_min_y + p.radius || p.pos.y > hole_max_y - p.radius)) {
			double min_y = std::min(std::abs(p.pos.y - (hole_min_y + p.radius)), std::abs(p.pos.y - (hole_max_y - p.radius)));
			double min_x = std::min(std::abs(p.pos.x - (wall_min_x - p.radius)), std::abs(p.pos.x - (wall_max_x + p.radius)));
			if (min_x < min_y) {
				if ((p.pos.x > wall_min_x - p.radius && p.vel.x < 0.0) || (p.pos.x < wall_max_x + p.radius && p.vel.x > 0.0))
					p.vel.x *= -1;
			} else {
				if ((p.pos.y < hole_min_y + p.radius && p.vel.y < 0.0) || (p.pos.y > hole_max_y - p.radius && p.vel.y > 0.0))
					p.vel.y *= -1;
			}
			p.is_colliding = true;
		}

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
 
	// Space partitioning optimization using grid system
	double container_width = container_max_x - container_min_x;
	double container_height = container_max_y - container_min_y;
	// Adaptive grid size: ensures grid is at least particle diameter and constains at least one particle when distributes particles evenly
	double grid_size = std::max(g_max_particle_radius * 2, std::sqrt(container_width * container_height / num_of_particles));
	// printf("grid_size: %f, max_grid_x: %f\n", grid_size, container_width / grid_size);
	// Factor to combine grid_x and grid_y into a unique key (power of 2 for fast multiplication)
	int grid_width_factor = 1024;
	
	// Create grid: map from grid coordinates to list of particle indices
	std::unordered_map<int, std::vector<int>> grid;
	
	// Function to get grid key from particle position
	auto get_grid_key = [&](double x, double y) -> int {
		// Convert world coordinates to grid coordinates
		int grid_x = static_cast<int>((x - this->container_min_x) / grid_size);
		int grid_y = static_cast<int>((y - this->container_min_y) / grid_size);
		// Ensure grid coordinates are within bounds
		grid_x = std::max(0, std::min(grid_x, static_cast<int>(container_width / grid_size) - 1));
		grid_y = std::max(0, std::min(grid_y, static_cast<int>(container_height / grid_size) - 1));
		// Create a unique key for the grid cell
		return grid_y * grid_width_factor + grid_x;
	};
 
	// Populate the grid with particle indices
	for (int i = 0; i < g_particles.size(); ++i) {
		auto& p = g_particles[i];
		int key = get_grid_key(p.pos.x, p.pos.y);
		grid[key].push_back(i);
	}

	// Determine number of threads to use (use hardware concurrency if available)
	int num_threads = std::max(4U, std::thread::hardware_concurrency());
	
	// Create a vector to store thread objects
	std::vector<std::thread> threads;
	
	// Create a vector of mutexes to protect particle velocity updates
	// Each particle has its own mutex to ensure thread safety
	std::vector<std::mutex> particle_mutexes(g_particles.size());
	
	// Function to handle collisions for a range of particles
	auto handle_collision_range = [&](int start_idx, int end_idx) {
		for (int i = start_idx; i < end_idx; ++i) {
			auto& p = g_particles[i];
			
			// Check current grid cell and all 8 neighboring cells
			for (int dy = -1; dy <= 1; ++dy) {
				for (int dx = -1; dx <= 1; ++dx) {
					// Calculate neighboring grid cell coordinates
					int grid_x = static_cast<int>((p.pos.x - this->container_min_x) / grid_size) + dx;
					int grid_y = static_cast<int>((p.pos.y - this->container_min_y) / grid_size) + dy;
					
					// Check if the neighboring grid cell is within bounds
					if (grid_x < 0 || grid_x >= static_cast<int>(container_width / grid_size) ||
						grid_y < 0 || grid_y >= static_cast<int>(container_height / grid_size))
						continue;
						
					// Get key for neighboring grid cell
					int neighbor_key = grid_y * grid_width_factor + grid_x;
						
					// Check if the neighboring grid cell exists
					auto it = grid.find(neighbor_key);
					if (it == grid.end())
						continue;

					// Check collisions with all particles in the neighboring grid cell
					for (int j : it->second) {
						// Avoid checking the same pair twice (i < j)
						if (i >= j)
							continue;
							
						// Check collision between particles i and j
						vec2d diff = g_particles[i].pos - g_particles[j].pos;
						double dist2 = glm::dot(diff, diff);
						double R = g_particles[i].radius + g_particles[j].radius;
							
						if (dist2 > R * R)
							continue;

						// Calculate relative velocity vector
						vec2d rel_vel = g_particles[j].vel - g_particles[i].vel;
							
						// Calculate relative velocity along normal direction
						double rel_vn = glm::dot(rel_vel, diff);
							
						// Only process if particles are approaching each other
						if (rel_vn < 0.0)
							continue;

						// Set collision flags (no need for mutex as we're only setting to true)
						g_particles[i].is_colliding = true;
						g_particles[j].is_colliding = true;

						double m1 = g_particles[i].mass;
						double m2 = g_particles[j].mass;
							 
						// Calculate impulse scalar for elastic collision
						// Formula: j = 2 * m1 * m2 * rel_vn / (m1 + m2)
						double impulse = (2.0 * m1 * m2 * rel_vn) / (m1 + m2);
							
						// Calculate velocity changes before acquiring locks
						vec2d delta_v_i = (impulse / m1) * diff / dist2;
						vec2d delta_v_j = -(impulse / m2) * diff / dist2;
						
						// Use a consistent locking order to prevent deadlocks
						// Always lock the particle with the smaller index first
						// (i < j is guaranteed by earlier check)
						std::lock_guard<std::mutex> lock_i(particle_mutexes[i]);
						std::lock_guard<std::mutex> lock_j(particle_mutexes[j]);
						// Update velocities using impulse and normal
						g_particles[i].vel += delta_v_i;
						g_particles[j].vel += delta_v_j;
					}
				}
			}
		}
	};
	
	// Calculate the number of particles per thread
	int particles_per_thread = (g_particles.size() + num_threads - 1) / num_threads;
	
	// Launch threads to handle different ranges of particles
	for (int t = 0; t < num_threads; ++t) {
		int start_idx = t * particles_per_thread;
		int end_idx = (t == num_threads - 1) ? g_particles.size() : (t + 1) * particles_per_thread;
		threads.emplace_back(handle_collision_range, start_idx, end_idx);
	}
	
	// Wait for all threads to complete
	for (auto& thread : threads) {
		if (thread.joinable())
			thread.join();
	}
}

