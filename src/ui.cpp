#include "ui.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <imgui.h>
#include <memory>
#include <map>
#include <algorithm>
#include <functional>
#include <filesystem>

#include "marker.h"
#include "pv.h"

namespace {
	bool g_rescan_files = true;
	std::shared_ptr<Entry> g_selected_tree_node = nullptr;
	std::shared_ptr<Entry> g_prev_node = nullptr;
	std::shared_ptr<Entry> g_file_list = nullptr;
	std::vector<std::shared_ptr<Entry>> g_path_stack;
}

extern bool g_parabola_test;
extern bool g_show_box;
extern bool g_show_cross;
extern bool g_show_point;
extern bool g_simulating;
extern bool g_replaying_sim;
extern double g_sim_time;
extern double g_sim_timestep;
extern std::map<int, Marker> g_markers;

void load_background(const std::string& file_path);
void recursive_sort_file_tree(std::shared_ptr<Entry> base);

// API for filelist.
extern "C" {
void new_filelist() {
	//printf("API: new_filelist\n");
	g_file_list = std::make_shared<Entry>();
	g_path_stack.push_back(g_file_list);
	g_rescan_files = false;
}

void add_folder(const char* folder_name) {
	//printf("API: add_folder\n");
	std::shared_ptr<Entry> p = std::make_shared<Entry>();
	p->path = "";
	p->name = folder_name;
	p->is_folder = true;
	g_path_stack.back()->children.push_back(p);
	g_path_stack.push_back(p);
}

void add_file(const char* path) {
	//printf("API: add_file\n");
	std::filesystem::path path2(path);
	std::shared_ptr<Entry> p = std::make_shared<Entry>();
	p->path = path;
	p->name = path2.filename().string();
	p->is_folder = false;
	g_path_stack.back()->children.push_back(p);
}

void pop_folder() {
	//printf("API: pop_folder %d\n", (int)g_path_stack.size());
	g_path_stack.pop_back();
}

void sort_filelist() {
	//printf("API: sort_filelist\n");
	recursive_sort_file_tree(g_file_list);
}

}	// extern "C"


void recursive_scan_folder(const std::string& dir, std::shared_ptr<Entry> base, const std::string& filters, bool recursive=true, char* file_filter=nullptr) {
	if (!std::filesystem::is_directory(dir))
		return;
	for (auto const& entry : std::filesystem::directory_iterator(dir)) {
		std::filesystem::path path(entry);
		bool is_folder = std::filesystem::is_directory(path);
		if (is_folder && !recursive)
			continue;
		if (is_folder) {
			add_folder(path.filename().string().c_str());
			recursive_scan_folder(path.string(), g_path_stack.back(), filters, recursive, file_filter);
		} else if (file_filter && strlen(file_filter) > 0 && path.filename().string().find(file_filter) == std::string::npos) {
			continue;
		} else {
			std::string filename(path.filename().string());
			bool matched = false;
			std::string::size_type pos_old = 0, pos = 0;
			std::string s = filters + "|";
			while (!matched && (pos = s.find('|', pos)) != std::string::npos) {
				std::string filter(filters.substr(pos_old, pos - pos_old));
				matched = filename.find(filter) != std::string::npos && filename.length() - filename.find(filter) == filter.length();
				pos_old = ++pos;
			}
			if (matched)
				add_file(path.string().c_str());
		}
	}
	pop_folder();
}

void recursive_sort_file_tree(std::shared_ptr<Entry> base) {
	if (base->children.empty())
		return;
	std::sort(base->children.begin(), base->children.end(),
		[ ](std::shared_ptr<Entry> a, std::shared_ptr<Entry> b) {
			if (a->is_folder == b->is_folder)
				return a->name < b->name;
			else
				return a->is_folder > b->is_folder;
		});
	for (auto& child: base->children)
		recursive_sort_file_tree(child);
}

void render_file_list(std::shared_ptr<Entry> base, std::function<void(const std::string& s)> callback, bool flat=false, bool recursive=true, const char* file_filter=nullptr)
{
	for (auto& child: base->children) {
		if (child->is_folder) {
			if (recursive && !child->children.empty()) {
				if (flat) {
					render_file_list(child, callback, flat, file_filter);
				} else if (ImGui::TreeNode(child->name.c_str())) {
					render_file_list(child, callback, flat, file_filter);
					ImGui::TreePop();
				}
			}
		} else {
			if (file_filter && strlen(file_filter) > 0 && child->name.find(file_filter) == std::string::npos)
				continue;
			bool selected = (g_selected_tree_node == child);
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 1,0,0,1 });
			if (ImGui::Button(child->name.c_str())) {
				callback(child->path);
				g_selected_tree_node = child;
			}
			if (selected)
				ImGui::PopStyleColor();
		}
	}
}

