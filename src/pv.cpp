#include "pv.h"

#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

/* Physics Vector */


extern double g_sim_time;

std::vector<Field*> g_fields;
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

vec2d GravityOnEarth::compute_accel(const vec2d& pos) {
	return vec2d(0, 9.8);
}

double GravityOnEarth::compute_potential(const vec2d& pos) {
	return 0;
}

vec2d GravityInSpace::compute_accel(const vec2d& pos) {
	vec2d v = center - pos;
	double r2 = glm::dot(v, v);
	return 200*30*30/(r2*std::sqrt(r2))*v;
}

double GravityInSpace::compute_potential(const vec2d& pos) {
	return 0;
}

void start_simulation(std::map<int, Marker>& markers) {
	for (auto* field : g_fields)
		delete field;
	g_fields.clear();
	//g_fields.push_back(new GravityOnEarth);
	g_fields.push_back(new GravityInSpace(vec2d(800.0, -600.0)));

	g_particles.resize(markers.size());
	int count = 0;
	for (auto&[idx, m] : markers) {
		g_particles[count++].set(g_sim_time, idx, 1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}
}

void compute_accel() {
	for (auto& field : g_fields) {
		for (auto& p : g_particles) {
			p.accel = field->compute_accel(p.pos);
		}
	}
}

void run_one_simulation_step(double timestep, int method_idx) {
	if (method_idx == 0) {	// Eular
		//printf("Eular \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos += timestep * p.vel;
			p.vel += timestep * p.accel;
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	} else if (method_idx == 1) {	// Explicit Trapezoid
		//printf("Explicit Trapezoid \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (auto& field : g_fields) {
			for (auto& p : g_particles) {
				p.accel_predicted = field->compute_accel(p.pos_predicted);
			}
		}
		for (auto& p : g_particles) {
			p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	} else if (method_idx == 2) {	// Taylor (2nd order)
		//printf("Taylor (2nd order) \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel += timestep * p.accel;
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	} else if (method_idx == 3) {	// Taylor (2nd order) + Explicit Trapezoid
		//printf("Taylor (2nd order) + Explicit Trapezoid \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * (p.vel + 0.5 * timestep * p.accel);
			//p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (auto& field : g_fields) {
			for (auto& p : g_particles) {
				p.accel_predicted = field->compute_accel(p.pos_predicted);
			}
		}
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.25 * timestep * (p.accel + p.accel_predicted));
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	}

	g_sim_time += timestep;
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
