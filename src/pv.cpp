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
#include <mutex>

#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>

//#include <tbb/global_control.h>
//tbb::global_control c(tbb::global_control::max_allowed_parallelism, 1);

/**
 * @brief Global simulation variables
 */
extern double g_sim_time;
extern bool g_simulating;
extern std::map<int, Marker> g_markers;
extern bool g_show_trajectories;
extern bool g_show_stats;

std::vector<std::vector<vec2d>> g_env;  ///< Environment boundaries
std::vector<std::shared_ptr<Field>> g_fields;       ///< Force fields affecting particles
std::vector<Particle> g_particles;  ///< Simulated particles
std::vector<float> g_t_array, g_energy_array; ///< Energy tracking arrays

std::vector<Particle> particles;
std::vector<int> idx_bases;

double g_max_particle_radius = 0.00001;
int g_frame_count = 0;

// Speed distribution statistics global variables
std::vector<int> g_speed_hist;
const int SPEED_BINS = 200;
double g_max_speed = 20.0; // Dynamic maximum speed

std::chrono::time_point<std::chrono::high_resolution_clock> tt0 = std::chrono::high_resolution_clock::now();
auto tt1 = tt0;
auto tt2 = tt1;
auto tt3 = tt2;

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
	return 9.8 * (-pos.y - 600.0);  // Gravitational potential energy relative to y=-600
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

	g_frame_count = 0;
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
void step_one_particle(double timestep, int method_idx, Particle& p) {
	p.accel = vec2d(0.0, 0.0);
	for (auto& field : g_fields)
		p.accel += field->compute_accel(p.pos, p.vel);

	if (method_idx == 0) { 	// Forward Euler method (explicit)
		p.pos += timestep * p.vel;  // Update position
		p.vel += timestep * p.accel;  // Update velocity
	}
	else if (method_idx == 1) { 	// Backward Euler method (implicit)
		// Initial prediction
		p.pos_predicted = p.pos + timestep * p.vel;
		p.vel_predicted = p.vel + timestep * p.accel;

		// Newton-Raphson iteration to solve implicit equations
		for (int i = 0; i < 10; ++i) {
			// Calculate accelerations at predicted state
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
			}

			// Refine predictions
			for (auto& p : g_particles) {
				p.pos_predicted = p.pos + timestep * p.vel_predicted;
				p.vel_predicted = p.vel + timestep * p.accel_predicted;
			}
		}

		// Apply the final predictions
		p.pos += timestep * p.vel_predicted;
		p.vel += timestep * p.accel_predicted;
	}
	else if (method_idx == 2) {	// Implicit Trapezoid method
		// Initial prediction
		p.pos_predicted = p.pos + timestep * p.vel;
		p.vel_predicted = p.vel + timestep * p.accel;

		// Iterative solution for implicit equations
		for (int i = 0; i < 10; ++i) {
			// Calculate accelerations at predicted state
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
			}

			// Refine predictions
			for (auto& p : g_particles) {
				p.pos_predicted = p.pos + timestep * p.vel_predicted;
				p.vel_predicted = p.vel + timestep * p.accel_predicted;
			}
		}

		// Apply trapezoidal update using average of current and predicted
		p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
		p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
	}
	else if (method_idx == 3) {	// Explicit Trapezoid method
		// Predict new state using Euler
		p.pos_predicted = p.pos + timestep * p.vel;
		p.vel_predicted = p.vel + timestep * p.accel;

		// Calculate acceleration at predicted state
		p.accel_predicted = vec2d(0.0, 0.0);
		for (auto& field : g_fields) {
			p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
		}

		// Apply trapezoidal update (no iteration)
		p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
		p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
	}
	else if (method_idx == 4) {	// Taylor series method (2nd order)
		// Update using Taylor expansion to 2nd order
		p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
		p.vel += timestep * p.accel;
	}
	else if (method_idx == 5) {	// Combined Taylor + Explicit Trapezoid method
		// Initial prediction using Taylor method
		p.pos_predicted = p.pos + timestep * (p.vel + 0.5 * timestep * p.accel);
		p.vel_predicted = p.vel + timestep * p.accel;

		// Calculate acceleration at predicted state
		p.accel_predicted = vec2d(0.0, 0.0);
		for (auto& field : g_fields) {
			p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
		}

		// Apply combined update
		p.pos += timestep * (p.vel + 0.25 * timestep * (p.accel + p.accel_predicted));
		p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
	}
	else if (method_idx == 6) {	// Velocity Verlet method
		// Update position and predict velocity
		p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
		p.vel_predicted = p.vel + timestep * p.accel;

		// Calculate new acceleration at updated position
		p.accel_predicted = vec2d(0.0, 0.0);
		for (auto& field : g_fields) {
			p.accel_predicted += field->compute_accel(p.pos, p.vel_predicted);
		}

		// Correct velocity using average acceleration
		p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
	}
}

