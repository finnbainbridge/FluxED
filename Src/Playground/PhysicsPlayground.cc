#include "Flux/Debug.hh"
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

Flux::EntityRef shapes[3];

void init(int argc, char** argv) {
    
    ctx = Flux::ECSCtx();

    int c = 0;

    cube_stick = ctx.createEntity();
    Flux::Transform::giveTransform(cube_stick);

    auto loader = Flux::Resources::deserialize("assets/Shapes.farc");
    auto ens = loader->addToECS(&ctx);

    int pos = -4;
    for (auto i : ens)
    {
        if (i.hasComponent<Flux::Renderer::MeshCom>())
        {
            // Flux::Physics::giveBoundingBox(i);
            Flux::Physics::giveConvexCollider(i);
            shapes[c] = i;
            c ++;
            meshes.push_back(i);
            Flux::Transform::removeParent(i);
            Flux::Transform::setParent(i, cube_stick);
            Flux::Transform::translate(i, glm::vec3(pos, 0, 0));
            pos += 4;
        }
        else
        {
            ctx.destroyEntity(i);
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

    Flux::Debug::enableDebugDraw(&ctx);
}

void loop(float delta)
{
    // Flux::Transform::rotate(cube_stick, glm::vec3(0, 1, 0), 0.5 * delta);

    if (Flux::Input::isKeyPressed(FLUX_KEY_LEFT))
    {
        Flux::Transform::translate(shapes[0], glm::vec3(-5 * delta, 0, 0));
    }

    if (Flux::Input::isKeyPressed(FLUX_KEY_RIGHT))
    {
        Flux::Transform::translate(shapes[0], glm::vec3(5 * delta, 0, 0));
    }

    if (Flux::Input::isKeyPressed(FLUX_KEY_UP))
    {
        Flux::Transform::translate(shapes[0], glm::vec3(0, 5 * delta, 0));
    }

    if (Flux::Input::isKeyPressed(FLUX_KEY_DOWN))
    {
        Flux::Transform::translate(shapes[0], glm::vec3(0, -5 * delta, 0));
    }

    {
        // auto cc = shapes[0].getComponent<Flux::Physics::ColliderCom>();
        auto collisions = Flux::Physics::getCollisions(shapes[0]);
        if (collisions.size() > 0)
        {
            Flux::Renderer::setUniform(shapes[0].getComponent<Flux::Renderer::MeshCom>()->mat_resource, "color", glm::vec3(1, 0, 0));
            // LOG_INFO("Hitting");

            // Show collision information
            auto data = collisions[0];
            auto endpoint = (glm::vec3(0, 5, 0) + data.normal) * data.depth;
            // Flux::Debug::drawLine(glm::vec3(0, 5, 0), endpoint, Flux::Debug::Colors::Purple);
            // Flux::Debug::drawPoint(endpoint, 0.15, Flux::Debug::Colors::Purple);

            // if (Flux::Input::isKeyPressed(FLUX_KEY_SPACE))
            // {
            Flux::Transform::translate(shapes[0], data.normal * data.depth);
            // }
        }
        else
        {
            Flux::Renderer::setUniform(shapes[0].getComponent<Flux::Renderer::MeshCom>()->mat_resource, "color", glm::vec3(0, 1, 0));
            // LOG_INFO("Not");
        }
    }

    // Flux::Debug::drawPoint(glm::vec3(0, 1.5, 0), 1, Flux::Debug::Colors::Red, true);
    // Flux::Debug::drawLine(glm::vec3(10, 2, 0), glm::vec3(-10, 2, 0), Flux::Debug::Colors::Purple);

    ctx.runSystems(delta);
    Flux::Debug::run(delta);
}

void end()
{
    LOG_INFO("Ended...");
    ctx.destroyAllEntities();
}
