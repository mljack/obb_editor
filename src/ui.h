#pragma once

#include <vector>
#include <memory>

struct Entry {
	std::string path;
	std::string name;
	bool is_folder;
	std::vector<std::shared_ptr<Entry>> children;
};

bool load_prev_file(std::shared_ptr<Entry> base=nullptr);
bool load_next_file(std::shared_ptr<Entry> base=nullptr);
void render_ui();