void run_one_simulation_step(double timestep, int method_idx) {
	if (!g_problem)
		return;

	int num_threads = std::max(4U, std::thread::hardware_concurrency());
	g_problem->num_threads = num_threads;
	
	auto t0 = std::chrono::high_resolution_clock::now();

	tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(g_particles.size())),
		[&, timestep, method_idx](const tbb::blocked_range<int>& r) {
			for (int i = r.begin(); i < r.end(); ++i) {
				auto& p = g_particles[i];
				step_one_particle(timestep, method_idx, p);
				if (g_show_trajectories || p.show_trajectory)
					p.traj.emplace_back(g_sim_time, p.pos);
			}
		}
	);

	auto t1 = std::chrono::high_resolution_clock::now();

	g_problem->handle_collision();

	auto t2 = std::chrono::high_resolution_clock::now();

	g_problem->handle_boundary();

	auto t3 = std::chrono::high_resolution_clock::now();

	struct EnergyAccumulator {
		double energy;
		bool has_nan;
		int nan_particle_id;

		EnergyAccumulator() : energy(0.0), has_nan(false), nan_particle_id(-1) {}
		EnergyAccumulator(EnergyAccumulator& a, tbb::split) : energy(0.0), has_nan(false), nan_particle_id(-1) {}
		void join(const EnergyAccumulator& rhs) {
			energy += rhs.energy;
			if (rhs.has_nan) {
				has_nan = true;
				nan_particle_id = rhs.nan_particle_id;
			}
		}
	};

	EnergyAccumulator result = tbb::parallel_reduce(
		tbb::blocked_range<int>(0, static_cast<int>(g_particles.size())),
		EnergyAccumulator(),
		[&](const tbb::blocked_range<int>& r, EnergyAccumulator local_acc) {
			for (int i = r.begin(); i < r.end(); ++i) {
				auto& p = g_particles[i];
				if (p.solution) {
					p.solution(g_sim_time + timestep, &p.pos, &p.vel, &p.accel);
				}
				else {
					local_acc.energy += 0.5 * p.mass * glm::dot(p.vel, p.vel);
					for (auto& field : g_fields) {
						local_acc.energy += field->compute_potential(p.pos) * p.mass;
					}
				}
				if (isnan(local_acc.energy) && !local_acc.has_nan) {
					local_acc.has_nan = true;
					local_acc.nan_particle_id = p.id;
				}
			}
			return local_acc;
		},
		[](EnergyAccumulator a, EnergyAccumulator b) {
			a.join(b);
			return a;
		}
	);

	double E = result.energy;
	if (result.has_nan)
		printf("Found NaN for the particle %d\n", result.nan_particle_id);

	auto t4 = std::chrono::high_resolution_clock::now();

	if (isnan(E)) {
		g_simulating = false;
	} else {
		g_sim_time += timestep;
		g_t_array.push_back(g_sim_time);
		g_energy_array.push_back(E);

		if (g_show_stats) {
			double max_speed = tbb::parallel_reduce(tbb::blocked_range<int>(0, static_cast<int>(g_particles.size())), 0.0,
				[&](const tbb::blocked_range<int>& r, double local_max) {
					for (int i = r.begin(); i < r.end(); ++i)
						local_max = std::max(local_max, glm::length(g_particles[i].vel));
					return local_max;
				},
				[](double a, double b) { return std::max(a, b); }
			);

			// Add a small buffer (20%) to ensure all particles are visible
			g_max_speed = max_speed * 1.2;
			// Ensure minimum value to avoid empty plots
			g_max_speed = std::max(0.1, g_max_speed);

			g_speed_hist.assign(SPEED_BINS, 0);
			std::mutex hist_mutex;
			tbb::parallel_for(tbb::blocked_range<size_t>(0, g_particles.size()),
				[&](const tbb::blocked_range<size_t>& r) {
					std::vector<int> local_hist(SPEED_BINS, 0);
					for (size_t i = r.begin(); i != r.end(); ++i) {
						const auto& p = g_particles[i];
						double speed = glm::length(p.vel);
						int bin = static_cast<int>((speed / g_max_speed) * SPEED_BINS);
						bin = std::max(0, std::min(bin, SPEED_BINS - 1));
						local_hist[bin]++;
					}

					std::lock_guard<std::mutex> lock(hist_mutex);
					for (int i = 0; i < SPEED_BINS; ++i)
						g_speed_hist[i] += local_hist[i];
				}
			);
		}
	}

	auto t5 = std::chrono::high_resolution_clock::now();
	double time_integrator = std::chrono::duration<double, std::milli>(t1 - t0).count();
	double time_collision = std::chrono::duration<double, std::milli>(t2 - t1).count();
	double time_boundary = std::chrono::duration<double, std::milli>(t3 - t2).count();
	double time_energy = std::chrono::duration<double, std::milli>(t4 - t3).count();
	double time_hist = std::chrono::duration<double, std::milli>(t5 - t4).count();
	double time_build_grid = std::chrono::duration<double, std::milli>(tt1 - tt0).count();
	double time_pos_filter = std::chrono::duration<double, std::milli>(tt2 - tt1).count();
	double time_collision_pair = std::chrono::duration<double, std::milli>(tt3 - tt2).count();

	printf("integrator: %.2f, collision: %.2f(init: %.1f, filter: %.1f, resolve: %.1f), boundary: %.2f, energy: %.2f, hist: %.2f\n",
		time_integrator, time_collision, time_build_grid, time_pos_filter, time_collision_pair, time_boundary, time_energy, time_hist);

	g_frame_count++;
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

