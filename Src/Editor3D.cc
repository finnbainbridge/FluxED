#include "Flux/Log.hh"
#include "gtkmm/expander.h"
#include "gtkmm/widget.h"
#include <string>
#define GLM_FORCE_CTOR_INIT
#include "FluxED/3DView.hh"
#include "glm/gtc/matrix_transform.hpp"
#include "Flux/ECS.hh"
#include "Flux/OpenGL/GLRenderer.hh"
#include "Flux/Renderer.hh"
#include "Flux/Resources.hh"
#include "FluxED/FluxEd.hh"

#include "FluxGTK/FluxGTK.hh"

using namespace FluxED;

Editor3D::Editor3D(Gtk::GLArea* glarea, Gtk::EventBox* event_box, Glib::RefPtr<Gtk::Builder> builder):
current_scene(nullptr),
current_scene_loader(nullptr),
glarea(glarea),
orbit_cam(nullptr),
com_list(nullptr),
gizmo_ecs(nullptr),
columns()
{
    Flux::GTK::startRenderer(event_box, glarea);
    has_scene = false;

    gizmo_ecs = new Flux::ECSCtx();
    Flux::GLRenderer::addGLRenderer(gizmo_ecs);
    // LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(gizmo_ecs);
    // LOG_INFO("Added transform systems");

    orbit_cam = new View3D::OrbitCamera();
    orbit_cam->addToCtx(gizmo_ecs);

    // Setup GTK UI Elements
    builder->get_widget("Entity", entity_tree);
    entity_store = Gtk::TreeStore::create(columns);
    entity_tree->set_model(entity_store);
    entity_tree->append_column("Name", columns.name);
    entity_tree->signal_row_activated().connect(sigc::mem_fun(*this, &Editor3D::entityClicked));

    builder->get_widget("List", com_list);

    // Load Component Plugins
    for (auto i : components)
    {
        i.second->init(this);
    }
    
}

void Editor3D::loadScene(std::filesystem::path place)
{
    orbit_cam->destroy();
    delete orbit_cam;

    if (has_scene)
    {
        current_scene->destroyAllEntities();

        current_scene_loader->destroyResources();

        delete current_scene_loader;
        current_scene_loader = nullptr;
        delete current_scene;
        current_scene = nullptr;
    }

    LOG_INFO("Editor3D: Loading scene...");
    current_scene_loader = new Flux::Resources::Deserializer(place);
    current_scene = new Flux::ECSCtx();

    // Check for memory errors
    // For some reason the existance of this line stops memory errors
    // FML
    // glm::rotate(glm::mat4(), 1.5f, glm::vec3(0, 1, 0));
    // auto thing = current_scene->createEntity();
    // Flux::Transform::giveTransform(thing);
    // Flux::Transform::rotate(thing, glm::vec3(0, 1, 0), 1.5);

    // Add Camera
    orbit_cam = new View3D::OrbitCamera();
    orbit_cam->addToCtx(gizmo_ecs);
    orbit_cam->addToCtx(current_scene);

    Flux::GLRenderer::addGLRenderer(current_scene);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(current_scene);
    LOG_INFO("Added transform systems");

    LOG_INFO(place);

    current_scene_loader = new Flux::Resources::Deserializer(place);
    auto entities = current_scene_loader->addToECS(current_scene);

    // Parse entities
    entity_store->clear();
    entity_tree_values = std::map<int, Gtk::TreeIter>();

    for (auto i : entities)
    {
        Gtk::TreeIter iter;
        if (i.hasComponent<Flux::Transform::TransformCom>())
        {
            auto tc = i.getComponent<Flux::Transform::TransformCom>();
            if (tc->has_parent)
            {
                auto parent_iter = entity_tree_values[tc->parent.getEntityID()];
                iter = entity_store->append(parent_iter->children());
            }
            else
            {
                iter = entity_store->append();
            }
        }
        else
        {
            iter = entity_store->append();
        }

        
        if (i.hasComponent<Flux::NameCom>())
        {
            (*iter)[columns.name] = i.getComponent<Flux::NameCom>()->name;
        }
        else
        {
            (*iter)[columns.name] = "Entity " + std::to_string(i.getEntityID() - 2);
        }

        entity_tree_values[i.getEntityID()] = iter;
    }

    for (auto i : comps)
    {
        com_list->remove(*i);
        i->remove();
        delete i;
    }
    for (auto i : active_complugins)
    {
        i->removed();
    }
    comps = std::vector<Gtk::Expander*>();
    active_complugins = std::vector<ComPlugin*>();

    has_scene = true;
}

void Editor3D::loadComponents(Flux::EntityRef entity)
{
    for (auto i : comps)
    {
        com_list->remove(*i);
        i->remove();
        delete i;
    }
    for (auto i : active_complugins)
    {
        i->removed();
    }
    comps = std::vector<Gtk::Expander*>();
    active_complugins = std::vector<ComPlugin*>();

    for (auto i : components)
    {
        Gtk::Widget* w = i.second->getForEntity(entity);
        
        if (w != nullptr)
        {
            auto exp = new Gtk::Expander();
            exp->set_label(i.first);
            w->show_all();
            exp->add(*w);

            comps.push_back(exp);
            exp->show();
            com_list->add(*exp);
            com_list->show();

            active_complugins.push_back(i.second);
        }
    }
}

void Editor3D::entityClicked(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto iter = entity_store->get_iter(path);

    for (auto i : entity_tree_values)
    {
        if (iter == i.second)
        {
            auto er = Flux::EntityRef(current_scene, i.first);
            if (er.hasComponent<Flux::Transform::TransformCom>())
            {
                orbit_cam->setPosition(Flux::Transform::getGlobalTranslation(er));
            }

            // Load components
            loadComponents(er);

            return;
        }
    }

    LOG_WARN("Could not find entity");
}

void Editor3D::loop(float delta)
{
    glarea->make_current();
    orbit_cam->loop(delta);
    
    if (has_scene)
    {
        current_scene->runSystems(delta);

        for (auto i : components)
        {
            i.second->loop(delta);
        }
    }

    gizmo_ecs->runSystems(delta);
}