#pragma once

class Material {
public:
    bool build(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource) {
        // Build and compile our shader program
        // Vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // Check for compile time errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n", infoLog);
            return 0;
        }
        // Fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // Check for compile time errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n", infoLog);
            return 0;
        }
        // Link shaders
        shader_prog_ = glCreateProgram();
        glAttachShader(shader_prog_, vertexShader);
        glAttachShader(shader_prog_, fragmentShader);
        glLinkProgram(shader_prog_);
        // Check for linking errors
        glGetProgramiv(shader_prog_, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_prog_, 512, NULL, infoLog);
            printf("ERROR::PROGRAM::LINK_FAILED: %s\n", infoLog);
            return 0;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return true;
    }
    void use() {
        glUseProgram(shader_prog_);
    }
    void set_transform(const glm::mat4& xform) {
        unsigned int xform_uniform = glGetUniformLocation(shader_prog_, "xform");
        glUniformMatrix4fv(xform_uniform, 1, GL_FALSE, glm::value_ptr(xform));
    }
private:
    GLuint shader_prog_;
};
