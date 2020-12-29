#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 tex_coord;

layout (std140) uniform Material 
{
    vec3 color;
    int has_diffuse;
};

uniform sampler2D diffuse_texture;

void main()
{
    if (has_diffuse == 1)
    {
        FragColor = texture(diffuse_texture, tex_coord);
        // FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        FragColor = vec4(color.x, color.y, color.z, 1.0f);
    }
    // FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}