#version 300 es
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec2 tex_coord;
out vec4 normal;
out vec4 world_pos;

uniform mat4 model_view_projection;
uniform mat4 model_view;
uniform mat4 model;


void main()
{
    gl_Position = model_view_projection * vec4(pos, 1.0f);
    tex_coord = tex;
    // normal = normalize(model_view * vec4(norm, 1.0f));
    normal = vec4(normalize(norm), 1.0f);
    world_pos = model * vec4(pos, 1.0f);
}