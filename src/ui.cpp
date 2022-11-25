#include "ui.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <imgui.h>
#include <memory>
#include <algorithm>
#include <functional>
#include <filesystem>

#include "marker.h"

namespace {
	bool g_rescan_files = true;
	std::shared_ptr<Entry> g_selected_tree_node = nullptr;
	std::shared_ptr<Entry> g_prev_node = nullptr;
	std::shared_ptr<Entry> g_file_list = nullptr;
	std::vector<std::shared_ptr<Entry>> g_path_stack;
}

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
	p->name = path2.filename().c_str();
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
			add_folder(path.filename().c_str());
			recursive_scan_folder(path, g_path_stack.back(), filters, recursive, file_filter);
		} else if (file_filter && strlen(file_filter) > 0 && std::string(path.filename()).find(file_filter) == std::string::npos) {
			continue;
		} else {
			std::string filename(path.filename());
			bool matched = false;
			std::string::size_type pos_old = 0, pos = 0;
			std::string s = filters + "|";
			while (!matched && (pos = s.find('|', pos)) != std::string::npos) {
				std::string filter(filters.substr(pos_old, pos - pos_old));
				matched = filename.find(filter) != std::string::npos && filename.length() - filename.find(filter) == filter.length();
				pos_old = ++pos;
			}
			if (matched)
				add_file(path.c_str());
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

void show_file_list() {
	static bool once = true;
	if (once) {
		once = false;
		ImGui::SetNextWindowSize(ImVec2(350, 900));
	}
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowBgAlpha(0.8f);
	ImGui::Begin("Side Bar");

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
	ImGui::SameLine();
	ImGui::Checkbox("Hide Manual Markers", &g_hide_manually_created_markers);

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

void render_ui() {
  show_file_list();
}