void Problem::init(std::map<int, Marker>* markers) {
	tt0 = std::chrono::high_resolution_clock::now();
	tt1 = tt0;
	tt2 = tt0;
	tt3 = tt0;

	g_fields.clear();
	g_env.clear();
	g_max_particle_radius = 0.00001;
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
	Problem::init(markers);

	// Create a central gravitational field at (800, -600)
	g_fields.push_back(std::make_shared<GravityInSpace>(vec2d(800.0, -600.0)));
	
	// Create a marker for the planet
	Marker m1;
	m1.x = 700.0f;    // Position 100 units left of center
	m1.y = -600.0f;   // Same y-coordinate as center
	m1.vy = -30.0f;   // Initial velocity to create circular orbit

	//Initialize environment
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
	Problem::init(markers);

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
	Problem::init(markers);

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

	num_of_particles = 1000000;
	double particle_radius = 0.03;

	g_fields.push_back(std::make_shared<GravityOnEarth>());

	// Define a rectangular container boundary
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
	std::uniform_real_distribution<double> y_dist(center_y + 150, container_max_y - 0.1);
	std::uniform_real_distribution<double> vel_dist(-50.0, 50.0);

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
			g_particles[idx].set(g_sim_time, idx, /*color_idx=*/0, particle_radius, /*mass=*/particle_radius * particle_radius * glm::pi<double>(), vec2d(x,  y), vec2d(vx, vy), vec2d(0.0, 0.0));
		else
			g_particles[idx].set(g_sim_time, idx, /*color_idx=*/2, particle_radius, /*mass=*/particle_radius * particle_radius * glm::pi<double>(), vec2d(x2, y), vec2d(vx, vy), vec2d(0.0, 0.0));
	}

	num_of_big_particles = 4;
	g_particles.back().show_trajectory = true;
	Particle p;
	p.set(g_sim_time, g_particles.size(), /*color_idx=*/1, particle_radius, /*mass=*/p.radius * p.radius * glm::pi<double>(), vec2d(center_x, center_y), vec2d(0.0, 0.0), vec2d(0.0, 0.0));
	p.is_vip = true;
	p.radius = 20.0;
	p.show_trajectory = true;

	double air_ratio = 0.1;

	p.pos.x = center_x - 150.0;
	p.pos.y = center_y + 50.0;
	p.mass = p.radius * p.radius * glm::pi<double>() * air_ratio * 0.01;
	g_particles.push_back(p);

	p.pos.x = center_x - 70.0;
	p.pos.y = center_y + 50.0;
	p.mass = p.radius * p.radius * glm::pi<double>() * air_ratio * 0.1;
	g_particles.push_back(p);

	p.pos.x = center_x + 70.0;
	p.pos.y = center_y + 50.0;
	p.mass = p.radius * p.radius * glm::pi<double>() * air_ratio * 0.5;
	g_particles.push_back(p);

	p.pos.x = center_x + 150.0;
	p.pos.y = center_y + 50.0;
	p.mass = p.radius * p.radius * glm::pi<double>() * air_ratio * 1.0;
	g_particles.push_back(p);
}

