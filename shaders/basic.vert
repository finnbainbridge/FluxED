#version 300 es
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec2 tex_coord;

uniform mat4 model_view_projection;


void main()
{
    gl_Position = model_view_projection * vec4(pos, 1.0f);
    tex_coord = tex;
}