bool load_prev_file(std::shared_ptr<Entry> base) {
	if (!base) {
		g_prev_node = nullptr;
		base = g_file_list;
	}
	for (auto& child : base->children) {
		if (child->is_folder) {
			if (load_prev_file(child))
				return true;
		} else {
			if (g_selected_tree_node == child) {
				if (g_prev_node) {
					load_background(g_prev_node->path);
					g_selected_tree_node = g_prev_node;
				}
				return true;
			}
			g_prev_node = child;
		}
	}
	return false;
}

bool load_next_file(std::shared_ptr<Entry> base) {
	if (!base)
		base = g_file_list;
	for (auto& child : base->children) {
		if (child->is_folder) {
			if (load_next_file(child))
				return true;
		}	else {
			if (g_selected_tree_node == nullptr) {
				g_selected_tree_node = child;
				load_background(child->path);
				return true;
			} else if (g_selected_tree_node == child) {
				g_selected_tree_node = nullptr;
			}
		}
	}
	return false;
}

void render_side_bar() {
	static bool once = true;
	if (once) {
		once = false;
		ImGui::SetNextWindowSize(ImVec2(350, 900));
	}
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowBgAlpha(0.8f);
	char title[] = "SideBar ( )";
	if (g_is_modified)
		title[9] = '*';
	ImGui::Begin(title);

	if (g_marker) {
		auto* m = g_marker;
		ImGui::Text("[Current Marker Attributes]\n       ID: %4d\n        X: %7.2f\n        Y: %7.2f\n     Size: [%5.1f x %5.1f]\n  Heading:  %5.1f\n    Score:    %.3f\nCertainty:    %.3f",
				m->id, m->x, m->y, m->length, m->width, m->heading, m->score, m->certainty);
	} else {
		ImGui::Text("[Current Marker Attributes]\n       ID:\n        X:\n        Y:\n     Size:\n  Heading:\n    Score:\nCertainty:");
	}
	ImGui::Separator();

	ImGui::PushItemWidth(-180.0f);
	ImGui::Checkbox("Use Metric Thresholds:", &g_use_metric_threshold);
	ImGui::SliderFloat("Low Score Threshold", &g_low_score_threshold, 0.0f, 1.0f);
	ImGui::SliderFloat("High Score Threshold", &g_high_score_threshold, 0.0f, 1.0f);
	ImGui::SliderFloat("Low Certainty Threshold", &g_low_certainty_threshold, 0.0f, 1.0f);
	ImGui::SliderFloat("High Certainty Threshold", &g_high_certainty_threshold, 0.0f, 1.0f);
	ImGui::PopItemWidth();

	ImGui::Checkbox("Show ALL Markers", &g_show_all_markers);

#if defined(__EMSCRIPTEN__)
	if (ImGui::Button("Open Folder..."))
		EM_ASM(app.open_folder());
#else
	if (ImGui::Button("Refresh List"))
		g_rescan_files = true;
#endif
	ImGui::SameLine();
	static char folder_path[1024] = "images";
	ImGui::PushItemWidth(-6.0f);
	ImGui::InputText("##folder_path", folder_path, sizeof(folder_path));
	ImGui::PopItemWidth();
	char types[] = ".jpg|.jpeg|.png|.bmp|.JPG|.JPEG|.PNG|.BMP";
	static bool traverse_subfolders = true;
	if (ImGui::Checkbox("Recursive", &traverse_subfolders)) {
#if !defined(__EMSCRIPTEN__)
		g_rescan_files = true;
#endif
	}
#if defined(__EMSCRIPTEN__)
	EM_ASM({app.traverse_subfolders = $0}, traverse_subfolders);
#endif
	ImGui::SameLine();
	ImGui::PushItemWidth(-110.0f);
	static char file_filter[1024] = "";
	if (ImGui::InputText("Filename Filter", file_filter, sizeof(file_filter))) {
#if !defined(__EMSCRIPTEN__)
		g_rescan_files = true;
#endif
	}
	ImGui::PopItemWidth();
	ImGui::BeginChild("##file_list", ImVec2(330, 560), true, ImGuiWindowFlags_HorizontalScrollbar);
	if (g_rescan_files) {
		std::string base_path = std::string() + folder_path;
		if (std::filesystem::is_directory(base_path)) {
			new_filelist();
			recursive_scan_folder(base_path, g_file_list, types, traverse_subfolders, file_filter);
			recursive_sort_file_tree(g_file_list);
			g_rescan_files = false;
		}
	}

	if (!g_rescan_files && g_file_list)
		render_file_list(g_file_list, load_background, /*flat=*/false, /*recursive=*/traverse_subfolders, /*file_filter=*/file_filter);

	ImGui::EndChild();
	ImGui::End();
}

