#version 300 es
precision highp float;
out vec4 FragColor;

in vec2 tex_coord;
in vec4 normal;
in vec4 world_pos;
in mat3 tbn;

// MUST be in alphabetical order
layout (std140) uniform Material 
{
    vec3 color;
    int has_diffuse;
    int has_metal_map;
    int has_normal_map;
    int has_roughness_map;

    float metallic;
    float roughness;
};

layout (std140) uniform Lights
{
    vec3 light_positions[128];
    vec3 light_directions[128];
    vec3 light_colors[128];
    vec3 light_infos[128];

};

// TODO: Put as uniforms in Material
// float metallic = 0.6;
// float roughness = 0.2;
// float ao = 0.0;

// TODO: Support multiple lights
// Position is global
// vec3 light_position = vec3(0, 0, 8);
// vec3 light_color = vec3(1, 1, 1);
// float lightRadius = 10.0;

uniform int light_indexes[8];

uniform vec3 cam_pos;

uniform sampler2D diffuse_texture;
uniform sampler2D metal_map;
uniform sampler2D normal_map;
uniform sampler2D roughness_map;

const float PI = 3.14159265359;

// Random but important equations
// I'm not even going to pretend to know how PBR lighting works
// Thanks so, so much to Joey DeVries
// https://learnopengl.com/PBR/Lighting
vec3 fresnelSchlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cos_theta, 0.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness_p)
{
    float a = roughness_p * roughness_p;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness_p)
{
    float r = (roughness_p + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness_p)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness_p);
    float ggx1 = geometrySchlickGGX(NdotL, roughness_p);

    return ggx1 * ggx2;
}

/*
Light Infos: Vec3(enum light type, float radius, float light_infos.z)
Types:
0 - Point light
1 - Directional light
2 - Spot light
*/
vec3 reflectanceEquation(vec3 N, vec3 V, vec3 F0, vec3 diffuse, vec3 light_pos, 
                        vec3 light_color, vec3 light_infos, vec3 light_direction,
                        float roughness_p, float metal_p)
{
    // Calculate per light radiance
    vec3 L;
    if (light_infos.x == 1.0)
    {
        L = normalize(-light_direction);
    }
    else
    {
        L = normalize(light_pos - world_pos.xyz);
    }

    vec3 H = normalize(V + L);
    float distance = length(light_pos - world_pos.xyz);
    // float attenuation = 1.0 / (distance * distance);
    vec3 radiance;
    if (light_infos.x == 0.0)
    {
        float attenuation = pow(1.0-pow((distance/light_infos[1]),4.0),2.0)/distance*distance+1.0;
        radiance = light_color * attenuation;
    }
    else if (light_infos.x == 2.0)
    {
        float theta = dot(L, normalize(-light_direction));
        float inner_cutoff = (light_infos.z * 1.1);
        float epsilon = inner_cutoff - light_infos.z;
        float attenuation = clamp((theta - light_infos.z) / epsilon, 0.0, 1.0);
        radiance = light_color * attenuation;
    }
    else
    {
        radiance = light_color;
    }

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness_p);
    float G = geometrySmith(N, V, L, roughness_p);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal_p;

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

    vec3 N;
    if (has_normal_map == 1)
    {
        N = texture(normal_map, tex_coord).xyz;

        // Make the normal map work properly
        N = normalize(N * 2.0 - 1.0);
        N = normalize(tbn * N);

        // TODO: Do stuff in vertex shader or something. See https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    }
    else
    {
        N = normal.xyz;
    }

    float roughness_p;
    if (has_roughness_map == 1)
    {
        roughness_p = texture(roughness_map, tex_coord).x;
    }
    else
    {
        roughness_p = roughness;
    }

    float metal_p;
    if (has_metal_map == 1)
    {
        metal_p = texture(metal_map, tex_coord).x;
    }
    else
    {
        metal_p = metallic;
    }

    vec3 V = normalize(cam_pos - world_pos.xyz);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, diffuse.xyz, metal_p);

    // Reflectance equation
    vec3 Lo = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < 8; i++)
    {
        if (light_indexes[i] != -1)
        {
            Lo += reflectanceEquation(N, V, F0, diffuse.xyz, light_positions[light_indexes[i]], light_colors[light_indexes[i]], light_infos[light_indexes[i]],
                    light_directions[light_indexes[i]],
                    // 0.5, 0.2);
                    roughness_p, metal_p);
            // Lo += vec3(0.5, 0, 0);
        }
    }

    vec3 color = pow(Lo/(Lo + 1.0), vec3(1.0/2.2));

    FragColor = vec4(color, 1);
    // FragColor = diffuse;
    // FragColor = vec4(1, 0, 0, 1);
}