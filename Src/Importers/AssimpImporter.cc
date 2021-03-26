#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Physics.hh"
#include "Flux/Resources.hh"
#include "FluxArc/FluxArc.hh"
#include "FluxProj/FluxProj.hh"
#include <cstring>
#include <filesystem>

#include <stb/stb_image.h>

#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <assimp/pbrmaterial.h>
#include <map>
#include <string>
#include <queue>

#include "Flux/Renderer.hh"
#include "assimp/light.h"
#include "assimp/material.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

static std::vector<Flux::Resources::ResourceRef<Flux::Resources::Resource>> resources;
static std::vector<Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes>> materials;
static Flux::ECSCtx* ctx = nullptr;
static std::map<std::string, Flux::Resources::ResourceRef<Flux::Resources::Resource>> mesh_ids;

static std::vector<Flux::Resources::ResourceRef<Flux::Renderer::TextureRes>> internal_textures;
static std::map<std::string, Flux::Resources::ResourceRef<Flux::Renderer::TextureRes>> external_textures;

// TODO: Destroy this entire importer
// It has too many weird, strange problems,
// Bad PBR support,
// And it's extremely slow.

// Generate a hopefully unique id from a string
uint32_t textToIHID(const std::string& text)
{
    // For now, just add up the characters
    uint32_t out = 1024;
    for (auto i : text)
    {
        out += i;
    }

    return out;
}

bool processMeshes(Flux::Resources::Serializer& ser, const aiScene* scene, bool release)
{
    if (!scene->HasMeshes())
    {
        LOG_WARN("Warning: File has no meshes");
        return true;
    }

    for (int i = 0; i < scene->mNumMeshes; i++)
    {
        Flux::Renderer::MeshRes* meshres = new Flux::Renderer::MeshRes;
        auto mesh = scene->mMeshes[i];

        meshres->vertices_length = mesh->mNumVertices;
        meshres->vertices = new Flux::Renderer::Vertex[meshres->vertices_length];

        for (int j = 0; j < mesh->mNumVertices; j++)
        {
            Flux::Renderer::Vertex vert;
            vert.x = mesh->mVertices[j].x;
            vert.y = mesh->mVertices[j].y;
            vert.z = mesh->mVertices[j].z;

            vert.nx = mesh->mNormals[j].x;
            vert.ny = mesh->mNormals[j].y;
            vert.nz = mesh->mNormals[j].z;

            if (mesh->mTangents == nullptr)
            {
                vert.tanx = 0;
                vert.tany = 0;
                vert.tanz = 0;

                vert.btanx = 0;
                vert.btany = 0;
                vert.btanz = 0;
            }
            else
            {
                vert.tanx = mesh->mTangents[j].x;
                vert.tany = mesh->mTangents[j].y;
                vert.tanz = mesh->mTangents[j].z;

                vert.btanx = mesh->mBitangents[j].x;
                vert.btany = mesh->mBitangents[j].y;
                vert.btanz = mesh->mBitangents[j].z;
            }
            

            // TODO: Deal with multiple UV layers
            if (mesh->mTextureCoords[0] != nullptr)
            {
                vert.tx = mesh->mTextureCoords[0][j].x;
                vert.ty = mesh->mTextureCoords[0][j].y;
            }
            else
            {
                vert.tx = 0;
                vert.ty = 0;
            }

            meshres->vertices[j] = vert;
        }

        // Do size after, so we actually know what it is
        std::vector<int> indices;
        for (int j = 0; j < mesh->mNumFaces; j++)
        {
            for (int k = 0; k < mesh->mFaces[j].mNumIndices; k++)
            {
                indices.push_back(mesh->mFaces[j].mIndices[k]);
            }
        }

        // Add to meshres
        meshres->indices_length = indices.size();
        meshres->indices = new uint32_t[meshres->indices_length];

        meshres->draw_mode = Flux::Renderer::DrawMode::Triangles;
        
        for (int j = 0; j < meshres->indices_length; j++) meshres->indices[j] = indices[j];

        // Add to resource system, and serializer
        auto str = "msh" + std::string(mesh->mName.C_Str());
        auto res = Flux::Resources::createResource(meshres);
        ser.addResource(Flux::Resources::ResourceRef<Flux::Resources::Resource>(res.getBaseEntity()), textToIHID(str));

        mesh_ids[std::string(mesh->mName.C_Str(), mesh->mName.length)] = Flux::Resources::ResourceRef<Flux::Resources::Resource>(res.getBaseEntity());
    }
    return true;
}

