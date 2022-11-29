#include <stdio.h>
#include <array>
#include <map>
#include <deque>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

#include <SDL.h>
#if defined(__EMSCRIPTEN__)
	#include <emscripten.h>
	#include <SDL_opengles2.h>
#elif defined(IMGUI_IMPL_OPENGL_ES2)
	
#else
	#if defined(_WIN32)
		#include <gl\glew.h>
	#endif
	#define GL_GLEXT_PROTOTYPES
	#include <SDL_opengl.h>
	//#include <SDL_opengl_glext.h>
#endif

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <rowmans.h>

#if defined(__EMSCRIPTEN__)
	#include "fs_helper.h"
#endif

#include "shaders.h"
#include "material.h"
#include "render_item.h"
#include "texture.h"
#include "marker.h"
#include "ui.h"


#if defined(__EMSCRIPTEN__)
// Emscripten wants to run the mainloop because of the way the browser is single threaded.
// For this, it requires a void() function. In order to avoid breaking the flow of the
// readability of this example, we're going to use a C++11 lambda to keep the mainloop code
// within the rest of the main function. This hack isn't needed in production, and one may
// simply have an actual main loop function to give to emscripten instead.
#include <functional>
static std::function<void()> loop;
static void main_loop() { loop(); }
#endif

#if 1
#define BACKGROUND_IMG "cat.jpg"
#else
#define BACKGROUND_IMG "6k.png"
#endif

#if defined(__EMSCRIPTEN__)
	std::string default_background = "/resources/" BACKGROUND_IMG;
#else
	std::string default_background = "../resources/" BACKGROUND_IMG;
#endif

namespace {
glm::vec3 red = {1.0f, 0.0f, 0.0f};
glm::vec3 yellow = {1.0f, 1.0f, 0.0f};
glm::vec3 blue = {0.0f, 0.0f, 1.0f};
glm::vec3 green = {0.0f, 1.0f, 0.0f};
glm::vec3 green2 = {0.0f, 0.8f, 0.0f};
glm::vec3 cyan = {0.0f, 1.0f, 1.0f};
glm::vec3 black = {0.0f, 0.0f, 0.0f};
glm::vec3 white = {1.0f, 1.0f, 1.0f};
};	// anonymous namespace

int g_window_width = 1600;
int g_window_height = 900;
int g_image_width = 0;
int g_image_height = 0;

float g_image_x = -1;
float g_image_y = -1;

bool g_image_dragging = false;
float g_offset_x = 0.0;
float g_offset_y = 0.0;
float g_offset_dx = 0.0;
float g_offset_dy = 0.0;
float g_drag_x = -1;
float g_drag_y = -1;
float g_scale = 1.0;

bool g_marker_dragging_ready = false;
bool g_marker_rotating_ready = false;
bool g_marker_front_edge_ready = false;
bool g_marker_back_edge_ready = false;
bool g_marker_left_edge_ready = false;
bool g_marker_right_edge_ready = false;
bool g_marker_right_clicked = false;
bool g_marker_dragging = false;
bool g_marker_rotating = false;
float g_marker_offset_dx = 0.0;
float g_marker_offset_dy = 0.0;
float g_marker_offset_heading = 0.0;
float g_marker_drag_x = -1;
float g_marker_drag_y = -1;

bool g_show_all_markers = false;
bool g_hide_manually_created_markers = false;
bool g_use_metric_threshold = true;
float g_low_score_threshold = 0.5f;
float g_high_score_threshold = 0.7f;
float g_low_certainty_threshold = 0.3f;
float g_high_certainty_threshold = 0.75f;

unsigned int g_background_texture_id = ~0U;

int g_max_marker_id = -1;
std::map<int, Marker> g_markers;
std::deque<Marker> g_undo_stack;
std::deque<Marker> g_redo_stack;

std::map<int, std::string> g_vehicle_type_name{ {-1,"ROI"}, {0,"car"}, {1,"bus"}, {2,"min_bus"}, {3,"truck"}, {4,"van"}, {5,"motor"}, {6,"bike"}, {7,"ped"}, {8,"big_truck"}, {9, "dont_care"} };

bool g_is_modified = false;
std::string g_filename;

Marker* g_marker = nullptr;

Marker* find_marker_hovered(float x, float y) {
	double min_d = 13.0f;
	for (auto& [idx, m] : g_markers) {
		float d2 = (x - m.x) * (x - m.x) + (y - m.y) * (y - m.y);
		if ((d2 < min_d * min_d) &&
			(g_show_all_markers || m.enabled) &&
			(!g_hide_manually_created_markers || !m.manually_created))
			return &m;
	}
	return nullptr;
}

