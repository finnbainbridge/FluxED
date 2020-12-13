#include "gdkmm/device.h"
#include "gtkmm/button.h"
#include "gtkmm/glarea.h"
#include "gtkmm/label.h"
#include "sigc++/functors/ptr_fun.h"
#define GLM_FORCE_CTOR_INIT
#include "gtkmm/applicationwindow.h"
#include <gtkmm.h>
#include <iostream>

#include "FluxGTK/FluxGTK.hh"
#include "Flux/Flux.hh"
#include "Flux/Resources.hh"
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
int color = 0;
double click_timer = 0;

Flux::Resources::ResourceRef<Flux::Renderer::MaterialRes> mat_res;

Gtk::Label* label = nullptr;

void changeColor();

void init(int argc, char *argv[])
{
    auto app =
        Gtk::Application::create(argc, argv,
            "org.flux.fluxed");


    auto refBuilder = Gtk::Builder::create();
    try
    {
        refBuilder->add_from_file("FluxED.glade");
    }
    catch(const Glib::FileError& ex)
    {
        std::cerr << "FileError: " << ex.what() << std::endl;

    }
    catch(const Glib::MarkupError& ex)
    {
        std::cerr << "MarkupError: " << ex.what() << std::endl;

    }
    catch(const Gtk::BuilderError& ex)
    {
        std::cerr << "BuilderError: " << ex.what() << std::endl;

    }

    Gtk::Window* window = nullptr;

    refBuilder->get_widget("BuildEditor", window);

    Gtk::GLArea* glarea = nullptr;
    refBuilder->get_widget("3DGLArea", glarea);

    Gtk::EventBox* event_box = nullptr;
    refBuilder->get_widget("3DEventBox", event_box);

    refBuilder->get_widget("MousePos", label);

    Gtk::Button* button = nullptr;
    // refBuilder->get_widget("Button", button);

    // button->signal_clicked().connect(sigc::ptr_fun(changeColor));

    Flux::GTK::startRenderer(event_box, glarea);

    // Flux stuff
    ctx = Flux::ECSCtx();

    quad = ctx.createEntity();

    // TODO: Add function to do this once resource system exists
    auto mc = new Flux::Renderer::MeshRes;
    mc->vertices = new Flux::Renderer::Vertex[4]
    {
        Flux::Renderer::Vertex { 0.5f,  0.5f, 0.0f},
        Flux::Renderer::Vertex { 0.5f, -0.5f, 0.0f},
        Flux::Renderer::Vertex {-0.5f, -0.5f, 0.0f},
        Flux::Renderer::Vertex {-0.5f,  0.5f, 0.0f}
    };
    mc->indices = new uint32_t[6] {
        0, 1, 3,
        1, 2, 3
    };

    mc->vertices_length = 4;
    mc->indices_length = 6;

    auto mesh_res = Flux::Resources::createResource(mc);
    auto shaders_res = Flux::Renderer::createShaderResource("./shaders/vertex.vert", "./shaders/fragment.frag");
    mat_res = Flux::Renderer::createMaterialResource(shaders_res);
    LOG_INFO("Created resources");
    // Flux::addComponent(ctx, quad, Flux::getComponentType("mesh"), mc);

    // Flux::Renderer::temp_addShaders(ctx, quad, "./shaders/vertex.vert", "./shaders/fragment.frag");

    Flux::Renderer::setUniform(mat_res, "color", glm::vec3(0.4, 0.6, 0.2));

    Flux::Renderer::addMesh(quad, mesh_res, mat_res);
    LOG_INFO("Added mesh");

    auto camera = ctx.createEntity();

    auto tc = new Flux::Transform::TransformCom;
    tc->transformation = glm::mat4();
    tc->model_view = glm::mat4();
    tc->has_parent = false;

    LOG_INFO("Created camera");

    camera.addComponent<Flux::Transform::TransformCom>(tc);
    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(0.5,-0.5,0.5);
    Flux::Transform::translate(camera, o);
    LOG_INFO("Setup camera");

    auto o2 = glm::vec3(0,0,0);
    Flux::Transform::translate(quad, o2);

    // Flux::Transform::setParent(ctx, camera, quad);
    LOG_INFO("Repositioned quad");

    Flux::GLRenderer::addGLRenderer(&ctx);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(&ctx);
    LOG_INFO("Added transform systems");

    LOG_INFO("Completed init");

    window->add_events(Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::SMOOTH_SCROLL_MASK | Gdk::POINTER_MOTION_MASK);

    // Flux::Input::setMouseMode(Flux::Input::MouseMode::Confined);

    app->run(*window);
}

void changeColor()
{
    if (color == 0)
    {
        Flux::Renderer::setUniform(mat_res, "color", glm::vec3(1.0, 0.0, 0.0));
        color = 1;
    }
    else if (color == 1)
    {
        Flux::Renderer::setUniform(mat_res, "color", glm::vec3(0.0, 1.0, 0.0));
        color = 2;
    }
    else if (color == 2)
    {
        Flux::Renderer::setUniform(mat_res, "color", glm::vec3(0.0, 0.0, 1.0));
        color = 0;
    }
}

void loop(float delta)
{
    if (Flux::Input::isKeyPressed(FLUX_KEY_A) && Flux::Renderer::getTime() > click_timer)
    {
        changeColor();

        click_timer = Flux::Renderer::getTime() + 0.25;
    }
    Flux::Transform::setTranslation(quad, glm::vec3(Flux::Input::getMousePosition() / glm::vec2(Flux::GLRenderer::current_window->width, -Flux::GLRenderer::current_window->height), 0));
    ctx.runSystems(delta);

    glm::vec2 mp = Flux::Input::getMousePosition();
    // glm::vec2 mp = Flux::GLRenderer::current_window->mouse_pos;
    // LOG_INFO(std::to_string(mp.x) + " " + std::to_string(mp.y));
    // label->set_label(std::to_string(mp.x) + " " + std::to_string(mp.y));
}

void end()
{
    ctx.destroyAllEntities();
}