Flux::Resources::ResourceRef<struct Flux::Renderer::TextureRes> createTexture(std::filesystem::path input, std::string name)
{
    // Create new texture
    auto tex_res = new Flux::Renderer::TextureRes;
    tex_res->internal = false;

    std::filesystem::path og = name;
    std::filesystem::path new_g;
    if (og.is_absolute() == true)
    {
        // Why do programs do this?
        // Move the textures here
        if (!std::filesystem::exists(input.parent_path() / (input.filename().string() + "_external_textures")))
        {
            std::filesystem::create_directory(input.parent_path() / (input.filename().string() + "_external_textures"));
        }

        if (std::filesystem::exists(input.parent_path() / (input.filename().string() + "_external_textures") / og.filename()))
        {
            // Delete it
            std::filesystem::remove(input.parent_path() / (input.filename().string() + "_external_textures") / og.filename());
        }

        std::filesystem::copy_file(og, input.parent_path() / (input.filename().string() + "_external_textures") / og.filename());
        new_g = input.parent_path() / (input.filename().string() + "_external_textures") / og.filename();
    }
    else
    {
        // Nice people who use relative paths get
        // to keep their own folder structure
        new_g = input.parent_path() / og;
    }

    // Use absolute path for loading
    tex_res->loadImage(new_g);
    
    // Manually set relative path for saving
    // It is relative to the OUTPUT, not the PROJECT
    tex_res->filename = std::filesystem::relative(new_g, input.parent_path());
    auto tx_res = Flux::Resources::createResource(tex_res);
    
    return tx_res;
}