Marker create_marker(float x, float y) {
	Marker m;
	m.id = ++g_max_marker_id;
	m.x = x;
	m.y = y;
	m.manually_created = true;
	if (g_marker) {
		m.length = g_marker->length;
		m.width = g_marker->width;
		m.heading = g_marker->heading;
	}
	return m;
}

void update_marker(const Marker& updated_marker) {
	bool new_created = (g_markers.count(updated_marker.id) == 0);
	if (new_created)
		g_markers.emplace(updated_marker.id, updated_marker);
	else if (g_markers.at(updated_marker.id) == updated_marker)	// not changed.
		return;

	if (g_undo_stack.size() > 100)
		g_undo_stack.pop_front();

	auto& old_marker = g_markers.at(updated_marker.id);
	g_undo_stack.push_back(old_marker);
	g_redo_stack.clear();
	old_marker = updated_marker;
}

void clear_marker_changes() {
	g_undo_stack.clear();
	g_redo_stack.clear();
}

void unapply_marker_change() {
	if (!g_undo_stack.empty()) {
		auto& old_m = g_undo_stack.back();
		auto& new_m = g_markers.at(old_m.id);
		g_redo_stack.push_back(new_m);
		if (old_m == new_m) {	// new created
			if (g_marker && g_marker->id == old_m.id)
				g_marker = nullptr;
			g_markers.erase(old_m.id);
		} else {	// modified
			new_m = old_m;
		}
		g_undo_stack.pop_back();
	}
}

void apply_marker_change() {
	if (!g_redo_stack.empty()) {
		auto& new_m = g_redo_stack.back();
		if (g_markers.count(new_m.id) == 0)	// new created
			g_markers.emplace(new_m.id, new_m);
		auto& old_m = g_markers.at(new_m.id);
		g_undo_stack.push_back(old_m);
		old_m = new_m;
		g_redo_stack.pop_back();
	}
}

void load_markers(const std::string& filename)
{
	static bool once = true;
	if (once) {
		once = false;
		//printf("load_markers once\n");
		return;
	}
	//printf("load_markers <%s>\n", filename.c_str());
#if defined(__EMSCRIPTEN__)
	int size = 0;
	char* buf = (char*)do_load(filename.c_str(), &size);
	//printf("load_markers size<%d>[%p]\n", size, buf);
	if (size == 0)
		return;
	//printf("load_markers before json print\n");
	//buf[size] = 0;
	std::string j_str(buf);
	//printf("load_markers json: @%s@\n", buf);
#else
	std::ifstream in(filename);
	if (!in.is_open())
		return;
	std::stringstream str_stream;
  str_stream << in.rdbuf(); //read the file
  std::string j_str = str_stream.str();
  in.close();
#endif

	nlohmann::json j;
	try {
		j = nlohmann::json::parse(j_str.begin(), j_str.end());
	}	catch (std::exception& e) {
		printf("Exception caught: %s\n", e.what());
		return;
	}

	if (j.empty())
		return;

	g_max_marker_id = -1;
	for (auto& markers_in_json : j) {
		for (auto& marker_in_json : markers_in_json) {
			Marker marker;
			marker.id = marker_in_json.value("id", -1);
			g_max_marker_id = std::max(g_max_marker_id, marker.id);
			marker.type = marker_in_json.value("type", 0);
			marker.x = marker_in_json["x"].get<double>();
			marker.y = marker_in_json["y"].get<double>();
			//marker.frame_id = marker_in_json["frame_id"];
			marker.enabled = marker_in_json.value("enabled", true);
			marker.manually_created = marker_in_json.value("manually_created", false);
			marker.score = marker_in_json.value("score", 1.0);
			marker.certainty = marker_in_json.value("certainty", 1.0);
			marker.width = marker_in_json.value("width", 2.4 * 10);
			marker.length = marker_in_json.value("length", 5.0 * 10);
			marker.heading = marker_in_json["heading_angle"].get<double>();
			g_markers.emplace_hint(g_markers.end(), std::make_pair(marker.id, marker));
		}
	}
	g_is_modified = false;
	clear_marker_changes();
}

