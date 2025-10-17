#include "pv.h"

#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

/* Physics Vector */


extern double g_sim_time;
extern bool g_simulating;

std::vector<Field*> g_fields;
std::vector<Particle> g_particles;
std::vector<float> g_t_array, g_energy_array;

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
	return 9.8 * (pos.y - 0.0);
}

vec2d GravityInSpace::compute_accel(const vec2d& pos) {
	vec2d r = center - pos;
	double r2 = glm::dot(r, r);
	return 200*30*30/(r2*std::sqrt(r2))*r;
}

double GravityInSpace::compute_potential(const vec2d& pos) {
	double r = glm::length(center - pos);
	return -200 * 30 * 30 / r;
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

	if (!g_particles.empty()) {
		Particle solution = g_particles[0];
		solution.traj.clear();
		solution.set_solution([](double t, vec2d* pos, vec2d* vel, vec2d* accel) {
			double r = 100.0;
			*pos = vec2d(800.0, -600.0) + r * vec2d(std::cos(t), std::sin(t));
		});
		g_particles.push_back(solution);
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
	} else if (method_idx == 1) {	// Backward Eular
		//printf("Backward Eular \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (int i = 0; i < 10; ++i) {
			for (auto& field : g_fields) {
				for (auto& p : g_particles) {
					p.accel_predicted = field->compute_accel(p.pos_predicted);
				}
			}
			for (auto& p : g_particles) {
				p.pos_predicted = p.pos + timestep * p.vel_predicted;
				p.vel_predicted = p.vel + timestep * p.accel_predicted;
			}
		}
		for (auto& p : g_particles) {
			p.pos += timestep * p.vel_predicted;
			p.vel += timestep * p.accel_predicted;
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	}
	else if (method_idx == 2) {	// Implicit Trapezoid
		//printf("Implicit Trapezoid \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (int i = 0; i < 10; ++i) {
			for (auto& field : g_fields) {
				for (auto& p : g_particles) {
					p.accel_predicted = field->compute_accel(p.pos_predicted);
				}
			}
			for (auto& p : g_particles) {
				p.pos_predicted = p.pos + timestep * p.vel_predicted;
				p.vel_predicted = p.vel + timestep * p.accel_predicted;
			}
		}
		for (auto& p : g_particles) {
			p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	} else if (method_idx == 3) {	// Explicit Trapezoid
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
	} else if (method_idx == 4) {	// Taylor (2nd order)
		//printf("Taylor (2nd order) \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel += timestep * p.accel;
			p.traj.emplace_back(g_sim_time, p.pos);
		}
	} else if (method_idx == 5) {	// Taylor (2nd order) + Explicit Trapezoid
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
	} else if (method_idx == 6) {	// Velocity Verlet
		//printf("Velocity Verlet \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			//p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (auto& field : g_fields) {
			for (auto& p : g_particles) {
				p.accel_predicted = field->compute_accel(p.pos);
			}
		}
		for (auto& p : g_particles) {
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
			p.traj.emplace_back(g_sim_time, p.pos);
		}
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