/**
 * @brief Handles particle collisions with container boundaries
 * 
 * Implements perfect elastic reflections when particles hit the container walls.
 * Reverses the appropriate velocity component when a particle is heading toward a boundary.
 */
void RarefiedGas::handle_boundary() {
	tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(g_particles.size())),
		[this](const tbb::blocked_range<int>& r) {
			for (int i = r.begin(); i < r.end(); ++i) {
				auto& p = g_particles[i];
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
	);
}

inline int next_power_of_two(int x, int* bits) {
	int xx = x - 1;
	*bits = 0;
	int v = 1;
	while (xx > 0) {
		(*bits)++;
		v <<= 1;
		xx >>= 1;
	}
	return v;
}

inline void z_order_to_2d(int z, int m_n, int* x, int* y) {
	*x = 0;
	*y = 0;

	for (int k = 0; k < m_n; ++k) {
		*x |= ((z >> (2 * k)) & 1) << k;
		*y |= ((z >> (2 * k + 1)) & 1) << k;
	}
}

/**
	* @brief Handles collisions between gas particles
	* 
	* Implements elastic collisions between particles, ensuring momentum and kinetic energy
	* are conserved during interactions.
	*/
void RarefiedGas::handle_collision() {
	// Space partitioning optimization using grid system
	double container_width = container_max_x - container_min_x;
	double container_height = container_max_y - container_min_y;
	// Adaptive grid size: ensures grid is at least particle diameter and constains at least one particle when distributes particles evenly
	double grid_size = std::max(g_max_particle_radius * 2, 0.5 * std::sqrt(container_width * container_height / num_of_particles));
	int grid_x_count = static_cast<int>(container_width / grid_size);
	int grid_y_count = static_cast<int>(container_height / grid_size);
	int grid_count = grid_x_count * grid_x_count;
	//printf("grid_size: %f, max_grid_x: %f, grid_count: %d\n", grid_size, container_width / grid_size, grid_count);

	// Function to get grid key from particle position
	auto get_grid_xy = [&](double x, double y, int* grid_x, int* grid_y) -> void {
		// Convert world coordinates to grid coordinates
		*grid_x = static_cast<int>((x - this->container_min_x) / grid_size);
		*grid_y = static_cast<int>((y - this->container_min_y) / grid_size);
		// Ensure grid coordinates are within bounds
		*grid_x = std::max(0, std::min(*grid_x, grid_x_count - 1));
		*grid_y = std::max(0, std::min(*grid_y, grid_y_count - 1));
	};

	tt0 = std::chrono::high_resolution_clock::now();

	// Create grid: map from grid coordinates to list of particle indices
	grid.resize(grid_count);
	tbb::parallel_for(tbb::blocked_range<int>(0, grid_count),
		[this, grid_count](const tbb::blocked_range<int>& r) {
		for (int i = r.begin(); i < r.end(); ++i) {
			auto& cell = grid[i];
			cell.clear();
			cell.reserve(num_of_particles / grid_count);
		}
	}
	);

	// Populate the grid with particle indices
	std::vector<std::mutex> m(num_threads * 30);
	tbb::parallel_for(tbb::blocked_range<int>(0, (int)g_particles.size()),
		[this, &get_grid_xy, grid_x_count, grid_y_count, grid_size, &m](const tbb::blocked_range<int>& r) {
		for (int i = r.begin(); i < r.end(); ++i) {
			auto& p = g_particles[i];
			p.is_colliding = false;
			get_grid_xy(p.pos.x, p.pos.y, &p.grid_x, &p.grid_y);
			p.grid_xy = p.grid_y * grid_x_count + p.grid_x;
			if (p.radius <= g_max_particle_radius) {
				std::lock_guard<std::mutex> lock(m[p.grid_xy % m.size()]);
				auto& cell = grid[p.grid_xy];
				auto it = std::lower_bound(cell.begin(), cell.end(), i,
					[](int idx_a, int idx_b) {return std::abs(idx_a) > std::abs(idx_b); });
				cell.insert(it, i);
			}
		}
	});

#if 1
	// Init Z-order curve indices
	if (z_order_curve_xy.empty()) {
		int x_bits, y_bits;
		int grid_x_count2 = next_power_of_two(grid_x_count, &x_bits);
		int grid_y_count2 = next_power_of_two(grid_y_count, &y_bits);
		int m = std::max(x_bits, y_bits);
		for (int z = 0; z < grid_x_count2*grid_y_count2; ++z) {
			int xx, yy;
			z_order_to_2d(z, m, &xx, &yy);
			if (xx < grid_x_count && yy < grid_y_count)
				z_order_curve_xy.push_back(yy * grid_x_count + xx);
		}
	}

	// Reorder small particles every 10 frames
	if (g_frame_count % 10 == 0) {

		auto tttt0 = std::chrono::high_resolution_clock::now();

		// Copy big particles without reordering.
		int new_idx = static_cast<int>(g_particles.size() - 1) - num_of_big_particles;
		particles.resize(g_particles.size());
		for (int i = 0; i < num_of_big_particles; ++i)
			particles[g_particles.size() - 1 - i] = g_particles[g_particles.size() - 1 - i];

		auto tttt1 = std::chrono::high_resolution_clock::now();

		// Calculate index base for each cell
		idx_bases.clear();
		idx_bases.reserve(grid.size());
		for (int grid_xy : z_order_curve_xy) {
			idx_bases.push_back(new_idx);
			new_idx -= grid[grid_xy].size();
		}

		auto tttt2 = std::chrono::high_resolution_clock::now();

		// Reorder small particles with multi-threading
		tbb::parallel_for(tbb::blocked_range<int>(0, (int)z_order_curve_xy.size()),
			[this](const tbb::blocked_range<int>& r) {
			for (int i = r.begin(); i < r.end(); ++i) {
				int grid_xy = z_order_curve_xy[i];
				int new_idx = idx_bases[i];
				for (auto& idx : grid[grid_xy]) {
					particles[new_idx] = g_particles[idx];
					idx = new_idx--;
				}
			}
		});

		g_particles.swap(particles);

		auto tttt3 = std::chrono::high_resolution_clock::now();
		double time_copy_big_particle = std::chrono::duration<double, std::milli>(tttt1 - tttt0).count();
		double time_base_idx = std::chrono::duration<double, std::milli>(tttt2 - tttt1).count();
		double time_copy = std::chrono::duration<double, std::milli>(tttt3 - tttt2).count();
		printf("!!!! \t\ttime_copy_big_particle: %.2f, time_base_idx: %.2f, time_copy: %.2f\n", time_copy_big_particle, time_base_idx, time_copy);
	}
#endif

#if 0
	// Reorder small particles every 5 frames
	if (g_frame_count % 5 == 3) {

		auto tttt0 = std::chrono::high_resolution_clock::now();

		// Copy big particles without reordering.
		int new_idx = static_cast<int>(g_particles.size() - 1) - num_of_big_particles;
		particles.resize(g_particles.size());
		for (int i = 0; i < num_of_big_particles; ++i)
			particles[g_particles.size() - 1 - i] = g_particles[g_particles.size() - 1 - i];

		auto tttt1 = std::chrono::high_resolution_clock::now();

		// Calculate index base for each cell
		idx_bases.clear();
		idx_bases.reserve(grid.size());
		for (size_t i = 0; i < grid.size(); ++i) {
			idx_bases.push_back(new_idx);
			new_idx -= grid[i].size();
		}

		auto tttt2 = std::chrono::high_resolution_clock::now();

		// Reorder small particles with multi-threading
		tbb::parallel_for(tbb::blocked_range<int>(0, (int)grid.size()),
			[this](const tbb::blocked_range<int>& r) {
			for (int i = r.begin(); i < r.end(); ++i) {
				auto& cell = grid[i];
				int new_idx = idx_bases[i];
				for (auto& idx : cell) {
					particles[new_idx] = g_particles.at(idx);
					idx = new_idx;
					new_idx--;
				}
			}
		});
		g_particles.swap(particles);

		auto tttt3 = std::chrono::high_resolution_clock::now();
		double time_copy_big_particle = std::chrono::duration<double, std::milli>(tttt1 - tttt0).count();
		double time_base_idx = std::chrono::duration<double, std::milli>(tttt2 - tttt1).count();
		double time_copy = std::chrono::duration<double, std::milli>(tttt3 - tttt2).count();
		printf("!!!! \t\ttime_copy_big_particle: %.2f, time_base_idx: %.2f, time_copy: %.2f\n", time_copy_big_particle, time_base_idx, time_copy);
	}
#endif

	// Step 1: Concurrent filtering of potential collision pairs
	tbb::concurrent_vector<std::vector<CollisionPair>> tbb_local_pairs;

	tt1 = std::chrono::high_resolution_clock::now();

	tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(g_particles.size())), 
		[&](const tbb::blocked_range<int>& r) {

		//auto ttt1 = std::chrono::high_resolution_clock::now();

			// Each thread uses its own local storage to avoid contention
			std::vector<CollisionPair> local_pairs;
			//local_pairs.reserve(1000); // Pre-reserve space to reduce allocations

			for (int i = r.begin(); i != r.end(); ++i) {
				auto& p = g_particles[i];
				
				// Check current grid cell and all 8 neighboring cells
				for (int dy = -1; dy <= 1; ++dy) {
					for (int dx = -1; dx <= 1; ++dx) {
						// Calculate neighboring grid cell coordinates
						int grid_x = p.grid_x + dx;
						int grid_y = p.grid_y + dy;

						// Check if the neighboring grid cell is within bounds
						if (grid_x < 0 || grid_x >= grid_x_count ||
							grid_y < 0 || grid_y >= grid_y_count)
							continue;
						
						// Get key for neighboring grid cell
						int neighbor_key = grid_y * grid_x_count + grid_x;
						
						// Check collisions with all particles in the neighboring grid cell
						for (int j : grid[neighbor_key]) {
							// Avoid checking the same pair twice (i < j)
							if (i >= j)
								break;

							// Check collision between particles i and j
							vec2d diff = g_particles[i].pos - g_particles[j].pos;
							double dist2 = glm::dot(diff, diff);
							double R = g_particles[i].radius + g_particles[j].radius;
							
							// Only save particle pairs that might collide (distance less than sum of radii)
							if (dist2 <= R * R)
								local_pairs.push_back({ i, j, dist2, diff });
						}
					}
				}
			}
			
			if (!local_pairs.empty())
				tbb_local_pairs.emplace_back(local_pairs);

			//auto ttt2 = std::chrono::high_resolution_clock::now();
			//double time = std::chrono::duration<double, std::milli>(tt3 - tt2).count();
			//printf("\t%.1f\n", time);
		}
	); 

	// big vs grid index
	std::vector<std::pair<int, int>> particle_grid_pairs;
	for (int i = g_particles.size() - num_of_big_particles; i < g_particles.size(); ++i) {
		auto& p_big = g_particles[i];
		int k = std::ceil(p_big.radius / grid_size);
		for (int dy = -k; dy <= k; ++dy) {
			for (int dx = -k; dx <= k; ++dx) {
				int grid_x = p_big.grid_x + dx;
				int grid_y = p_big.grid_y + dy;
				if (grid_x >= 0 && grid_x < grid_x_count && grid_y >= 0 && grid_y < grid_y_count) {
					int grid_xy = grid_y * grid_x_count + grid_x;
					particle_grid_pairs.emplace_back(i, grid_xy);
				}
			}
		}
	}

	// big vs grid in parallel
	tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(particle_grid_pairs.size())),
		[&](const tbb::blocked_range<int>& r) {

		//auto ttt1 = std::chrono::high_resolution_clock::now();

			// Each thread uses its own local storage to avoid contention
		std::vector<CollisionPair> local_pairs;
		//local_pairs.reserve(1000); // Pre-reserve space to reduce allocations

		for (int pair_idx = r.begin(); pair_idx != r.end(); ++pair_idx) {
			int i = particle_grid_pairs[pair_idx].first;
			int g_idx = particle_grid_pairs[pair_idx].second;
			// Check collisions with all particles in the neighboring grid cell
			for (int j : grid[g_idx]) {
				// Check collision between particles i and j
				vec2d diff = g_particles[i].pos - g_particles[j].pos;
				double dist2 = glm::dot(diff, diff);
				double R = g_particles[i].radius + g_particles[j].radius;

				// Only save particle pairs that might collide (distance less than sum of radii)
				if (dist2 <= R * R)
					local_pairs.push_back({ i, j, dist2, diff });
			}
		}
		if (!local_pairs.empty())
			tbb_local_pairs.emplace_back(local_pairs);

		//auto ttt2 = std::chrono::high_resolution_clock::now();
		//double time = std::chrono::duration<double, std::milli>(tt3 - tt2).count();
		//printf("\t%.1f\n", time);
	}
	);

	// big vs big
	if (tbb_local_pairs.empty())
		tbb_local_pairs.emplace_back();
	for (int i = g_particles.size() - num_of_big_particles; i < g_particles.size(); ++i) {
		auto& p_big_a = g_particles[i];
		for (int j = i + 1; j < g_particles.size(); ++j) {
			vec2d diff = g_particles[i].pos - g_particles[j].pos;
			double dist2 = glm::dot(diff, diff);
			double R = g_particles[i].radius + g_particles[j].radius;

			// Only save particle pairs that might collide (distance less than sum of radii)
			if (dist2 <= R * R)
				tbb_local_pairs.back().push_back({ i, j, dist2, diff });
		}
	}


	tt2 = std::chrono::high_resolution_clock::now();

	// Step 2: Sequential processing of all potential collision pairs
	int total_pairs = 0;
	int actual_collisions = 0;
	std::set<std::pair<int, int>> shadowed_pairs;
	// Iterate through collision pairs collected by all threads
	for (auto& pair_block : tbb_local_pairs) {
		// Process each potential collision pair collected by the current thread
		for (const auto& pair : pair_block) {
			int i = pair.i;
			int j = pair.j;

			total_pairs++;
			
			// Avoid numerical instability caused by division by zero or very small values
			const double min_dist2 = 1e-12;
			if (pair.dist2 < min_dist2)
				continue;
			
			// Calculate relative velocity (using latest particle states)
			vec2d rel_vel = g_particles[j].vel - g_particles[i].vel;
			
			// Calculate normal relative velocity component
			double rel_vn = glm::dot(rel_vel, pair.diff);
			
			// Only process particles that are approaching each other
			if (rel_vn < 0.0)
				continue;
			
			// Set collision flags
			g_particles[i].is_colliding = true;
			g_particles[j].is_colliding = true;
			
			// Get particle masses
			double m1 = g_particles[i].mass;
			double m2 = g_particles[j].mass;
			 
			// Calculate impulse scalar for elastic collision
			double impulse = (2.0 * m1 * m2 * rel_vn) / (m1 + m2);
				
			// Calculate velocity changes
			vec2d delta_v_i = (impulse / m1) * pair.diff / pair.dist2;
			vec2d delta_v_j = -(impulse / m2) * pair.diff / pair.dist2;
			
			// Update velocities (sequential processing, no locks needed)
			g_particles[i].vel += delta_v_i;
			g_particles[j].vel += delta_v_j;
			
			actual_collisions++;
		}
	}

	tt3 = std::chrono::high_resolution_clock::now();
	printf("\tpairs: %d/%d = %.1f%%\n", actual_collisions, total_pairs, (double)actual_collisions / total_pairs * 100.0);
}

