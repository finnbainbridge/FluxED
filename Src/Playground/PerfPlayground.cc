#include "Flux/Physics.hh"
#include "FluxArc/FluxArc.hh"
#include <cstdio>
#include <cstdlib>
#define GLM_FORCE_CTOR_INIT

// #include <bits/stdint-uintn.h>
#include <iostream>
#include "Flux/ECS.hh"
#include "Flux/Flux.hh"
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include "Flux/Log.hh"
#include "Flux/Input.hh"
#include "glm/fwd.hpp"


Flux::ECSCtx ctx;

Flux::EntityRef quad;
Flux::EntityRef camera;
Flux::EntityRef cube_stick;

Flux::EntityRef light;
Flux::EntityRef light_stick;
int color = 0;
double click_timer = 0;

Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes> mat_res;

std::vector<Flux::EntityRef> meshes;

void init(int argc, char** argv) {
    
    ctx = Flux::ECSCtx();

    int c = 0;

    cube_stick = ctx.createEntity();
    Flux::Transform::giveTransform(cube_stick);

    for (int x = -24; x < 24; x+=4)
    {
        for (int y = -24; y < 24; y+=4)
        {
            for (int z = -24; z < 24; z+=4)
            {
                auto loader = Flux::Resources::deserialize("Presets/DefaultCube.farc");
                auto ens = loader->addToECS(&ctx);

                for (auto i : ens)
                {
                    if (i.hasComponent<Flux::Renderer::MeshCom>())
                    {
                        // Flux::Physics::giveBoundingBox(i);
                        Flux::Physics::giveConvexCollider(i);
                        c ++;
                        meshes.push_back(i);
                        Flux::Transform::removeParent(i);
                        Flux::Transform::setParent(i, cube_stick);
                        Flux::Transform::translate(i, glm::vec3(x, y, z));
                    }
                    else
                    {
                        ctx.destroyEntity(i);
                    }
                }
            }
        }
    }

    std::cout << "Added bounding box: " << c << "\n";

    camera = ctx.createEntity();
    Flux::Transform::giveTransform(camera);

    // camera_stick = ctx.createEntity();
    // Flux::Transform::giveTransform(camera_stick);

    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(0,0,10);
    Flux::Transform::translate(camera, o);
    LOG_INFO("Setup camera");

    // Flux::Transform::setParent(camera, camera_stick);

    // Flux::Renderer::addPointLight(camera, 30, glm::vec3(1, 1, 1));

    light = ctx.createEntity();
    Flux::Transform::giveTransform(light);

    light_stick = ctx.createEntity();
    Flux::Transform::giveTransform(light_stick);

    Flux::Transform::translate(light, o);
    LOG_INFO("Setup camera");

    Flux::Transform::setParent(light, light_stick);

    Flux::Renderer::addPointLight(camera, 80, glm::vec3(1, 1, 1));

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");
}

void loop(float delta)
{
    Flux::Transform::rotate(cube_stick, glm::vec3(0, 1, 0), 0.5 * delta);

    ctx.runSystems(delta);
}

void end()
{
    LOG_INFO("Ended...");
    ctx.destroyAllEntities();
}
