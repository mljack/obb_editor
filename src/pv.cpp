#include "pv.h"

#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

/* Physics Vector */


extern double g_sim_time;

std::vector<Particle> g_particles;

// pi => glm::pi()
// angle() => glm::orientedAngle(), glm::angle()
// distance_v() => glm::distance()
// dot()
// cross()
// glm::length()

vec2d accel(const vec2d& pt, const vec2d& v) {
	return vec2d(0, 9.8);
	//return vec2d(0, 9.8) + v * 0.1;
}

void start_simulation(std::map<int, Marker>& markers) {
	g_particles.resize(markers.size());
	int count = 0;
	for (auto&[idx, m] : markers) {
		g_particles[count++].set(g_sim_time, idx, 1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}
}

void run_one_simulation_step(double timestep) {
	g_sim_time += timestep;
	for (auto& p : g_particles) {
		p.accel = accel(p.pos, p.vel);
		p.pos += timestep * (p.vel + 0.5 * p.accel);
		p.vel += timestep * p.accel;
		p.traj.emplace_back(g_sim_time, p.pos);
	}
}

void stop_simulation(std::map<int, Marker>* markers) {
	for (auto& p : g_particles) {
		auto& m = markers->at(p.id);
		m.x = p.pos.x; m.y = p.pos.y;
		m.vx = p.vel.x; m.vy = p.vel.y;
		m.ax = p.accel.x; m.ay = p.accel.y;
	}
}

void seek_to_sim_time_moment(double t, std::map<int, Marker>* markers) {
	t *= 0.9999;
	int count = 0;
	for (auto&[idx, m] : *markers) {
		if (count >= g_particles.size())
			continue;
		for (auto& traj_pt : g_particles[count++].traj) {
			if (traj_pt.t > t) {
				m.x = traj_pt.pos.x;
				m.y = traj_pt.pos.y;
				break;
			}
		}
	}
	g_sim_time = t;
}
