#ifndef FLUXED_HH
#define FLUXED_HH

/**
Main header file of FluxED
Terminology:
    BuildEditor - The main FluxED window. Handles most IO operations
    Editor3D - The 3D editor for 3d file types. Lives inside BuildEditor
*/

#include "glibmm/ustring.h"
#include "gtkmm/eventbox.h"
#include "gtkmm/glarea.h"
#include "gtkmm/treemodel.h"
#include "gtkmm/treemodelcolumn.h"
#include "gtkmm/treestore.h"
#include "gtkmm/window.h"
#include "FluxGTK/FluxGTK.hh"
#include "FluxProj/FluxProj.hh"

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

    class BuildEditor
    {
    public:
        BuildEditor(int argc, char *argv[]);

        /** Starts the GTK window, and runs the BuildEditor */
        void run();

        // File IO
        void newProject();

        void openProject();

        void importFile();

        void removeFile();

    private:
        // GTK stuff
        Glib::RefPtr<Gtk::Application> app;
        Gtk::Window* window;
        FileModelColumns columns;
        Glib::RefPtr<Gtk::ListStore> file_store;
        Gtk::TreeView* tree;

        // Editor3D
        Editor3D* editor;

        // Project
        FluxProj::Project proj;
        bool has_project;

        // UI
        void updateFiles();

        // Data
        std::vector<Gtk::TreeModel::iterator> file_items;

    };

    class Editor3D
    {
        public:
            Editor3D(Gtk::GLArea* glarea, Gtk::EventBox* event_box);
    };
}

#endif