bool processMaterials(std::filesystem::path input, Flux::Resources::Serializer& ser, const aiScene* scene, bool release)
{
    // Finding textures
    // Internal textures
    for (int i = 0; i < scene->mNumTextures; i++)
    {
        auto tex = scene->mTextures[i];
        
        if (tex->mHeight == 0)
        {
            // It's a normal image file dumped into the model
            // Use stb_image to decode it
            int x, y, channels;

            auto data = (unsigned char*)stbi_load_from_memory((unsigned char*)tex->pcData, tex->mWidth, &x, &y, &channels, 4);

            if (!data)
            {
                LOG_ERROR("Failed to load internal image :(");
            }

            auto tex_res = new Flux::Renderer::TextureRes;
            tex_res->width = x;
            tex_res->height = y;

            tex_res->internal = true;
            tex_res->image_data_size = sizeof(unsigned char) * 4 * x * y;
            tex_res->image_data = data;

            auto tx_res = Flux::Resources::createResource(tex_res);
            auto str = "tex" + std::string(tex->mFilename.C_Str());
            ser.addResource(tx_res.getBaseEntity(), textToIHID(str));
            internal_textures.push_back(tx_res);
        }
        else
        {
            std::vector<aiTexel> pixels(tex->pcData, tex->pcData + tex->mWidth * tex->mHeight);
            std::vector<aiTexel> flipped_pixels;
            flipped_pixels.resize(pixels.size());

            for (int i = pixels.size(); i > 0; i -= tex->mWidth)
            {
                // TODO: Convert ARGB (aiTexel) to RGBA
                flipped_pixels.insert(flipped_pixels.end(), pixels.begin() + (i - tex->mWidth), pixels.begin() + i);
            }

            // Add to resource
            auto tex_res = new Flux::Renderer::TextureRes;
            tex_res->width = tex->mWidth;
            tex_res->height = tex->mHeight;

            tex_res->internal = true;
            tex_res->image_data_size = sizeof(aiTexel) * flipped_pixels.size();
            tex_res->image_data = new unsigned char[tex_res->image_data_size];
            std::memcpy(tex_res->image_data, &flipped_pixels[0], tex_res->image_data_size);

            auto tx_res = Flux::Resources::createResource(tex_res);
            auto str = "tex" + std::string(tex->mFilename.C_Str());
            ser.addResource(tx_res.getBaseEntity(), textToIHID(str));
            internal_textures.push_back(tx_res);
        }
        
    }

    // Add dummy texture
    auto dummy_texture_res = new Flux::Renderer::TextureRes;
    dummy_texture_res->width = 1;
    dummy_texture_res->height = 1;

    dummy_texture_res->internal = true;
    dummy_texture_res->image_data_size = sizeof(unsigned char) * 4;
    dummy_texture_res->image_data = (unsigned char *)(new unsigned char[4] {1, 0, 0, 1});
    auto dummy_texture = Flux::Resources::createResource(dummy_texture_res);
    auto str = "tex::dummy";
    ser.addResource(dummy_texture.getBaseEntity(), textToIHID(str));

    // TODO: Don't hard code this
    // TODO: Even more. 
    auto shaders_res = Flux::Renderer::createShaderResource("./shaders/vertex.vert", "./shaders/fragment.frag");
    // auto mat_res = Flux::Renderer::createMaterialResource(shaders_res);

    for (int i = 0; i < scene->mNumMaterials; i++)
    {
        auto mat = scene->mMaterials[i];
        aiColor3D color (0.f,0.f,0.f);
        
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE,color) != aiReturn_SUCCESS)
        {
            // Error. We don't really care, though, since the color will
            // still be black
        }

        auto mat_res = Flux::Renderer::createMaterialResource(shaders_res);
        Flux::Renderer::setUniform(mat_res, "color", glm::vec3(color.r, color.g, color.b));

        // Deal with textures
        // TODO: Multiple textures
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            Flux::Renderer::setUniform(mat_res, "has_diffuse", true);
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            std::string name = path.C_Str();
            
            if (name[0] == '*')
            {
                // Embeded texture
                int index = std::stoi(name.substr(1));
                Flux::Renderer::setUniform(mat_res, "diffuse_texture", internal_textures[index]);
            }
            else
            {
                // External texture
                if (external_textures.find(name) == external_textures.end())
                {
                    auto tx_res = createTexture(input, name);
                    auto str = "tex" + std::string(tx_res->filename);
                    ser.addResource(tx_res.getBaseEntity(), textToIHID(str));

                    external_textures[name] = tx_res;
                }

                Flux::Renderer::setUniform(mat_res, "diffuse_texture", external_textures[name]);
            }
        }
        else
        {
            Flux::Renderer::setUniform(mat_res, "has_diffuse", false);
            Flux::Renderer::setUniform(mat_res, "diffuse_texture", dummy_texture);
        }

        // Check for various PBR materials
        float roughness = 0.5;
        float metallic = 0.2;
        if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, roughness) != aiReturn_SUCCESS)
        {
            // Not GLTF :(
            // That makes everything more annoying
        }

        if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, roughness) != aiReturn_SUCCESS)
        {
            // Not GLTF :(
            // That makes everything more annoying
        }

        Flux::Renderer::setUniform(mat_res, "roughness", roughness);
        Flux::Renderer::setUniform(mat_res, "metallic", metallic);

        // Get Roughness, Normal, and metallic textures
        if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            Flux::Renderer::setUniform(mat_res, "has_normal_map", true);
            aiString path;
            mat->GetTexture(aiTextureType_NORMALS, 0, &path);
            std::string name = path.C_Str();
            
            if (name[0] == '*')
            {
                // Embeded texture
                int index = std::stoi(name.substr(1));
                Flux::Renderer::setUniform(mat_res, "normal_map", internal_textures[index]);
            }
            else
            {
                // External texture
                if (external_textures.find(name) == external_textures.end())
                {
                    auto tx_res = createTexture(input, name);
                    auto str = "nmtex" + std::string(tx_res->filename);
                    ser.addResource(tx_res.getBaseEntity(), textToIHID(str));

                    external_textures[name] = tx_res;
                }

                Flux::Renderer::setUniform(mat_res, "normal_map", external_textures[name]);
            }
        }
        else
        {
            Flux::Renderer::setUniform(mat_res, "has_normal_map", false);
            Flux::Renderer::setUniform(mat_res, "normal_map", dummy_texture);
        }

        if (mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
        {
            Flux::Renderer::setUniform(mat_res, "has_roughness_map", true);
            aiString path;
            mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path);
            std::string name = path.C_Str();
            
            if (name[0] == '*')
            {
                // Embeded texture
                int index = std::stoi(name.substr(1));
                Flux::Renderer::setUniform(mat_res, "roughness_map", internal_textures[index]);
            }
            else
            {
                // External texture
                if (external_textures.find(name) == external_textures.end())
                {
                    auto tx_res = createTexture(input, name);
                    auto str = "rmtex" + std::string(tx_res->filename);
                    ser.addResource(tx_res.getBaseEntity(), textToIHID(str));

                    external_textures[name] = tx_res;
                }

                Flux::Renderer::setUniform(mat_res, "roughness_map", external_textures[name]);
            }
        }
        else
        {
            Flux::Renderer::setUniform(mat_res, "has_roughness_map", false);
            Flux::Renderer::setUniform(mat_res, "roughness_map", dummy_texture);
        }

        if (mat->GetTextureCount(aiTextureType_METALNESS) > 0)
        {
            Flux::Renderer::setUniform(mat_res, "has_metal_map", true);
            aiString path;
            mat->GetTexture(aiTextureType_METALNESS, 0, &path);
            std::string name = path.C_Str();
            
            if (name[0] == '*')
            {
                // Embeded texture
                int index = std::stoi(name.substr(1));
                Flux::Renderer::setUniform(mat_res, "metal_map", internal_textures[index]);
            }
            else
            {
                // External texture
                if (external_textures.find(name) == external_textures.end())
                {
                    auto tx_res = createTexture(input, name);
                    auto str = "rmtex" + std::string(tx_res->filename);
                    ser.addResource(tx_res.getBaseEntity(), textToIHID(str));

                    external_textures[name] = tx_res;
                }

                Flux::Renderer::setUniform(mat_res, "metal_map", external_textures[name]);
            }
        }
        else
        {
            Flux::Renderer::setUniform(mat_res, "has_metal_map", false);
            Flux::Renderer::setUniform(mat_res, "metal_map", dummy_texture);
        }

        auto x = mat_res->uniforms;

        std::string str(mat->GetName().C_Str());
        str = "mat" + str;
        ser.addResource(mat_res.getBaseEntity(), textToIHID(str));
        materials.push_back(mat_res);
    }

    return true;
}

