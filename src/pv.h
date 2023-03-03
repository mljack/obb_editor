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
	std::vector<TrajPt> traj;
};

extern std::vector<Particle> g_particles;

void start_simulation(std::map<int, Marker>& markers);
void run_one_simulation_step(double timestep);
void stop_simulation(std::map<int, Marker>* markers);
void seek_to_sim_time_moment(double t, std::map<int, Marker>* markers);
