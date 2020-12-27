#include "FluxArc/FluxArc.hh"
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

    auto tc = new Flux::Transform::TransformCom;
    tc->transformation = glm::mat4();
    tc->model_view = glm::mat4();
    tc->has_parent = false;

    LOG_INFO("Created camera");

    camera.addComponent<Flux::Transform::TransformCom>(tc);
    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(0,0,3.5);
    Flux::Transform::translate(camera, o);
    LOG_INFO("Setup camera");

    // auto o2 = glm::vec3(0,0,0);
    // Flux::Transform::translate(quad, o2);

    // // Flux::Transform::setParent(ctx, camera, quad);
    // LOG_INFO("Repositioned quad");

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");

    // LOG_INFO("Completed init");

    // Flux::Resources::Serializer ser;
    // ser.addEntity(quad);
    // ser.addEntity(camera);
    // FluxArc::Archive arc("test.farc");
    // ser.save(arc, true);

    // Flux::Resources::Deserializer s("assets/backpack.farc");
    Flux::Resources::Deserializer s("assets/GreenV2.1.farc");
    s.addToECS(&ctx);
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
    ctx.runSystems(delta);
}

void end()
{
    LOG_INFO("Ended...");
    ctx.destroyAllEntities();
}