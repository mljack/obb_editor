#pragma once

const char* vertexShaderSource = R"(#version 300 es
in vec3 position;
in vec4 inputColor;
in vec2 aTexCoord;

uniform mat4 transform;

out vec4 color;
out vec2 TexCoord;
void main() {
    gl_Position = transform * vec4(position, 1.0);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    color = inputColor;
}
)";


const char* fragmentShaderSource = R"(#version 300 es
precision mediump float;
out vec4 fragColor;

in vec4 color;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    fragColor = color * texture(texture1, TexCoord); 
}
)";