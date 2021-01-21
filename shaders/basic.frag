#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 tex_coord;

layout (std140) uniform Material 
{
    vec3 color;
};

void main()
{

    FragColor = vec4(color.x, color.y, color.z, 1.0f);
    // FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}