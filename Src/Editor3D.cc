#include "Flux/Log.hh"
#include "FluxArc/FluxArc.hh"
#include "gtkmm/dialog.h"
#include "gtkmm/expander.h"
#include "gtkmm/widget.h"
#include <algorithm>
#include <iostream>
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

    // Load presets
    builder->get_widget("PresetPicker", preset_picker);
    builder->get_widget("PPListBox", preset_tree);
    preset_store = Gtk::TreeStore::create(columns);
    preset_tree->set_model(preset_store);
    preset_tree->append_column("Name", columns.name);
    
    for (auto i : presets)
    {
        Gtk::TreeIter iter = preset_store->append();
        (*iter)[columns.name] = i.first;
        iters.push_back(iter);
        preset_iters[iters.size()-1] = i.second;
        std::cout << i.first << "\n";
    }
}

void Editor3D::loadScene(std::filesystem::path place)
{
    orbit_cam->destroy();
    delete orbit_cam;

    if (has_scene)
    {
        current_scene->destroyAllEntities();

        // current_scene_loader->destroyResources();

        // delete current_scene_loader;
        // current_scene_loader = nullptr;
        // Current scene loader will automatically free itself
        delete current_scene;
        current_scene = nullptr;
    }

    filename = place;

    LOG_INFO("Editor3D: Loading scene...");
    // current_scene_loader = new Flux::Resources::Deserializer(place);
    current_scene_loader = Flux::Resources::deserialize(place);
    current_scene = new Flux::ECSCtx();

    // Add Camera
    orbit_cam = new View3D::OrbitCamera();
    orbit_cam->addToCtx(gizmo_ecs);
    orbit_cam->addToCtx(current_scene);

    Flux::GLRenderer::addGLRenderer(current_scene);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(current_scene);
    LOG_INFO("Added transform systems");

    LOG_INFO(place);

    // current_scene_loader = new Flux::Resources::Deserializer(place);
    entities = current_scene_loader->addToECS(current_scene);

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

bool Editor3D::sceneSaveAs(std::filesystem::path path)
{
    if (!has_scene)
    {
        // lol this happened
        // TODO: Tell the user
        return false;
    }

    Flux::Resources::Serializer ser(path);

    for (auto e : entities)
    {
        ser.addEntity(e);
    }

    FluxArc::Archive arc(path);

    ser.save(arc, true);

    return true;
}

void Editor3D::sceneSave()
{
    if (!has_scene)
    {
        // lol this happened
        // TODO: Tell the user
        return;
    }

    Flux::Resources::Serializer ser(filename);

    for (auto e : entities)
    {
        ser.addEntity(e);
    }

    FluxArc::Archive arc(filename);

    ser.save(arc, true);

    return;
}

void Editor3D::linkScene(std::filesystem::path path)
{
    if (!has_scene)
    {
        return;
    }

    // Create new entity
    // TODO: Name
    auto new_entity = current_scene->createEntity();
    Flux::Transform::giveTransform(new_entity);

    Flux::Resources::createSceneLink(new_entity, path);
    entities.push_back(new_entity);

    // Add to tree
    Gtk::TreeIter iter;
    if (new_entity.hasComponent<Flux::Transform::TransformCom>())
    {
        auto tc = new_entity.getComponent<Flux::Transform::TransformCom>();
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

    
    if (new_entity.hasComponent<Flux::NameCom>())
    {
        (*iter)[columns.name] = new_entity.getComponent<Flux::NameCom>()->name;
    }
    else
    {
        (*iter)[columns.name] = "Entity " + std::to_string(new_entity.getEntityID() - 2);
    }

    entity_tree_values[new_entity.getEntityID()] = iter;
}

void Editor3D::newScene(std::filesystem::path path)
{
    // TODO: "Do you want to save" dialog
    orbit_cam->destroy();
    delete orbit_cam;

    if (has_scene)
    {
        current_scene->destroyAllEntities();

        // current_scene_loader->destroyResources();

        // delete current_scene_loader;
        // current_scene_loader = nullptr;
        // Current scene loader will automatically free itself
        delete current_scene;
        current_scene = nullptr;
    }

    filename = path;

    LOG_INFO("Editor3D: Loading scene...");
    // current_scene_loader = new Flux::Resources::Deserializer(place);
    // current_scene_loader = Flux::Resources::deserialize(place);
    current_scene = new Flux::ECSCtx();

    // Add Camera
    orbit_cam = new View3D::OrbitCamera();
    orbit_cam->addToCtx(gizmo_ecs);
    orbit_cam->addToCtx(current_scene);

    Flux::GLRenderer::addGLRenderer(current_scene);
    LOG_INFO("Added GL Renderer");
    Flux::Transform::addTransformSystems(current_scene);
    LOG_INFO("Added transform systems");


    // current_scene_loader = new Flux::Resources::Deserializer(place);
    // entities = current_scene_loader->addToECS(current_scene);
    entities = std::vector<Flux::EntityRef>();

    // Parse entities
    entity_store->clear();
    entity_tree_values = std::map<int, Gtk::TreeIter>();

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

    // Make it exist
    sceneSave();
}

void Editor3D::rename(const std::string &name)
{
    if (name == "")
    {
        return;
    }
    
    auto selection = entity_tree->get_selection();
    auto iter = selection->get_selected();

    for (auto i : entity_tree_values)
    {
        if (i.second == iter)
        {
            auto er = Flux::EntityRef(current_scene, i.first);
            
            if (!er.hasComponent<Flux::NameCom>())
            {
                er.addComponent(new Flux::NameCom);
            }

            er.getComponent<Flux::NameCom>()->name = name;

            (*iter)[columns.name] = name;
        }
    }
}

void Editor3D::addPreset()
{
    if (!has_scene) {return;}
    auto out = preset_picker->run();

    switch (out)
    {
        case (Gtk::RESPONSE_OK):
            auto iter = preset_tree->get_selection()->get_selected();
            auto fname = preset_iters[std::find(iters.begin(), iters.end(), iter) - iters.begin()];
            // preset_picker->hide();

            // Load the file
            auto file = Flux::Resources::deserialize(fname, true);
            auto en = file->addToECS(current_scene);

            for (auto i : en)
            {
                entities.push_back(i);

                // TODO: Make this bit of code not deeply flawed
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

            break;
    }

    preset_picker->hide();
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