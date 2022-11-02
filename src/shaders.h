#pragma once

const char* vertexShaderSource = R"(#version 300 es
in vec4 position;
in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
   gl_Position = vec4(position.xyz, 1.0);
   TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
)";


const char* fragmentShaderSource = R"(#version 300 es
precision mediump float;
out vec4 fragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
   fragColor = texture(texture1, TexCoord);
}
)";