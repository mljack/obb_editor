#pragma once

#include <string>

unsigned int load_texture(const std::string& file_path, int* width, int* height);
void delete_texture(unsigned int texture_id);