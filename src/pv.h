#pragma once

#include <map>
#include <vector>
#include <set>
#include <functional>

#include <glm/vec2.hpp>
using vec2d = glm::dvec2;
using vec2f = glm::vec2;

#include "marker.h"

struct TrajPt {
	TrajPt(double t1, const vec2d& pos1) {
		t = t1; pos = pos1;
	}
	double t;
	vec2d pos;
};
struct Particle {
	Particle() {}
	void set(double t, int id1, double radius1, double mass1, const vec2d& pos1, const vec2d& vel1, const vec2d& accel1) {
		id = id1; radius = radius1;  mass = mass1;  pos = pos1;
		vel = vel1; accel = accel1;
	}
	void set_solution(std::function<void(double, vec2d*, vec2d*, vec2d*)> s) {
		solution = s;
	}
	int id;
	double radius;
	double mass;
	vec2d pos;
	vec2d vel;
	vec2d accel;
	vec2d pos_predicted;
	vec2d vel_predicted;
	vec2d accel_predicted;
	bool is_colliding = false;
	std::vector<TrajPt> traj;
	std::function<void(double, vec2d*, vec2d*, vec2d*)> solution = nullptr;
};

class Field {
public:
	Field() {}
	virtual ~Field() {}
	virtual vec2d compute_accel(const vec2d& pos, const vec2d& vel) = 0;
	virtual double compute_potential(const vec2d& pos) = 0;
};

class GravityOnEarth : public Field {
public:
	vec2d compute_accel(const vec2d& pos, const vec2d& vel) override;
	double compute_potential(const vec2d& pos) override;
};

class GravityInSpace : public Field {
public:
	GravityInSpace(const vec2d& c) { center = c; }
	vec2d compute_accel(const vec2d& pos, const vec2d& vel) override;
	double compute_potential(const vec2d& pos) override;
	vec2d center;
};

class AirResistance : public Field {
public:
	vec2d compute_accel(const vec2d& pos, const vec2d& vel) override;
	double compute_potential(const vec2d& pos) override { return 0.0; };
};

extern std::vector<Particle> g_particles;
extern std::vector<std::vector<vec2d>> g_env;
extern std::vector<float> g_t_array, g_energy_array;
extern std::vector<int> g_speed_hist;
extern const int SPEED_BINS;
extern double g_max_speed;

/**
 * @brief Calculate and update the maximum speed of all particles
 */
void update_max_speed();

class Problem {
public:
	virtual ~Problem() {}
	virtual void init(std::map<int, Marker>* markers) = 0;
	virtual void handle_boundary() {};
	virtual void handle_collision() {};
	virtual void destory() {};
};

class PlanetOrbit : public Problem {
public:
	void init(std::map<int, Marker>* markers) override;
};

class BadmintonClearShot : public Problem {
public:
	void init(std::map<int, Marker>* markers) override;
};

class RarefiedGas : public Problem {
public:
	void init(std::map<int, Marker>* markers) override;
	void handle_boundary() override;
	void handle_collision() override;
private:
	// Container boundary variables
	int num_of_particles;
	double particle_radius;
	double container_min_x;
	double container_min_y;
	double container_max_x;
	double container_max_y;
};

extern std::unique_ptr<Problem> g_problem;

void start_simulation(int problem_idx, std::map<int, Marker>* markers);
void run_one_simulation_step(double timestep, int method_idx);
void stop_simulation(std::map<int, Marker>* markers);
void seek_to_sim_time_moment(double t, std::map<int, Marker>* markers);
