#include "texture.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <SDL_opengles2.h>
#elif defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
//#include <SDL_opengl_glext.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if defined(__EMSCRIPTEN__)
#include "fs_helper.h"
#endif

#include <cstdio>
#include <vector>

unsigned char* load_image_file(const std::string& path, int* size) {
	FILE* fp = fopen(path.c_str(), "rb");
	if (!fp)
		return nullptr;
	fseek(fp, 0L, SEEK_END);
	*size = (int)ftell(fp);
	unsigned char* buf = (unsigned char*)malloc(*size);
	fseek(fp, 0L, SEEK_SET);	
	fread(buf, sizeof(char), *size, fp);
	fclose(fp);
	return buf;
}

void delete_texture(unsigned int texture_id) {
	glDeleteTextures(1, &texture_id);
}

unsigned int load_texture(const std::string& file_path, int* width, int* height) {
	printf("[%s]\n", file_path.c_str());
	int size = 0;
	unsigned char* buf = load_image_file(file_path, &size);
#if defined(__EMSCRIPTEN__)
	if (!buf)	// try to load from js side if it's not a local file.
		buf = do_load(file_path.c_str(), &size);
#endif
	if (size == 0) {
		printf("Failed to load [%s]\n", file_path.c_str());
	}

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int nrChannels;
	unsigned char *data = stbi_load_from_memory(buf, size, width, height, &nrChannels, 0);
	free(buf);
	if (data) {
		printf("[%d x %d], %d\n", *width, *height, nrChannels);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		if (nrChannels > 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, *width, *height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else {
		printf("Failed to load texture\n");
	}
	stbi_image_free(data);
	return texture;
}