void render_combo(const char* title, const char** items, int n_items, int* idx) {
	const char* timestep_opt = items[*idx];
	if (ImGui::BeginCombo(title, timestep_opt)) {
		for (int i = 0; i < n_items; ++i) {
			bool is_selected = (*idx == i);
			if (ImGui::Selectable(items[i], is_selected))
				*idx = i;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

void render_simulation_settings() {
	static bool once = true;
	if (once) {
		once = false;
		ImGui::SetNextWindowSize(ImVec2(350, 900));
	}
	ImGui::SetNextWindowPos(ImVec2(1320, 10));
	ImGui::SetNextWindowBgAlpha(0.8f);
	ImGui::Begin("Simulation Settings");

	static int problem_idx = 1;
	static const char* problems[] = { "Rotated Parabola", "One-Body", "Two-Body", "Three-Body", "N-Body" };
	render_combo("Problem", problems, IM_ARRAYSIZE(problems), &problem_idx);
	g_parabola_test = (problem_idx == 0);
	if (g_parabola_test) {
		g_show_box = false;
		g_show_cross = g_show_point = true;
	} else {
		g_show_box = g_show_cross = false;
		g_show_point = true;
	}

	if (!g_parabola_test) {
		static int timestep_idx = 4;
		static double steps[] = { 0.00001, 0.0001, 0.001, 0.01, 0.1, 0.2, 0.5 };
		static const char* timestep_opts[] = { "0.00001", "0.0001", "0.001", "0.01", "0.1", "0.2", "0.5" };
		render_combo("Time Step", timestep_opts, IM_ARRAYSIZE(timestep_opts), &timestep_idx);
		g_sim_timestep = steps[timestep_idx];

		static int integrator_idx = 0;
		static const char* integrator_names[] = { "Eular", "Backward Eular", "Runge-Kutta (4th order)", "Verlet" };
		render_combo("Integrator", integrator_names, IM_ARRAYSIZE(integrator_names), &integrator_idx);
		const char* integrator_name = integrator_names[integrator_idx];

		static float sim_time_f = 0.0f;
		static float max_time = 0.0f;
		if (g_simulating) {
			if (ImGui::Button("Stop")) {
				g_simulating = false;
				stop_simulation(&g_markers);
			}
		} else {
			if (ImGui::Button("Start") && !g_markers.empty()) {
				if (sim_time_f < max_time)
					seek_to_sim_time_moment(max_time, &g_markers);
				g_simulating = true;
				start_simulation(g_markers);
			}
		}
		ImGui::SameLine();
		if (g_replaying_sim) {
			if (ImGui::Button("Stop Replay"))
				g_replaying_sim = false;
		} else {
			if (ImGui::Button("Replay")) {
				g_replaying_sim = true;
				if (sim_time_f >= max_time)
					sim_time_f = 0.0f;
			}
		}
		if (g_replaying_sim) {
			if (sim_time_f < max_time)
				seek_to_sim_time_moment(sim_time_f + g_sim_timestep, &g_markers);
			else
				g_replaying_sim = false;
		}

		if (ImGui::Button("Clear trajectories")) {
			g_particles.clear();
			g_sim_time = 0.0;
			max_time = 0.0f;
		}
		sim_time_f = (float)g_sim_time;
		if (ImGui::SliderFloat("Sim Time", &sim_time_f, 0.0f, max_time, "%.4f")) {
			seek_to_sim_time_moment(sim_time_f, &g_markers);
		}
		g_sim_time = sim_time_f;
		max_time = std::max(sim_time_f, max_time);
	}

	ImGui::End();
}

void render_ui() {
	render_side_bar();
	render_simulation_settings();
}