struct ToProcess
{
    aiNode* node;
    Flux::EntityRef entity;
};

void createScene(const std::string& fname, Flux::Resources::Serializer& ser, const aiScene* scene, bool release)
{
    std::queue<ToProcess> nodes;
    auto e = ctx->createNamedEntity(fname);
    Flux::Transform::TransformCom* f = new Flux::Transform::TransformCom;
    f->has_parent = false;
    f->visible = true;
    e.addComponent(f);
    nodes.emplace(ToProcess {scene->mRootNode, e});

    while (nodes.size() != 0)
    {
        ToProcess node_c = nodes.front();
        nodes.pop();

        auto node = node_c.node;
        auto entity = node_c.entity;

        aiVector3t scaling(0.0f);
        aiVector3t rot_axis(0.0f);
        aiVector3t position(0.0f);
        float rot_angle;

        node->mTransformation.Decompose(scaling, rot_axis, rot_angle, position);

        float aaa[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };

        // Add transform
        glm::mat4 transform = glm::make_mat4(aaa);
        transform = glm::translate(transform, glm::vec3(position.x, position.y, position.z));

        // Predictably, rotating around an axis that doesn't exist causes major problems
        if (!(rot_axis.x == 0 && rot_axis.y == 0 && rot_axis.z == 0))
        {
            transform = glm::rotate(transform, rot_angle, glm::vec3(rot_axis.x, rot_axis.y, rot_axis.z));
        }
        
        transform = glm::scale(transform, glm::vec3(scaling.x, scaling.y, scaling.z));
        entity.getComponent<Flux::Transform::TransformCom>()->transformation = transform;
        ser.addEntity(entity);

        if (node->mNumMeshes > 0)
        {
            // Create a new entity for each mesh
            // TODO: Maybe change this later...?
            for (int i = 0; i < node->mNumMeshes; i++)
            {
                auto mesh = scene->mMeshes[node->mMeshes[i]];
                auto mat = materials[mesh->mMaterialIndex];
                auto resource = mesh_ids[std::string(mesh->mName.C_Str(), mesh->mName.length)];

                auto new_entity = ctx->createNamedEntity(std::string(mesh->mName.C_Str()));

                Flux::Renderer::addMesh(new_entity, resource, mat);

                // Add Bounding Box
                Flux::Physics::giveBoundingBox(new_entity);

                Flux::Transform::setParent(new_entity, entity);
                ser.addEntity(new_entity);
            }
        }

        for (int i = 0; i < node->mNumChildren; i++)
        {
            auto new_entity = ctx->createNamedEntity(node->mChildren[i]->mName.C_Str());
            Flux::Transform::TransformCom* f = new Flux::Transform::TransformCom;
            f->has_parent = false;
            f->visible = true;
            new_entity.addComponent(f);
            Flux::Transform::setParent(new_entity, entity);
            nodes.emplace(ToProcess {node->mChildren[i], new_entity});
        }
    }

    // Add lights
    // Unfortunately, Assimp's light system sucks
    // I will have to write my own importer at some point soon
    for (int i = 0; i < scene->mNumLights; i++)
    {
        auto light = scene->mLights[i];
        auto new_entity = ctx->createNamedEntity(light->mName.C_Str());
        
        Flux::Transform::giveTransform(new_entity);
        Flux::Transform::translate(new_entity, glm::vec3(light->mPosition.x, light->mPosition.y, light->mPosition.z));
        // aiLightSourceType::
        Flux::Renderer::addPointLight(new_entity,light->mSize.Length(), glm::vec3(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b));

        ser.addEntity(new_entity);
    }
}