void save_markers(const std::string& filename)
{
	nlohmann::json j;
	for (auto& [idx, marker] : g_markers) {
		nlohmann::json objectInfo = nlohmann::json::array();
		nlohmann::json info;
		info["id"] = marker.id;
		if (marker.type != 0)
			info["type"] = marker.type;
		info["x"] = marker.x;
		info["y"] = marker.y;
		info["frame_id"] = 0;
		info["length"] = marker.length;
		info["width"] = marker.width;
		if (!marker.enabled)
			info["enabled"] = false;
		if (marker.manually_created)
			info["manually_created"] = true;
		if (marker.score != 1.0f)
			info["score"] = marker.score;
		if (marker.certainty != 1.0f)
			info["certainty"] = marker.certainty;
		info["heading_angle"] = marker.heading;
		objectInfo.push_back(info);
		j.push_back(objectInfo);
	}
	std::string j_str = j.dump(2);
#if defined(__EMSCRIPTEN__)
	do_save(filename.c_str(), j_str.c_str());
#else
	std::ofstream out(filename);
	if (!out.is_open())
		return;
	out << j_str << std::endl;
	out.close();
#endif
	g_is_modified = false;
}

void handle_key_down_event(const SDL_Event& e) {
	if (e.key.keysym.sym == SDLK_UP) {
		load_prev_file();
	} else if (e.key.keysym.sym == SDLK_DOWN) {
		load_next_file();
	} else if (e.key.keysym.sym == SDLK_s) {
		std::filesystem::path path(g_filename);
		save_markers(path.replace_extension("vehicle_markers.json").string());
	} else if (e.key.keysym.sym == SDLK_o) {
#if defined(__EMSCRIPTEN__)
		EM_ASM(app.open_folder());
#endif
	} else if (e.key.keysym.sym == SDLK_d) {
		if (g_marker) {
			auto m = *g_marker;
			m.enabled = !m.enabled;
			update_marker(m);
		}
	} else if (e.key.keysym.sym == SDLK_LSHIFT) {
		g_show_all_markers = true;
	} else if (e.key.keysym.sym == SDLK_LALT) {
		g_hide_manually_created_markers = true;
	} else if (e.key.keysym.sym == SDLK_z) {
		unapply_marker_change();
	} else if (e.key.keysym.sym == SDLK_c) {
		apply_marker_change();
	} else if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
		auto m = *g_marker;
		m.type = int(e.key.keysym.sym - SDLK_0);
		update_marker(m);
	}
}

void handle_key_up_event(const SDL_Event& e)
{
	if (e.key.keysym.sym == SDLK_LSHIFT) {
		g_show_all_markers = false;
	} else if (e.key.keysym.sym == SDLK_LALT) {
		g_hide_manually_created_markers = false;
	}
}

void handle_mouse_down_event(const SDL_Event& e) {
	g_image_x = (e.button.x - g_offset_x - g_offset_dx) / g_scale;
	g_image_y = (e.button.y + g_offset_y + g_offset_dy - g_window_height) / g_scale + g_image_height;

	if (e.button.button == SDL_BUTTON_LEFT) {
		g_drag_x = e.button.x;
		g_drag_y = e.button.y;
		g_image_dragging = true;
	} else if (e.button.button == SDL_BUTTON_RIGHT) {
		g_marker_drag_x = g_image_x;
		g_marker_drag_y = g_image_y;
		SDL_Keymod mod = SDL_GetModState();
		if (mod & KMOD_LCTRL) {
			auto m = create_marker(g_image_x, g_image_y);
			update_marker(m);
			g_marker = &g_markers.at(m.id);
		} else if (g_marker_dragging_ready) {
			g_marker_dragging = true;
		} else if (g_marker_rotating_ready) {
			g_marker_rotating = true;
		} else if (g_marker) {
			g_marker_right_clicked = true;
		}
		//printf("marker: [%.1f, %.1f, %.1f, %.1f, %.1f]\n", g_marker->x, g_marker->y, g_marker->length, g_marker->width, g_marker->heading);
	}
	//printf("mouse: [%.1f, %.1f]\n", g_image_x, g_image_y);
}

