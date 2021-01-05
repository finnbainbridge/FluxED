#define GLM_FORCE_CTOR_INIT
#include "Flux/ECS.hh"
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include "FluxED/FluxEd.hh"

#include "FluxGTK/FluxGTK.hh"

using namespace FluxED;

Editor3D::Editor3D(Gtk::GLArea* glarea, Gtk::EventBox* event_box):
current_scene_loader(nullptr),
current_scene(nullptr)
{
    Flux::GTK::startRenderer(event_box, glarea);
    this->glarea = glarea;
    has_scene = false;
}

void Editor3D::loadScene(std::filesystem::path place)
{
    if (has_scene)
    {
        current_scene->destroyAllEntities();

        // vvv The bad line vvv
        current_scene_loader->destroyResources();

        delete current_scene_loader;
        current_scene_loader = nullptr;
        delete current_scene;
        current_scene = nullptr;
    }

    LOG_INFO("Editor3D: Loading scene...");
    // current_scene_loader = new Flux::Resources::Deserializer(place);
    current_scene = new Flux::ECSCtx();

    // Add Camera
    auto camera = current_scene->createEntity();

    Flux::Transform::giveTransform(camera);
    Flux::Transform::setCamera(camera);
    auto o = glm::vec3(0,0,2.5);
    LOG_INFO("Setup camera");


    Flux::GLRenderer::addGLRenderer(current_scene);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(current_scene);
    LOG_INFO("Added transform systems");

    Flux::Transform::translate(camera, o);

    LOG_INFO(place);
    
    current_scene_loader = new Flux::Resources::Deserializer(place);
    current_scene_loader->addToECS(current_scene);

    has_scene = true;
}

void Editor3D::loop(float delta)
{
    if (has_scene)
    {
        glarea->make_current();
        current_scene->runSystems(delta);
    }
}