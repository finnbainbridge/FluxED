#ifndef FLUXED_HH
#define FLUXED_HH

/**
Main header file of FluxED
Terminology:
    BuildEditor - The main FluxED window. Handles most IO operations
    Editor3D - The 3D editor for 3d file types. Lives inside BuildEditor
*/

#include "3DView.hh"
#include "Flux/ECS.hh"
#include "Flux/Resources.hh"
#include "glibmm/ustring.h"
#include "gtkmm/box.h"
#include "gtkmm/entry.h"
#include "gtkmm/eventbox.h"
#include "gtkmm/glarea.h"
#include "gtkmm/listbox.h"
#include "gtkmm/treemodel.h"
#include "gtkmm/treemodelcolumn.h"
#include "gtkmm/treestore.h"
#include "gtkmm/window.h"
#include "FluxGTK/FluxGTK.hh"
#include "FluxProj/FluxProj.hh"
#include <filesystem>
#include <functional>

namespace FluxED
{
    class Editor3D;

    class FileModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        FileModelColumns()
        {
            add(input_filename);
            add(output_filename);
        }

        Gtk::TreeModelColumn<Glib::ustring> input_filename;
        Gtk::TreeModelColumn<Glib::ustring> output_filename;
    };

    class EntityModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        EntityModelColumns()
        {
            add(name);
        }

        Gtk::TreeModelColumn<Glib::ustring> name;
    };

    class BuildEditor
    {
    public:
        BuildEditor(int argc, char *argv[]);

        /** Starts the GTK window, and runs the BuildEditor */
        void run();

        /** Gets called every frame */
        void loop(float delta);

        // Helpers
        std::string getTextDialog(const std::string& title, const std::string& message);

        // File IO
        void newProject();

        void openProject();

        void importFile();

        void removeFile();

        void rebuildProject();

        void sceneSaveAs();

        void sceneSave();

        void linkScene();

        void newScene();

        void sceneRename();

        // Tree
        void itemClicked(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

    private:
        // GTK stuff
        Glib::RefPtr<Gtk::Application> app;
        Gtk::Window* window;
        FileModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> file_store;
        Gtk::TreeView* tree;

        Gtk::Dialog* entry_dialog;
        Gtk::Entry* entry_entry;
        Gtk::Label* entry_label;

        // Editor3D
        Editor3D* editor;

        // Project
        FluxProj::Project* proj;
        bool has_project;

        // UI
        void updateFiles();

        // Data
        std::vector<Gtk::TreeModel::iterator> file_items;

    };

    /**
    Base class for component plugins.
    Plugins show a GUI element in the componets section, and while that gui section is shown,
    can run every frame.
    */
    class ComPlugin
    {
    public:
        virtual Gtk::Widget* getForEntity(Flux::EntityRef e) {return nullptr;};

        virtual void loop(float delta) {};

        virtual void init(Editor3D* be) {};

        virtual void removed() {};
    };

    inline std::unordered_map<std::string, ComPlugin*> components;

    /**
    Adds a new component plugin to FluxED.
    Component plugins can show a GUI box, and while that box is shown,
    they can run every frame.
    */
    inline bool addComponentPlugin(const std::string& ext, ComPlugin* com)
    {
        components[ext] = com;
        return true;
    }

    inline std::unordered_map<std::string, std::string> presets;

    /**
    Adds a preset to FluxED. This preset can be added to any scene
    */
    inline bool addPreset(const std::string& ext, const std::string& filename)
    {
        presets[ext] = filename;
        return true;
    }

    class Editor3D
    {
    public:
        Editor3D(Gtk::GLArea* glarea, Gtk::EventBox* event_box, Glib::RefPtr<Gtk::Builder> builder);

        /** 
        Load a 3D Scene into the 3D editor.
        A 3D scene is a file created using the Serializer class.
        */
        void loadScene(std::filesystem::path place);

        void loop(float delta);

        void entityClicked(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

        /**  The Camera  */
        View3D::OrbitCamera* orbit_cam;

        /** Where the gizmos live */
        Flux::ECSCtx* gizmo_ecs;

        bool sceneSaveAs(std::filesystem::path path);

        void sceneSave();

        void linkScene(std::filesystem::path path);

        void newScene(std::filesystem::path path);

        void rename(const std::string& name);

        void addPreset();

    private:

        void loadComponents(Flux::EntityRef entity);

        Flux::ECSCtx* current_scene;
        Flux::Resources::Deserializer* current_scene_loader;
        std::filesystem::path filename;
        bool has_scene;

        std::vector<Flux::EntityRef> entities;

        Gtk::GLArea* glarea;
        Gtk::Box* com_list;

        std::vector<Gtk::Expander*> comps;
        std::vector<ComPlugin*> active_complugins;

        EntityModelColumns columns;
        Gtk::TreeView* entity_tree;
        Glib::RefPtr<Gtk::TreeStore> entity_store;
        std::map<int, Gtk::TreeIter> entity_tree_values;

        Gtk::TreeView* preset_tree;
        Glib::RefPtr<Gtk::TreeStore> preset_store;
        Gtk::Dialog* preset_picker;
        std::map<int, std::string> preset_iters;
        std::vector<Gtk::TreeIter> iters;
    };
}

#endif