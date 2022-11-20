#include "ui.h"

#include <imgui.h>
#include <memory>
#include <algorithm>
#include <functional>
#include <filesystem>

namespace {
	bool g_rescan_files = true;
	std::shared_ptr<Entry> g_selected_tree_node = nullptr;
	std::shared_ptr<Entry> g_prev_node = nullptr;
	std::shared_ptr<Entry> g_file_list = nullptr;
	std::vector<std::shared_ptr<Entry>> g_path_stack;
}

void load_background(const std::string& file_path);

// API for filelist.
extern "C" {
void new_filelist() {
	g_file_list = std::make_shared<Entry>();
	g_path_stack.push_back(g_file_list);
}

void new_folder(const char* folder_name) {
	std::shared_ptr<Entry> p = std::make_shared<Entry>();
	p->path = "";
	p->name = folder_name;
	p->is_folder = true;
	g_path_stack.back()->children.push_back(p);
	g_path_stack.push_back(p);
}

void add_file(const char* path) {
	std::filesystem::path path2(path);
	std::shared_ptr<Entry> p = std::make_shared<Entry>();
	p->path = path;
	p->name = path2.filename().c_str();
	p->is_folder = false;
	g_path_stack.back()->children.push_back(p);
}

void pop_folder() {
	g_path_stack.pop_back();
}
}

void recursive_scan_folder(const std::string& dir, std::shared_ptr<Entry> base, const std::string& filters, bool recursive=true, char* file_filter=nullptr) {
	if (!std::filesystem::is_directory(dir))
		return;
	for (auto const& entry : std::filesystem::directory_iterator(dir)) {
		std::filesystem::path path(entry);
		bool is_folder = std::filesystem::is_directory(path);
		if (is_folder && !recursive)
			continue;
		if (is_folder) {
			new_folder(path.filename().c_str());
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
		[ ](std::shared_ptr<Entry> a, std::shared_ptr<Entry> b) { return a->name < b->name; });
	for (auto& child: base->children)
		recursive_sort_file_tree(child);
}

void render_file_list(std::shared_ptr<Entry> base, std::function<void(const std::string& s)> callback,	bool flat=false)
{
	for (auto& child: base->children) {
		if (child->is_folder) {
			if (flat) {
				render_file_list(child, callback, flat);
			} else if (ImGui::TreeNode(child->name.c_str())) {
				render_file_list(child, callback, flat);
				ImGui::TreePop();
			}
		} else {
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
		ImGui::SetNextWindowSize(ImVec2(350, 800));
	}
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	//ImGui::SetNextWindowBgAlpha(0.6f);
	ImGui::Begin("File List");
	if (ImGui::Button("Refresh List"))
		g_rescan_files = true;
	ImGui::SameLine();
	static char folder_path[1024] = "images";
	ImGui::InputText("##folder_path", folder_path, sizeof(folder_path));
	char types[] = ".jpg|.jpeg|.png|.bmp|.JPG|.JPEG|.PNG|.BMP";
	static bool traverse_subfolders = true;
	if (ImGui::Checkbox("Recursive", &traverse_subfolders))
		g_rescan_files = true;
	ImGui::SameLine();
	ImGui::PushItemWidth(-110.0f);
	static char file_filter[1024] = "";
	if (ImGui::InputText("Filename Filter", file_filter, sizeof(file_filter)))
		g_rescan_files = true;
	ImGui::PopItemWidth();
	ImGui::BeginChild("##file_list", ImVec2(330, 700), true, ImGuiWindowFlags_HorizontalScrollbar);
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
		render_file_list(g_file_list, load_background, /*flat=*/false);

	ImGui::EndChild();
	ImGui::End();
}

void render_ui() {
  show_file_list();
}
