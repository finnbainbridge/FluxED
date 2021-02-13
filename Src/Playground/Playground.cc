#include "FluxArc/FluxArc.hh"
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
Flux::EntityRef camera_stick;

Flux::EntityRef light;
Flux::EntityRef light_stick;
int color = 0;
double click_timer = 0;

Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes> mat_res;

void init(int argc, char** argv) {
    
    ctx = Flux::ECSCtx();

    // quad = ctx.createEntity();
    
    // // // TODO: Add function to do this once resource system exists
    // auto mc = new Flux::Renderer::MeshRes;
    // mc->vertices = new Flux::Renderer::Vertex[4]
    // {
    //     //                        X      Y     Z     NX   NY     NZ    TX    TY
    //     Flux::Renderer::Vertex { 0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    //     Flux::Renderer::Vertex { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    //     Flux::Renderer::Vertex {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    //     Flux::Renderer::Vertex {-0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
    // };
    // mc->indices = new uint32_t[6] {
    //     0, 1, 3,
    //     1, 2, 3
    // };

    // mc->vertices_length = 4;
    // mc->indices_length = 6;
    
    // auto mesh_res = Flux::Resources::createResource(mc);
    // auto shaders_res = Flux::Renderer::createShaderResource("./shaders/vertex.vert", "./shaders/fragment.frag");
    // mat_res = Flux::Renderer::createMaterialResource(shaders_res);
    // LOG_INFO("Created resources");
    // // Flux::addComponent(ctx, quad, Flux::getComponentType("mesh"), mc);

    // // Flux::Renderer::temp_addShaders(ctx, quad, "./shaders/vertex.vert", "./shaders/fragment.frag");

    // Flux::Renderer::setUniform(mat_res, "color", glm::vec3(0.4, 0.6, 0.2));
    
    // Flux::Renderer::addMesh(quad, mesh_res, mat_res);
    // LOG_INFO("Added mesh");

    camera = ctx.createEntity();
    Flux::Transform::giveTransform(camera);

    camera_stick = ctx.createEntity();
    Flux::Transform::giveTransform(camera_stick);

    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(0,0,10);
    Flux::Transform::translate(camera, o);
    LOG_INFO("Setup camera");

    Flux::Transform::setParent(camera, camera_stick);

    Flux::Renderer::addLight(camera, 10, glm::vec3(1, 0.1, 0.1));

    light = ctx.createEntity();
    Flux::Transform::giveTransform(light);

    light_stick = ctx.createEntity();
    Flux::Transform::giveTransform(light_stick);

    Flux::Transform::translate(light, o);
    LOG_INFO("Setup camera");

    Flux::Transform::setParent(light, light_stick);

    Flux::Renderer::addLight(light, 10, glm::vec3(0.1, 0.1, 1));

    // auto o2 = glm::vec3(0,0,0);
    // Flux::Transform::translate(quad, o2);

    // // Flux::Transform::setParent(ctx, camera, quad);
    // LOG_INFO("Repositioned quad");

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");

    for (int i = 0; i < 12; i++)
    {
        int num = random() % 3;
        
        if (num == 0)
        {
            auto loader = Flux::Resources::deserialize("assets/GreenV2.1.farc");
            auto ens = loader->addToECS(&ctx);
            Flux::Transform::translate(ens[0], glm::vec3((i-6) * 3, 0, 0));
        }
        else if (num == 1)
        {
            auto loader = Flux::Resources::deserialize("assets/jmodl.farc");
            auto ens = loader->addToECS(&ctx);
            Flux::Transform::translate(ens[0], glm::vec3((i-6) * 3, 0, 0));
        }
        else
        {
            auto loader = Flux::Resources::deserialize("assets/backpack.farc");
            auto ens = loader->addToECS(&ctx);
            Flux::Transform::translate(ens[0], glm::vec3((i-6) * 3, 0, 0));
        }
    }
}

void loop(float delta)
{
    // Do stuff
    // LOG_INFO("Looping...");
    // Flux::Transform::rotate(ctx, quad, glm::vec3(0, 1, 0), 3.14 * delta);
    
    // if (Flux::Input::isKeyPressed(FLUX_KEY_A) && Flux::Renderer::getTime() > click_timer)
    // {
    //     if (color == 0)
    //     {
    //         Flux::Renderer::setUniform(mat_res, "color", glm::vec3(1.0, 0.0, 0.0));
    //         color = 1;
    //     }
    //     else if (color == 1)
    //     {
    //         Flux::Renderer::setUniform(mat_res, "color", glm::vec3(0.0, 1.0, 0.0));
    //         color = 2;
    //     }
    //     else if (color == 2)
    //     {
    //         Flux::Renderer::setUniform(mat_res, "color", glm::vec3(0.0, 0.0, 1.0));
    //         color = 0;
    //     }

    //     click_timer = Flux::Renderer::getTime() + 0.25;
    // }

    // if (Flux::Input::isKeyPressed(FLUX_KEY_S) && Flux::Renderer::getTime() > click_timer)
    // {
    //     Flux::Resources::Serializer ser;
    //     ser.addEntity(quad);
    //     ser.addEntity(camera);
    //     FluxArc::Archive arc("test.farc");
    //     ser.save(arc, true);

    //     click_timer = Flux::Renderer::getTime() + 0.25;
    // }

    // Flux::Transform::setTranslation(quad, glm::vec3(Flux::Input::getMousePosition() / glm::vec2(Flux::GLRenderer::current_window->width, -Flux::GLRenderer::current_window->height), 0));
    Flux::Transform::rotate(camera_stick, glm::vec3(0, 1, 0), 0.5 * delta);
    // Flux::Transform::rotate(light_stick, glm::vec3(0, 1, 0), -0.5 * delta);
    ctx.runSystems(delta);
}

void end()
{
    LOG_INFO("Ended...");
    ctx.destroyAllEntities();
}