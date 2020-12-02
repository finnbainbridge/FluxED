#version 300 es
layout (location = 0) in vec3 pos;

uniform mat4 model_view;

void main()
{
    gl_Position = model_view * vec4(pos, 1.0f);
}