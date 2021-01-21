#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Resources.hh"
#include "FluxArc/FluxArc.hh"
#include "FluxProj/FluxProj.hh"
#include <cstring>
#include <filesystem>

#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <map>
#include <string>
#include <queue>

#include "Flux/Renderer.hh"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

static std::vector<Flux::Resources::ResourceRef<Flux::Resources::Resource>> resources;
static std::vector<Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes>> materials;
static Flux::ECSCtx* ctx = nullptr;
static std::map<std::string, Flux::Resources::ResourceRef<Flux::Resources::Resource>> mesh_ids;

static std::vector<Flux::Resources::ResourceRef<Flux::Renderer::TextureRes>> internal_textures;
static std::map<std::string, Flux::Resources::ResourceRef<Flux::Renderer::TextureRes>> external_textures;

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
        auto res = Flux::Resources::createResource(meshres);
        // auto id = ser.addResource(Flux::Resources::ResourceRef<Flux::Resources::Resource>(res.getBaseEntity()));

        mesh_ids[std::string(mesh->mName.C_Str(), mesh->mName.length)] = Flux::Resources::ResourceRef<Flux::Resources::Resource>(res.getBaseEntity());
    }
    return true;
}

bool processMaterials(std::filesystem::path input, Flux::Resources::Serializer& ser, const aiScene* scene, bool release)
{
    // Finding textures
    // Internal textures
    for (int i = 0; i < scene->mNumTextures; i++)
    {
        auto tex = scene->mTextures[i];
        std::vector<aiTexel> pixels(tex->pcData, tex->pcData + tex->mWidth * tex->mHeight);
        std::vector<aiTexel> flipped_pixels;
        flipped_pixels.resize(pixels.size());

        for (int i = pixels.size(); i > 0; i -= tex->mWidth)
        {
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
        ser.addResource(tx_res.getBaseEntity());
        internal_textures.push_back(tx_res);
    }

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
                    ser.addResource(tx_res.getBaseEntity());

                    external_textures[name] = tx_res;
                }

                Flux::Renderer::setUniform(mat_res, "diffuse_texture", external_textures[name]);
            }
        }
        else
        {
            Flux::Renderer::setUniform(mat_res, "has_diffuse", false);
        }

        ser.addResource(mat_res.getBaseEntity());
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

                auto new_entity = ctx->createNamedEntity(mesh->mName.C_Str());

                Flux::Renderer::addMesh(new_entity, resource, mat);

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
}

bool assimpImporter(std::filesystem::path input, std::filesystem::path output, bool release)
{
    Assimp::Importer importer;

    const struct aiScene* scene = importer.ReadFile(input, aiProcess_CalcTangentSpace |
                aiProcess_Triangulate            |
                aiProcess_JoinIdenticalVertices  |
                // aiProcess_FlipUVs                |
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
    Flux::Resources::Serializer ser;
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
    for (auto i : resources)
    {
        Flux::Resources::removeResource(i);
    }
    ctx->destroyAllEntities();

    return true;
}

static bool res = FluxProj::addExtension(".obj", assimpImporter);
static bool res2 = FluxProj::addExtension(".gltf", assimpImporter);
static bool res3 = FluxProj::addExtension(".glb", assimpImporter);
static bool res4 = FluxProj::addExtension(".fbx", assimpImporter);
static bool res5 = FluxProj::addExtension(".dae", assimpImporter);