bool assimpImporter(std::filesystem::path input, std::filesystem::path output, bool release)
{
    Assimp::Importer importer;

    const struct aiScene* scene = importer.ReadFile(input, aiProcess_CalcTangentSpace |
                aiProcess_Triangulate            |
                aiProcess_JoinIdenticalVertices  |
                // aiProcess_FlipUVs                |
                aiProcess_CalcTangentSpace       |
                aiProcess_GenNormals             |
                aiProcess_OptimizeMeshes         | // This may break things
                aiProcess_SortByPType);
    
    // Make sure it worked
    if (!scene)
    {
        LOG_ERROR("Assimp import failed: " + std::string(importer.GetErrorString()));
        return false;
    }

    // Create containers
    resources = std::vector<Flux::Resources::ResourceRef<Flux::Resources::Resource>>();
    ctx = new Flux::ECSCtx();

    // Load Meshes into archive
    Flux::Resources::Serializer ser(output);
    processMeshes(ser, scene, release);

    // Load Materials
    materials = std::vector<Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes>>();
    internal_textures.clear();
    external_textures.clear();
    processMaterials(input, ser, scene, release);

    // Create scene
    createScene(input.replace_extension("").filename(), ser, scene, release);

    // Save
    FluxArc::Archive file(output);
    ser.save(file, release);

    // Cleanup
    // for (auto i : resources)
    // {
    //     Flux::Resources::removeResource(i);
    // }
    ctx->destroyAllEntities();

    return true;
}

static bool res = FluxProj::addExtension(".obj", assimpImporter);
static bool res2 = FluxProj::addExtension(".gltf", assimpImporter);
static bool res3 = FluxProj::addExtension(".glb", assimpImporter);
static bool res4 = FluxProj::addExtension(".fbx", assimpImporter);
static bool res5 = FluxProj::addExtension(".dae", assimpImporter);