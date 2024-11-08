#include "Flux/Debug.hh"
#include "Flux/Physics.hh"
#include "FluxArc/FluxArc.hh"
#include "glm/detail/func_common.hpp"
#include "glm/detail/type_vec.hpp"
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

Flux::EntityRef player;
Flux::EntityRef camera;
Flux::EntityRef cube_stick;
Flux::EntityRef physics_object;

Flux::EntityRef light;
Flux::EntityRef light_stick;
int color = 0;
double click_timer = 0;

Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes> mat_res;

std::vector<Flux::EntityRef> meshes;

// Flux::EntityRef shapes[3];

float randomFloat(float max)
{
    return float(rand())/float((RAND_MAX)) * max;
}

Flux::EntityRef createPhysicsCube()
{
    auto loader = Flux::Resources::deserialize("Presets/DefaultCube.farc");
    auto ens = loader->addToECS(&ctx);

    Flux::EntityRef phhysics_object;

    for (auto i : ens)
    {
        if (i.hasComponent<Flux::Renderer::MeshCom>())
        {
            // Flux::Physics::giveBoundingBox(i);
            Flux::Physics::giveConvexCollider(i);
            meshes.push_back(i);
            Flux::Transform::removeParent(i);
            // Flux::Transform::setParent(i, cube_stick);
            Flux::Transform::translate(i, glm::vec3(-3, 9, 0));

            // auto rc = new Flux::Physics::RigidCom;
            // rc->linear = glm::vec3(rand() % 4, -1.81, rand() % 4);
            // rc->angular = glm::vec3(1.0, 1.0, 1.0);
            // rc->mass = 1;
            // rc->velocity = glm::vec3();

            // i.addComponent(rc);
            Flux::Physics::giveRigidBody(i, 80);
            // auto rc = i.getComponent<Flux::Physics::RigidCom>();
            // rc->force = glm::vec3(0, -0.001, 0);
            // rc->torque = glm::vec3(0.001, 0, 0.001);
            Flux::Physics::addForce(i, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
            phhysics_object = i;
        }
        else
        {
            ctx.destroyEntity(i);
        }
    }
    return phhysics_object;
}


void init(int argc, char** argv)
{

    ctx = Flux::ECSCtx();

    int c = 0;

    // cube_stick = ctx.createEntity();
    // Flux::Transform::giveTransform(cube_stick);

    auto loader = Flux::Resources::deserialize("assets/Platforms.farc");
    auto ens = loader->addToECS(&ctx);
    Flux::Transform::rotate(ens[0], glm::vec3(0, 1, 0), glm::radians(90.0f));

    int pos = -4;
    for (auto i : ens)
    {
        if (i.hasComponent<Flux::Renderer::MeshCom>())
        {
            // Flux::Physics::giveBoundingBox(i);
            Flux::Physics::giveConvexCollider(i);
            // shapes[c] = i;
            c ++;
            meshes.push_back(i);
            // Flux::Transform::removeParent(i);
            // Flux::Transform::setParent(i, cube_stick);
            // Flux::Transform::translate(i, glm::vec3(pos, 0, 0));
            auto entity = ctx.createEntity();
            i.getComponent<Flux::Physics::BoundingCom>()->box->setDisplayEntity(entity);
            pos += 4;
            
        }
        else
        {
            // if (i.hasComponent<Flux::NameCom>())
            // {
            //     if (i.getComponent<Flux::NameCom>()->name == "Player_Cube.003")
            //     {
            //         ctx.destroyEntity(i);
            //     }
            // }
        }
    }

    srand(time(NULL));

    // for (int x = -8; x < 8; x+=4)
    // {
    //     for (int y = -8; y < 8; y+=4)
    //     {
    //         for (int z = -8; z < 8; z+=4)
    //         {
    //             auto loader = Flux::Resources::deserialize("Presets/DefaultCube.farc");
    //             auto ens = loader->addToECS(&ctx);

    //             for (auto i : ens)
    //             {
    //                 if (i.hasComponent<Flux::Renderer::MeshCom>())
    //                 {
    //                     // Flux::Physics::giveBoundingBox(i);
    //                     Flux::Physics::giveConvexCollider(i);
    //                     c ++;
    //                     meshes.push_back(i);
    //                     Flux::Transform::removeParent(i);
    //                     Flux::Transform::setParent(i, cube_stick);
    //                     Flux::Transform::translate(i, glm::vec3(x, y, z));

    //                     // auto rc = new Flux::Physics::RigidCom;
    //                     // rc->linear = glm::vec3(rand() % 4, -1.81, rand() % 4);
    //                     // rc->angular = glm::vec3(1.0, 1.0, 1.0);
    //                     // rc->mass = 1;
    //                     // rc->velocity = glm::vec3();

    //                     // i.addComponent(rc);
    //                     Flux::Physics::giveRigidBody(i, 1);
    //                     // auto rc = i.getComponent<Flux::Physics::RigidCom>();
    //                     // rc->force = glm::vec3(0, -0.001, 0);
    //                     // rc->torque = glm::vec3(0.001, 0, 0.001);
    //                     Flux::Physics::addForce(i, glm::vec3(x+randomFloat(0.25f), y+randomFloat(0.25f), z+randomFloat(0.25f)), glm::vec3(randomFloat(100), +randomFloat(100), +randomFloat(100)));
    //                 }
    //                 else
    //                 {
    //                     ctx.destroyEntity(i);
    //                 }
    //             }
    //         }
    //     }
    // }

    physics_object = createPhysicsCube();

    std::cout << "Added bounding box: " << c << "\n";

    camera = ctx.createEntity();
    Flux::Transform::giveTransform(camera);

    // camera_stick = ctx.createEntity();
    // Flux::Transform::giveTransform(camera_stick);

    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(0,0,20);
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

    player = ctx.getNamedEntity("Player");
    Flux::Transform::removeParent(player);
    Flux::Transform::setParent(camera, player);

    // auto pcc = (Flux::Physics::ConvexCollider)player.getComponent<Flux::Physics::ColliderCom>()->collider;

    Flux::Transform::translate(player, glm::vec3(0, 3, 0));

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");

    Flux::Debug::enableDebugDraw(&ctx);
}

glm::vec3 velocity(0, -8, 0);
bool on_ground = false;

void loop(float delta)
{
    // Flux::Transform::rotate(cube_stick, glm::vec3(0, 1, 0), 0.5 * delta);
    // glm::vec3 velocity(0, 0, 0);
    if (Flux::Input::isKeyPressed(FLUX_KEY_LEFT))
    {
        // Flux::Physics::move(player, glm::vec3(-5 * delta, -8 * delta, 0));
        velocity.x = -8;
    }
    else if (Flux::Input::isKeyPressed(FLUX_KEY_RIGHT))
    {
        // Flux::Physics::move(player, glm::vec3(5 * delta, -8 * delta, 0));
        velocity.x = 8;
    }
    else
    {
        velocity.x = 0;
    }

    if (Flux::Input::isKeyPressed(FLUX_KEY_UP) && on_ground)
    {
        velocity.y = 8;
    }

    // velocity.y -= 8;
    // velocity.y = glm::min(-8.0f, velocity.y);

    // auto out = Flux::Physics::move(player, velocity * delta);
    // Translate it initially
    Flux::Transform::translate(player, velocity * delta);

    // Check for collisions
    auto out = Flux::Physics::getCollisions(player);

    // "Solve" collisions
    for (auto i : out)
    {
        if (i.entity == physics_object)
        {
            continue;
        }
        Flux::Transform::globalTranslate(player, glm::vec3(0, (i.normal * i.depth).y, 0));
    }

    if (out.size() > 0)
    {
        // Check collision vector
        if (glm::dot(out[0].normal, glm::vec3(0, -1, 0)) < 0)
        {
            // I think we're on the ground
            // Don't apply gravity
            on_ground = true;
        }
        else
        {
            // We hit something that isn't ground
            // if (velocity.y > -8)
            // {
                // velocity.y -= 8 * delta;
            // }
            // else
            // {
                velocity.y = -8;
            // }

            on_ground = false;
        }
    }
    else
    {
        if (velocity.y > -8)
        {
            velocity.y -= 8 * delta;
        }
        else
        {
            velocity.y = -8;
        }

        on_ground = false;
    }

    // if (Flux::Input::isKeyPressed(FLUX_KEY_R))
    // {
    //     auto t = Flux::Transform::getTranslation(physics_object);
    //     printf("%f, %f, %f \n", t.x, t.y, t.z);
    // }
    // std::cout << out.size() << "\n";

    // Flux::Physics::addForce(physics_object, glm::vec3(-3, 8, 0), glm::vec3(0, -1, 0));

    ctx.runSystems(delta);
    Flux::Debug::run(delta);
}

void end()
{
    LOG_INFO("Ended...");
    ctx.destroyAllEntities();
}
