#include "pv.h"

#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

#include <random>
#include <set>

/* Physics Vector */


extern double g_sim_time;
extern bool g_simulating;

extern std::map<int, Marker> g_markers;
extern bool g_show_trajectories;
std::vector<std::vector<vec2d>> g_env;
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

vec2d GravityOnEarth::compute_accel(const vec2d& pos, const vec2d& vel) {
	return vec2d(0, 9.8);
}

double GravityOnEarth::compute_potential(const vec2d& pos) {
	return 9.8 * (pos.y - 0.0);
}

vec2d GravityInSpace::compute_accel(const vec2d& pos, const vec2d& vel) {
	vec2d r = center - pos;
	double r2 = glm::dot(r, r);
	return 200*30*30/(r2*std::sqrt(r2))*r;
}

double GravityInSpace::compute_potential(const vec2d& pos) {
	double r = glm::length(center - pos);
	return -200 * 30 * 30 / r;
}

vec2d AirResistance::compute_accel(const vec2d& pos, const vec2d& vel) {
	double v2 = glm::dot(vel, vel);
	return -10.0 / std::sqrt(v2)*vel;
}


void start_simulation(int problem_idx, std::map<int, Marker>* markers) {
	if (problem_idx == 1)
		g_problem = std::make_unique<PlanetOrbit>();
	else if (problem_idx == 2)
		g_problem = std::make_unique<BadmintonClearShot>();
	else if (problem_idx == 3)
		g_problem = std::make_unique<RarefiedGas>();
	else
		g_problem = nullptr;

	g_problem->init(markers);

	if (g_show_trajectories) {
		for (auto& p : g_particles)
			p.traj.emplace_back(g_sim_time, p.pos);
	}
}

void compute_accel() {
	for (auto& p : g_particles) {
		p.accel = vec2d(0.0, 0.0);
		for (auto& field : g_fields) {
				p.accel += field->compute_accel(p.pos, p.vel);
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
		}
	} else if (method_idx == 1) {	// Backward Eular
		//printf("Backward Eular \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (int i = 0; i < 10; ++i) {
			for (auto& p : g_particles) {
				p.accel_predicted = vec2d(0.0, 0.0);
				for (auto& field : g_fields) {
					p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
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
			for (auto& p : g_particles) {
				p.accel_predicted = vec2d(0.0, 0.0);
				for (auto& field : g_fields) {
					p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
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
		}
	} else if (method_idx == 3) {	// Explicit Trapezoid
	 //printf("Explicit Trapezoid \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * p.vel;
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (auto& p : g_particles) {
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
			}
		}
		for (auto& p : g_particles) {
			p.pos += timestep * 0.5 * (p.vel + p.vel_predicted);
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	} else if (method_idx == 4) {	// Taylor (2nd order)
		//printf("Taylor (2nd order) \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel += timestep * p.accel;
		}
	} else if (method_idx == 5) {	// Taylor (2nd order) + Explicit Trapezoid
		//printf("Taylor (2nd order) + Explicit Trapezoid \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos_predicted = p.pos + timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (auto& p : g_particles) {
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos_predicted, p.vel_predicted);
			}
		}
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.25 * timestep * (p.accel + p.accel_predicted));
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	} else if (method_idx == 6) {	// Velocity Verlet
		//printf("Velocity Verlet \n");
		compute_accel();
		for (auto& p : g_particles) {
			p.pos += timestep * (p.vel + 0.5 * timestep * p.accel);
			p.vel_predicted = p.vel + timestep * p.accel;
		}
		for (auto& p : g_particles) {
			p.accel_predicted = vec2d(0.0, 0.0);
			for (auto& field : g_fields) {
				p.accel_predicted += field->compute_accel(p.pos, p.vel_predicted);
			}
		}
		for (auto& p : g_particles) {
			p.vel += timestep * 0.5 * (p.accel + p.accel_predicted);
		}
	}

	g_problem->handle_collision();
	g_problem->handle_boundary();

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

void PlanetOrbit::init(std::map<int, Marker>* markers) {
	g_fields.clear();
	//g_fields.push_back(new GravityOnEarth);
	g_fields.push_back(new GravityInSpace(vec2d(800.0, -600.0)));
	
	Marker m1;
	m1.x = 700.0f;
	m1.y = -600.0f;
	m1.vy = -30.0f;

	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(800.0, -600.0));

	markers->clear();
	markers->emplace(0, m1);
	g_particles.resize(markers->size());
	int count = 0;
	for (auto&[idx, m] : *markers) {
		g_particles[count++].set(g_sim_time, idx, /*radius=*/4.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}

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

void BadmintonClearShot::init(std::map<int, Marker>* markers) {
	g_fields.clear();
	g_fields.push_back(new GravityOnEarth);
	g_fields.push_back(new AirResistance);

	Marker m1, m2;
	m1.x = 700.0f;
	m1.y = -700.0f;
	m1.vx = 80.0f;
	m1.vy = -80.0f;

	m2.x = 700.0f;
	m2.y = -700.0f;
	m2.vx = 80.0f;
	m2.vy = -90.0f;

	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(0.0, -600.0));
	g_env.back().push_back(vec2d(1500.0, -600.0));

	markers->clear();
	markers->emplace(0, m1);
	markers->emplace(1, m2);
	g_particles.resize(markers->size());
	int count = 0;
	for (auto&[idx, m] : *markers) {
		g_particles[count++].set(g_sim_time, idx, /*radius=*/4.0,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}
}

void RarefiedGas::init(std::map<int, Marker>* markers) {
	g_fields.clear();
	//g_fields.push_back(new GravityOnEarth);

	g_env.clear();
	g_env.push_back(std::vector<vec2d>());
	g_env.back().push_back(vec2d(600.0, -600.0));
	g_env.back().push_back(vec2d(1000.0, -600.0));
	g_env.back().push_back(vec2d(1000.0, -1000.0));
	g_env.back().push_back(vec2d(600.0, -1000.0));
	g_env.back().push_back(vec2d(600.0, -600.0));


	std::default_random_engine generator(1234);
	std::uniform_real_distribution<double> xy_dist(0.0 + 0.1, 200.0 - 0.1);
	std::uniform_real_distribution<double> vel_dist(-5.0, 5.0);

	markers->clear();
	Marker m;
	for (int i = 0; i < 1000; ++i) {
		markers->emplace(i, m);
		m.x = xy_dist(generator) + 600.0;
		m.y = xy_dist(generator) - 1000.0;
		m.vx = vel_dist(generator);
		m.vy = vel_dist(generator);
		markers->emplace(i, m);
	}

	g_particles.resize(markers->size());
	int count = 0;
	for (auto&[idx, m] : *markers) {
		g_particles[count++].set(g_sim_time, idx, /*radius=*/1.0 ,/*mass=*/1.0, vec2d(m.x, m.y), vec2d(m.vx, m.vy), vec2d(0.0, 0.0));
	}
}

void RarefiedGas::handle_boundary() {
	for (auto& p : g_particles) {
		if ((p.pos.x < 600.0 && p.vel.x < 0.0)|| (p.pos.x > 1000.0 && p.vel.x > 0.0))
			p.vel.x *= -1;
		if ((p.pos.y < -1000.0 && p.vel.y < 0.0) || (p.pos.y > -600.0 && p.vel.y > 0.0))
			p.vel.y *= -1;
	}
}

void RarefiedGas::handle_collision() {
	for (auto& p : g_particles)
		p.is_colliding = false;

	static std::set<std::pair<int, int>> collision_pairs;
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

