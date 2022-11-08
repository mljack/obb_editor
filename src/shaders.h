#pragma once

const char* vertex_shader_src = R"(#version 300 es
in vec3 position;
in vec4 inputColor;
in vec2 aTexCoord;

uniform mat4 xform;

out vec4 color;
out vec2 TexCoord;
void main() {
    gl_Position = xform * vec4(position, 1.0);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    color = inputColor;
}
)";

const char* fragment_shader_src = R"(#version 300 es
precision mediump float;
out vec4 fragColor;

in vec4 color;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    fragColor = color * texture(texture1, TexCoord); 
}
)";

const char* vertex_shader_src2 = R"(#version 300 es
in vec3 position;
in vec4 inputColor;

uniform mat4 xform;

out vec4 color;
void main() {
    gl_Position = xform * vec4(position, 1.0);
    color = inputColor;
}
)";

const char* fragment_shader_src2 = R"(#version 300 es
precision mediump float;
out vec4 fragColor;

in vec4 color;
void main() {
    fragColor = color; 
}
)";