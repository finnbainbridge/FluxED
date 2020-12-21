#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Resources.hh"
#include "FluxArc/FluxArc.hh"
#include "FluxProj/FluxProj.hh"
#include <filesystem>

#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <map>
#include <string>
#include <queue>

#include "Flux/Renderer.hh"
#include "glm/gtc/type_ptr.hpp"

static std::vector<Flux::Resources::ResourceRef<Flux::Resources::Resource>> resources;
static Flux::ECSCtx ctx;
static std::map<std::string, Flux::Resources::ResourceRef<Flux::Resources::Resource>> mesh_ids;

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
        
        for (int j = 0; j < meshres->indices_length; j++) meshres->indices[j] = indices[j];

        // Add to resource system, and serializer
        auto res = Flux::Resources::createResource(meshres);
        auto id = ser.addResource(Flux::Resources::ResourceRef<Flux::Resources::Resource>(res.getBaseEntity()));

        mesh_ids[std::string(mesh->mName.C_Str(), mesh->mName.length)] = Flux::Resources::ResourceRef<Flux::Resources::Resource>(res.getBaseEntity());
    }
    return true;
}

struct ToProcess
{
    aiNode* node;
    Flux::EntityRef entity;
};

void createScene(Flux::Resources::Serializer& ser, const aiScene* scene, bool release)
{
    std::queue<ToProcess> nodes;
    auto e = ctx.createEntity();
    Flux::Transform::TransformCom* f = new Flux::Transform::TransformCom;
    f->has_parent = false;
    e.addComponent(f);
    nodes.emplace(ToProcess {scene->mRootNode, e});

    // TODO: Don't hard code this
    auto shaders_res = Flux::Renderer::createShaderResource("./shaders/vertex.vert", "./shaders/fragment.frag");
    auto mat_res = Flux::Renderer::createMaterialResource(shaders_res);

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
                auto resource = mesh_ids[std::string(mesh->mName.C_Str(), mesh->mName.length)];

                auto new_entity = ctx.createEntity();

                // TODO: Materials
                Flux::Renderer::addMesh(new_entity, resource, mat_res);

                Flux::Transform::setParent(new_entity, entity);
                ser.addEntity(new_entity);
            }
        }

        for (int i = 0; i < node->mNumChildren; i++)
        {
            auto new_entity = ctx.createEntity();
            Flux::Transform::TransformCom* f = new Flux::Transform::TransformCom;
            f->has_parent = false;
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
                aiProcess_SortByPType);
    
    // Make sure it worked
    if (!scene)
    {
        LOG_ERROR("Assimp import failed: " + std::string(importer.GetErrorString()));
        return false;
    }

    // Create containers
    resources = std::vector<Flux::Resources::ResourceRef<Flux::Resources::Resource>>();
    ctx = Flux::ECSCtx();

    // Load Meshes into archive
    Flux::Resources::Serializer ser;
    processMeshes(ser, scene, release);

    // Create scene
    createScene(ser, scene, release);

    // Save
    FluxArc::Archive file(output);
    ser.save(file, release);

    // Cleanup
    for (auto i : resources)
    {
        Flux::Resources::removeResource(i);
    }
    ctx.destroyAllEntities();

    return true;
}

static bool res = FluxProj::addExtension(".obj", assimpImporter);
static bool res2 = FluxProj::addExtension(".gltf", assimpImporter);
static bool res3 = FluxProj::addExtension(".glb", assimpImporter);
static bool res4 = FluxProj::addExtension(".fbx", assimpImporter);
static bool res5 = FluxProj::addExtension(".dae", assimpImporter);