void handle_mouse_up_event(const SDL_Event& e) {
	g_image_x = (e.button.x - g_offset_x - g_offset_dx) / g_scale;
	g_image_y = (e.button.y + g_offset_y + g_offset_dy - g_window_height) / g_scale + g_image_height;

	if (e.button.button == SDL_BUTTON_LEFT) {
		g_offset_x += g_offset_dx;
		g_offset_y += g_offset_dy;
		g_offset_dx = 0;
		g_offset_dy = 0;
		g_drag_x = -1;
		g_drag_y = -1;
		g_image_dragging = false;
	} else if (e.button.button == SDL_BUTTON_RIGHT && g_marker) {
		auto m = *g_marker;
		if (g_marker_right_clicked) {
			g_marker_right_clicked = false;
			glm::vec2 c(m.x, m.y);
			glm::vec2 v(g_image_x, g_image_y);
			v -= c;
			float yaw = glm::radians(m.heading);
			glm::vec2 dir(std::cos(yaw), std::sin(yaw));
			glm::vec2 normal(-std::sin(yaw), std::cos(yaw));
			float s = dot(v, dir);
			float l = dot(v, normal);
			if (g_marker_front_edge_ready) {
				c += (s - m.length / 2) / 2 * dir;
				m.length = s + m.length / 2;
			} else if (g_marker_back_edge_ready) {
				c += (s + m.length / 2) / 2 * dir;
				m.length = -s + m.length / 2;
			} else if (g_marker_left_edge_ready) {
				c += (l + m.width / 2) / 2 * normal;
				m.width = -l + m.width / 2;
			} else if (g_marker_right_edge_ready) {
				c += (l - m.width / 2) / 2 * normal;
				m.width = l + m.width / 2;
			}
			m.x = c.x;
			m.y = c.y;
		} else {
			m.x += g_marker_offset_dx;
			m.y += g_marker_offset_dy;
			m.heading = std::fmod(m.heading + g_marker_offset_heading, 360.0f);
			g_marker_offset_dx = 0.0;
			g_marker_offset_dy = 0.0;
			g_marker_offset_heading = 0.0;
			g_marker_drag_x = -1;
			g_marker_drag_y = -1;
			g_marker_dragging = false;
			g_marker_rotating = false;
		}
		update_marker(m);
		g_is_modified = true;
		//printf("marker: [%.1f, %.1f, %.1f, %.1f, %.1f]\n", g_marker->x, g_marker->y, g_marker->length, g_marker->width, g_marker->heading);
	} else if (e.button.button == SDL_BUTTON_MIDDLE && g_marker) {
		auto m = *g_marker;
		m.heading = std::fmod(m.heading + 180.0f, 360.0f);
		update_marker(m);
		g_is_modified = true;
	}
}

void handle_mouse_move_event(const SDL_Event& e) {
	//printf("mouse: [%d, %d]\n", e.motion.x, e.motion.y);
	if (g_marker_right_clicked) {
		g_marker_right_clicked = false;
		g_marker_rotating = true;
		g_marker_rotating_ready = true;
		g_marker_front_edge_ready = false;
		g_marker_back_edge_ready = false;
		g_marker_left_edge_ready = false;
		g_marker_right_edge_ready = false;
	}
	if (g_image_dragging) {
		g_offset_dx = e.motion.x - g_drag_x;
		g_offset_dy = -(e.motion.y - g_drag_y);
	}
	//printf("g_offset: [%.1f, %.1f]\n", g_offset_x, g_offset_y);
	g_image_x = (e.motion.x - g_offset_x - g_offset_dx) / g_scale;
	g_image_y = (e.motion.y + g_offset_y + g_offset_dy - g_window_height) / g_scale + g_image_height;
	//printf("image: [%.1f, %.1f]\n", g_image_x, g_image_y);

	if (g_marker_dragging) {
		g_marker_offset_dx = g_image_x - g_marker_drag_x;
		g_marker_offset_dy = g_image_y - g_marker_drag_y;
		//printf("marker dxdy: [%.1f, %.1f]\n", g_marker_offset_dx, g_marker_offset_dy);
	} else if (g_marker_rotating) {
		g_marker_offset_heading = (g_marker_drag_x - g_image_x) * g_scale * 0.1;
	} else {
		auto* m = find_marker_hovered(g_image_x, g_image_y);
		if (m)
			g_marker = m;

		if (g_marker) {
			glm::vec2 v(g_image_x - g_marker->x, g_image_y - g_marker->y);
			float yaw = glm::radians(g_marker->heading);
			glm::vec2 dir(std::cos(yaw), std::sin(yaw));
			glm::vec2 normal(-std::sin(yaw), std::cos(yaw));
			float s = dot(v, dir);
			float l = dot(v, normal);
			float length = g_marker->length / 2;
			float width = g_marker->width / 2;
			float r = std::sqrt(s * s + l * l);
			float R = std::sqrt(width * width + length * length);
			//printf("marker dist: [%.1f, %.1f]\n", s, l);
			float ratio = std::abs(s / l);
			float marker_ratio = length / width;
			float drag_radius = width / 2;
			g_marker_dragging_ready = (r <= drag_radius);
			g_marker_front_edge_ready = (r > drag_radius && s > 0 && ((s < length && ratio > marker_ratio) || (s >= length && std::abs(l) < width && s < R * 5)));
			g_marker_back_edge_ready = (r > drag_radius && s < 0 && ((s > -length && ratio > marker_ratio) || (s <= -length && std::abs(l) < width && s > -R * 5)));
			g_marker_left_edge_ready = (r > drag_radius && l < 0 && ((l >- width && ratio < marker_ratio) || (l <= -width && std::abs(s) < length && l > -width * 5)));
			g_marker_right_edge_ready = (r > drag_radius && l > 0 && ((l < width && ratio < marker_ratio) || (l >= width && std::abs(s) < length && l < width * 5)));
			g_marker_rotating_ready = !(g_marker_dragging_ready || g_marker_front_edge_ready || g_marker_back_edge_ready || g_marker_left_edge_ready || g_marker_right_edge_ready);
		}
	}
}

