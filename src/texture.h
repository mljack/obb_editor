#pragma once

unsigned int load_texture(const char* file_path, int* width, int* height) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int nrChannels;
    printf("[%s]\n", file_path);
    unsigned char *data = stbi_load(file_path, width, height, &nrChannels, 0);
    if (data) {
        printf("[%d x %d], %d\n", *width, *height, nrChannels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (nrChannels > 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, *width, *height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return texture;
}