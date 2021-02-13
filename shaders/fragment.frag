#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 tex_coord;
in vec4 normal;
in vec4 world_pos;

layout (std140) uniform Material 
{
    vec3 color;
    int has_diffuse;
};

layout (std140) uniform Lights
{
    vec3 light_positions[128];
    vec3 light_colors[128];
    float light_radii[128];
};

// TODO: Put as uniforms in Material
float metallic = 0.6;
float roughness = 0.2;
// float ao = 0.0;

// TODO: Support multiple lights
// Position is global
// vec3 light_position = vec3(0, 0, 8);
// vec3 light_color = vec3(1, 1, 1);
// float lightRadius = 10.0;

uniform int light_indexes[8];

uniform vec3 cam_pos;

uniform sampler2D diffuse_texture;

const float PI = 3.14159265359;

// Random but important equations
// I'm not even going to pretend to know how PBR lighting works
// Thanks so, so much to Joey DeVries
// https://learnopengl.com/PBR/Lighting
vec3 fresnelSchlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cos_theta, 0.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 reflectanceEquation(vec3 N, vec3 V, vec3 F0, vec3 diffuse, vec3 light_pos, vec3 light_color, float light_radius)
{
    // Calculate per light radiance
    vec3 L = normalize(light_pos - world_pos.xyz);
    vec3 H = normalize(V + L);
    float distance = length(light_pos - world_pos.xyz);
    // float attenuation = 1.0 / (distance * distance);
    float attenuation = pow(1.0-pow((distance/light_radius),4.0),2.0)/distance*distance+1.0;
    vec3 radiance = light_color * attenuation;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    // Add to outgoing radience Lo
    float NdotL = max(dot(N, L), 0.0);
    return (kD * diffuse / PI + specular) * radiance * NdotL;
}

void main()
{
    vec4 diffuse;
    if (has_diffuse == 1)
    {
        diffuse = texture(diffuse_texture, tex_coord);
    }
    else
    {
        diffuse = vec4(color.x, color.y, color.z, 1.0f);
    }
    
    vec3 N = normal.xyz;
    vec3 V = normalize(cam_pos - world_pos.xyz);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, diffuse.xyz, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < 8; i++)
    {
        if (i != -1)
        {
            Lo += reflectanceEquation(N, V, F0, diffuse.xyz, light_positions[light_indexes[i]], light_colors[light_indexes[i]], light_radii[light_indexes[i]]);
        }
    }

    vec3 color = pow(Lo/(Lo + 1.0), vec3(1.0/2.2));

    // Preserve the alpha from the material
    FragColor = vec4(color, diffuse.w);
}