void handle_mouse_wheel_event(const SDL_Event& e) {
#if defined(__EMSCRIPTEN__)
	//printf("wheel [%d][%d][%d, %d][%f, %f]\n", e.wheel.timestamp, e.wheel.direction, e.wheel.x, e.wheel.y, e.wheel.preciseX, e.wheel.preciseY);
	bool up = e.wheel.preciseY > 0;
#else
	bool up = e.wheel.y > 0;
#endif

	float s = 1.2f;
	if (!up)
		s = 1.0f / s;

	g_offset_x += g_image_x * g_scale * (1 - s) ;
	g_offset_y += (g_image_height - g_image_y) * g_scale * (1 - s) ;
	g_scale = std::min(200.0f, std::max(1.0f/200.0f, g_scale*s));
}

Material* g_background_img_material = nullptr;
Material* g_line_material = nullptr;

void clean_up() {
	delete g_background_img_material;
	delete g_line_material;
	// Terminate GLFW, clearing any resources allocated by GLFW.
	//glfwTerminate();
	//OUT std::cout << "exit main" << std::endl;
}

void build_text(const std::string& s, const glm::vec3& p, const glm::vec3& color, float scale, std::vector<GLfloat>* v_buf, std::vector<GLuint>* idx_buf)
{
	static char char_map[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
	float x_offset = 0;
	for (char c : s) {
		int char_id = 0;
		for(; char_id < (int)sizeof(char_map); ++char_id)
			if (char_map[char_id] == c)
				break;
		int strokes_count = rowmans_size[char_id];
		auto strokes = rowmans[char_id];

		for (int kk = 0; kk < strokes_count; kk += 4) {
			GLuint base_idx = (GLuint)v_buf->size() / 7;
			v_buf->push_back(p.x + scale * (strokes[kk + 0] + x_offset));
			v_buf->push_back(p.y + scale * (rowmans_height - strokes[kk + 1]));
			v_buf->push_back(12.0f);
			v_buf->push_back(color.x); v_buf->push_back(color.y); v_buf->push_back(color.z); v_buf->push_back(1.0f);
			v_buf->push_back(p.x + scale * (strokes[kk + 2] + x_offset));
			v_buf->push_back(p.y + scale * (rowmans_height - strokes[kk + 3]));
			v_buf->push_back(12.0f);
			v_buf->push_back(color.x); v_buf->push_back(color.y); v_buf->push_back(color.z); v_buf->push_back(1.0f);
			idx_buf->push_back(base_idx + 0); idx_buf->push_back(base_idx + 1);
		}
		x_offset += rowmans_width[char_id];
	}
}

void build_latest_marker_geom(std::vector<GLfloat>* v_buf, std::vector<GLuint>* idx_buf) {
	float yaw = glm::radians(g_marker->heading + g_marker_offset_heading);
	glm::vec2 c(g_marker->x + g_marker_offset_dx, g_marker->y + g_marker_offset_dy);
	glm::vec2 dir(std::cos(yaw), std::sin(yaw));
	glm::vec2 normal(-std::sin(yaw), std::cos(yaw));

	float length = g_marker->length / 2;
	float width = g_marker->width / 2;
	float scale = 10.0f;
	std::array<glm::vec2, 20> pts = {
		c - length * dir - width * normal,
		c + length * dir - width * normal,
		c + length * dir - width * normal,
		c + length * dir + width * normal,
		c + length * dir + width * normal,
		c - length * dir + width * normal,
		c - length * dir + width * normal,
		c - length * dir - width * normal,
		c + length * dir,
		c + (length + width / 2) * dir,
		c + (length + width / 4) * dir + width / 4 * normal,
		c + (length + width / 4) * dir - width / 4 * normal,
		c - glm::vec2(scale, 0.0f),
		c + glm::vec2(scale, 0.0f),
		c - glm::vec2(0.0f, scale),
		c + glm::vec2(0.0f, scale),
		c - glm::vec2(scale/4, 0.0f),
		c - glm::vec2(0.0f, scale/4),
		c + glm::vec2(scale/4, 0.0f),
		c + glm::vec2(0.0f, scale/4),
	};
	GLuint base_idx = (GLuint)v_buf->size() / 7;
	for (auto& pt : pts) {
		v_buf->push_back(pt.x);
		v_buf->push_back(g_image_height - pt.y);
		v_buf->push_back(10.0f);
		auto idx = &pt-pts.data();
		bool ready = false;
		ready = ready || (g_marker_dragging_ready && idx >=12 && idx <= 19);
		ready = ready || (g_marker_front_edge_ready && idx >=2 && idx <= 3);
		ready = ready || (g_marker_back_edge_ready && idx >= 6 && idx <= 7);
		ready = ready || (g_marker_left_edge_ready && idx >= 0 && idx <= 1);
		ready = ready || (g_marker_right_edge_ready && idx >= 4 && idx <= 5);
		ready = ready || (g_marker_rotating_ready && idx >= 8 && idx <= 11);
		if (ready) {
			v_buf->push_back(1.0f); v_buf->push_back(0.0f); v_buf->push_back(0.0f); v_buf->push_back(1.0f);
		} else {
			v_buf->push_back(0.0f); v_buf->push_back(0.0f); v_buf->push_back(1.0f); v_buf->push_back(1.0f);
		}
	}
	// box
	idx_buf->push_back(base_idx + 0); idx_buf->push_back(base_idx + 1);
	idx_buf->push_back(base_idx + 2); idx_buf->push_back(base_idx + 3);
	idx_buf->push_back(base_idx + 4); idx_buf->push_back(base_idx + 5);
	idx_buf->push_back(base_idx + 6); idx_buf->push_back(base_idx + 7);

	// arrow
	idx_buf->push_back(base_idx + 8); idx_buf->push_back(base_idx + 9);
	idx_buf->push_back(base_idx + 9); idx_buf->push_back(base_idx + 10);
	idx_buf->push_back(base_idx + 9); idx_buf->push_back(base_idx + 11);

	// center
	idx_buf->push_back(base_idx + 12); idx_buf->push_back(base_idx + 13);
	idx_buf->push_back(base_idx + 14); idx_buf->push_back(base_idx + 15);
	idx_buf->push_back(base_idx + 16); idx_buf->push_back(base_idx + 17);
	idx_buf->push_back(base_idx + 17); idx_buf->push_back(base_idx + 18);
	idx_buf->push_back(base_idx + 18); idx_buf->push_back(base_idx + 19);
	idx_buf->push_back(base_idx + 19); idx_buf->push_back(base_idx + 16);
}

void build_marker_geom(const Marker& m, const glm::vec3& color, std::vector<GLfloat>* v_buf, std::vector<GLuint>* idx_buf) {
	float yaw = glm::radians(m.heading);
	glm::vec2 c(m.x, m.y);
	glm::vec2 dir(std::cos(yaw), std::sin(yaw));
	glm::vec2 normal(-std::sin(yaw), std::cos(yaw));

	float length = m.length / 2;
	float width = m.width / 2;
	float scale = 10.0f;
	std::array<glm::vec2, 16> pts = {
		c - length * dir - width * normal,
		c + length * dir - width * normal,
		c + length * dir + width * normal,
		c - length * dir + width * normal,
		c + length * dir,
		c + (length + width / 2) * dir,
		c + (length + width / 4) * dir + width / 4 * normal,
		c + (length + width / 4) * dir - width / 4 * normal,
		c - glm::vec2(scale, 0.0f),
		c + glm::vec2(scale, 0.0f),
		c - glm::vec2(0.0f, scale),
		c + glm::vec2(0.0f, scale),
		c - glm::vec2(scale/4, 0.0f),
		c - glm::vec2(0.0f, scale/4),
		c + glm::vec2(scale/4, 0.0f),
		c + glm::vec2(0.0f, scale/4),
	};
	GLuint base_idx = (GLuint)v_buf->size() / 7;
	for (auto& pt : pts) {
		v_buf->push_back(pt.x);
		v_buf->push_back(g_image_height - pt.y);
		v_buf->push_back(10.0f);
		v_buf->push_back(color.x); v_buf->push_back(color.y); v_buf->push_back(color.z); v_buf->push_back(1.0f);
	}
	// box
	idx_buf->push_back(base_idx + 0); idx_buf->push_back(base_idx + 1);
	idx_buf->push_back(base_idx + 1); idx_buf->push_back(base_idx + 2);
	idx_buf->push_back(base_idx + 2); idx_buf->push_back(base_idx + 3);
	idx_buf->push_back(base_idx + 3); idx_buf->push_back(base_idx + 0);

	// arrow
	idx_buf->push_back(base_idx + 4); idx_buf->push_back(base_idx + 5);
	idx_buf->push_back(base_idx + 5); idx_buf->push_back(base_idx + 6);
	idx_buf->push_back(base_idx + 5); idx_buf->push_back(base_idx + 7);

	// center
	idx_buf->push_back(base_idx + 8); idx_buf->push_back(base_idx + 9);
	idx_buf->push_back(base_idx + 10); idx_buf->push_back(base_idx + 11);
	idx_buf->push_back(base_idx + 12); idx_buf->push_back(base_idx + 13);
	idx_buf->push_back(base_idx + 13); idx_buf->push_back(base_idx + 14);
	idx_buf->push_back(base_idx + 14); idx_buf->push_back(base_idx + 15);
	idx_buf->push_back(base_idx + 15); idx_buf->push_back(base_idx + 12);
}

void build_markers_buffer(const std::map<int, Marker>& markers, std::vector<GLfloat>* v_buf, std::vector<GLuint>* idx_buf,
	std::vector<GLfloat>* v_buf2, std::vector<GLuint>* idx_buf2, std::vector<GLfloat>* v_buf3, std::vector<GLuint>* idx_buf3) {
	for (const auto& [idx, m] : markers) {
		auto& v_type = g_vehicle_type_name.at(m.type);
		glm::vec3 c = red;
		if (&m == g_marker) {
			if (!g_show_all_markers && !m.enabled)
				continue;
			if (g_hide_manually_created_markers && m.manually_created)
				continue;
			build_latest_marker_geom(v_buf, idx_buf);
			if (m.type != 0) {
				glm::vec3 p(m.x + g_marker_offset_dx - 5.0f, g_image_height - m.y - g_marker_offset_dy - 5.0f, 0.0f);
				build_text(v_type, p, green2, 1.0f, v_buf2, idx_buf2);
				build_text(v_type, p, black, 1.0f, v_buf3, idx_buf3);
			}
		} else {
			if (!g_show_all_markers && !m.enabled)
				continue;
			if (g_hide_manually_created_markers && m.manually_created)
				continue;
			if (g_use_metric_threshold) {
				if (m.score < g_low_score_threshold)
					c = yellow;
				else if (m.score < g_high_score_threshold)
					c = green;

				if (m.certainty > g_low_certainty_threshold && m.certainty < g_high_certainty_threshold)
					c = cyan;
			}
			build_marker_geom(m, c, v_buf, idx_buf);
			if (m.type != 0) {
				glm::vec3 p(m.x - 5.0f, g_image_height - m.y - 5.0f, 0.0f);
				build_text(v_type, p, green2, 1.0f, v_buf2, idx_buf2);
				build_text(v_type, p, black, 1.0f, v_buf3, idx_buf3);
			}
		}
	}
}

void load_background(const std::string& file_path) {
	if (g_background_texture_id != ~0U)
		delete_texture(g_background_texture_id);

	g_background_texture_id = load_texture(file_path, &g_image_width, &g_image_height);
	g_scale = std::min(g_window_width / (float)g_image_width, g_window_height / (float)g_image_height);
	g_offset_x = (g_window_width - g_image_width * g_scale) / 2;
	g_offset_y = (g_window_height - g_image_height * g_scale) / 2;

	g_marker = nullptr;
	g_markers.clear();
	g_filename = file_path;
	std::filesystem::path path(g_filename);
	auto marker_path = path.replace_extension("vehicle_markers.json").string();
	load_markers(marker_path);
}

int main(int, char**)
{
	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to the latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Decide GL+GLSL versions
#if defined(__EMSCRIPTEN__)
	// For the browser using Emscripten, we are going to use WebGL1 with GL ES2. See the Makefile. for requirement details.
	// It is very likely the generated file won't work in many browsers. Firefox is the only sure bet, but I have successfully
	// run this code on Chrome for Android for example.
	// GL ES 3.0 + GLSL 300 ES (WebGL2)
	const char* glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

#if defined(__EMSCRIPTEN__)
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
#elif defined(__linux__)
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
#elif defined(_WIN32)
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED);
#else
	#error Unsupported platform
#endif
	SDL_Window* window = SDL_CreateWindow("obb_editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_window_width, g_window_height, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

#if defined(_WIN32)
	//Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		return -1;
	}
#endif

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	g_background_img_material = new Material;
	if (!g_background_img_material->build(vertex_shader_src, fragment_shader_src))
		return -1;
	g_line_material = new Material;
	if (!g_line_material->build(vertex_shader_src2, fragment_shader_src2))
		return -1;

#if 1
#define BACKGROUND_IMG "cat.jpg"
#else
#define BACKGROUND_IMG "6k.png"
#endif

#if defined(__EMSCRIPTEN__)
	std::string default_background = "/resources/" BACKGROUND_IMG;
#else
	std::string default_background = "../resources/" BACKGROUND_IMG;
#endif

	load_background(default_background);

	std::vector<GLfloat> vertices2 = {
		// x  y     z     r     g     b     a     u     v
		0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f, 0.0f,
	};
	std::vector<GLuint> indices2 = {
		0, 1, 2, 0, 2, 3
	};

	RenderItem rect(GL_TRIANGLES);
	RenderItem lines(GL_LINES);
	RenderItem wide_lines(GL_LINES);
	RenderItem wide_lines2(GL_LINES);

	bool initialized = false;

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;
#if defined(__EMSCRIPTEN__)
	// See comments around line 30.
	loop = [&]()
	// Note that this doesn't process the 'done' boolean anymore. The function emscripten_set_main_loop will
	// in effect not return, and will set an infinite loop, that is going to be impossible to break. The
	// application can then only stop when the brower's tab is closed, and you will NOT get any notification
	// about exitting. The browser will then cleanup all resources on its own.
#else
	while (!done)
#endif
	{

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
				done = true;
			else if (!io.WantCaptureKeyboard) {
				if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.sym == SDLK_ESCAPE)
						done = true;
					else
						handle_key_down_event(event);
				} else if (event.type == SDL_KEYUP)
						handle_key_up_event(event);
			}

			if (!io.WantCaptureMouse) {
				if (event.type == SDL_MOUSEBUTTONDOWN)
					handle_mouse_down_event(event);
				else if (event.type == SDL_MOUSEBUTTONUP)
					handle_mouse_up_event(event);
				else if (event.type == SDL_MOUSEWHEEL)
					handle_mouse_wheel_event(event);
				else if (event.type == SDL_MOUSEMOTION)
					handle_mouse_move_event(event);
			}
		}

		if (!initialized) {
			SDL_GL_GetDrawableSize(window, &g_window_width, &g_window_height);
			printf("canvas size: [%d, %d]\n", g_window_width, g_window_height);
			load_background(default_background);
			rect.set_material(g_background_img_material);
			rect.update_buffers_for_textured_geoms(vertices2, indices2);
			lines.set_material(g_line_material);
			wide_lines.set_material(g_line_material);
			wide_lines2.set_material(g_line_material);
			initialized = true;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		render_ui();

		// Rendering Viewport
		glViewport(0, 0, g_window_width, g_window_height);
		//printf("[%d x %d]\n", (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glm::mat4 model(1.0);
		glm::mat4 view(1.0);
		model = glm::scale(model, glm::vec3(g_image_width, g_image_height, 0.0f));
		view = glm::translate(view, glm::vec3(g_offset_x + g_offset_dx, g_offset_y + g_offset_dy, 0.0f));
		view = glm::scale(view, glm::vec3(g_scale, g_scale, 1.0f));
		glm::mat4 proj = glm::ortho(0.0f, (float)g_window_width, 0.0f, (float)g_window_height, -1000.0f, 1000.0f);
		{
			glm::mat4 xform = proj * view * model;
			glBindTexture(GL_TEXTURE_2D, g_background_texture_id);
			rect.render_textured(xform);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		{
			std::vector<GLfloat> line_buf, wide_line_buf, wide_line_buf2;
			std::vector<GLuint> line_idx, wide_line_idx, wide_line_idx2;
			build_markers_buffer(g_markers, &line_buf, &line_idx, &wide_line_buf, &wide_line_idx, &wide_line_buf2, &wide_line_idx2);
			lines.update_buffers_for_nontextured_geoms(line_buf, line_idx);
			wide_lines.update_buffers_for_nontextured_geoms(wide_line_buf, wide_line_idx);
			wide_lines2.update_buffers_for_nontextured_geoms(wide_line_buf2, wide_line_idx2);

			glDisable(GL_DEPTH_TEST);
			glm::mat4 xform = proj * view;
			glLineWidth(6.0f);
			wide_lines2.render_nontextured(xform);
			glLineWidth(3.0f);
			wide_lines.render_nontextured(xform);
			glLineWidth(2.0f);
			lines.render_nontextured(xform);
			glLineWidth(1.0f);
			glEnable(GL_DEPTH_TEST);
		}

		// Rendering GUI
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
#if !defined(__EMSCRIPTEN__)
		SDL_Delay(17);
#endif
	};
#if defined(__EMSCRIPTEN__)
	// See comments around line 30.
	// This function call will not return.
	emscripten_set_main_loop(main_loop, 0, true);
	// A fully portable version of this code that doesn't use the lambda hack might have an #else that does:
	//    while (!done) main_loop();
#endif

	clean_up();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
