#pragma once

#include <map>
#include <vector>

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
	void set(double t, int id1, double mass1, const vec2d& pos1, const vec2d& vel1, const vec2d& accel1) {
		id = id1; mass = mass1;  pos = pos1;
		vel = vel1; accel = accel1;
		traj.emplace_back(t, pos);
	}
	int id;
	double mass;
	vec2d pos;
	vec2d vel;
	vec2d accel;
	vec2d pos_predicted;
	vec2d vel_predicted;
	vec2d accel_predicted;
	std::vector<TrajPt> traj;
};

class Field {
public:
	Field() {}
	virtual ~Field() {}
	virtual vec2d compute_accel(const vec2d& pos) = 0;
	virtual double compute_potential(const vec2d& pos) = 0;
};

class GravityOnEarth : public Field {
public:
	vec2d compute_accel(const vec2d& pos) override;
	double compute_potential(const vec2d& pos) override;
};

class GravityInSpace : public Field {
public:
	GravityInSpace(const vec2d& c) { center = c; }
	vec2d compute_accel(const vec2d& pos) override;
	double compute_potential(const vec2d& pos) override;
	vec2d center;
};

extern std::vector<Particle> g_particles;
extern std::vector<float> g_t_array, g_energy_array;

void start_simulation(std::map<int, Marker>& markers);
void run_one_simulation_step(double timestep, int method_idx);
void stop_simulation(std::map<int, Marker>* markers);
void seek_to_sim_time_moment(double t, std::map<int, Marker>* markers);
