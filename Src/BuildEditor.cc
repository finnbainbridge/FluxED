#include "Flux/Log.hh"
#include "FluxED/FluxEd.hh"
#include "gtkmm/filechooser.h"
#include "gtkmm/liststore.h"
#include "gtkmm/menuitem.h"
#include "gtkmm/treeview.h"
#include "sigc++/functors/mem_fun.h"

#include <filesystem>
#include <gtkmm.h>
#include <iostream>

#include "FluxProj/FluxProj.hh"

using namespace FluxED;

BuildEditor::BuildEditor(int argc, char *argv[]):
window(nullptr),
tree(nullptr),
editor(nullptr),
proj(nullptr),
entry_dialog(nullptr),
entry_entry(nullptr),
entry_label(nullptr)
{
    // Setup variables
    has_project = false;

    
    // Create GTK window and load widgets
    app =
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

    window = nullptr;
    refBuilder->get_widget("BuildEditor", window);
    
    if (window == nullptr)
    {
        LOG_ERROR("Could not start window: Missing FluxED.glade file");
        return;
    }

    // Prepare GTK for 3D Editor
    Gtk::GLArea* glarea = nullptr;
    refBuilder->get_widget("3DGLArea", glarea);

    Gtk::EventBox* event_box = nullptr;
    refBuilder->get_widget("3DEventBox", event_box);

    window->add_events(Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::SMOOTH_SCROLL_MASK | Gdk::POINTER_MOTION_MASK  | Gdk::SCROLL_MASK);

    // Create 3D Editor
    editor = new Editor3D(glarea, event_box, refBuilder);

    // Connect signals
    Gtk::MenuItem* menu = nullptr; // Reuse the same variable

    // Entry dialog
    refBuilder->get_widget("TextEntryDialog", entry_dialog);
    refBuilder->get_widget("TEEntry", entry_entry);
    refBuilder->get_widget("TEMessage", entry_label);
    // entry_dialog->run();

    // File Menu:
    refBuilder->get_widget("MenuNew", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::newProject));

    refBuilder->get_widget("MenuOpen", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::openProject));

    refBuilder->get_widget("MenuImport", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::importFile));

    refBuilder->get_widget("MenuRemove", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::removeFile));

    refBuilder->get_widget("MenuRebuild", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::rebuildProject));

    refBuilder->get_widget("MenuSaveAs", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::sceneSaveAs));

    refBuilder->get_widget("MenuSave", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::sceneSave));
    
    refBuilder->get_widget("LinkScene", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::linkScene));

    refBuilder->get_widget("MenuNewScene", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::newScene));

    refBuilder->get_widget("MenuRename", menu);
    menu->signal_activate().connect(sigc::mem_fun(*this, &BuildEditor::sceneRename));

    refBuilder->get_widget("MenuAddPreset", menu);
    menu->signal_activate().connect(sigc::mem_fun(*editor, &Editor3D::addPreset));

    // Stores and Trees
    refBuilder->get_widget("FileTree", tree);
    file_store = Gtk::ListStore::create(columns);
    tree->set_model(file_store);

    tree->signal_row_activated().connect(sigc::mem_fun(*this, &BuildEditor::itemClicked));

    tree->append_column("Input File", columns.input_filename);
    tree->append_column("Output File", columns.output_filename);

    file_store->set_sort_column(columns.input_filename, Gtk::SORT_ASCENDING);

}

void BuildEditor::run()
{
    app->run(*window);
}

void BuildEditor::loop(float delta)
{
    editor->loop(delta);
}

void BuildEditor::updateFiles()
{
    // for (auto i: file_items)
    // {
    //     file_store->erase(i);
    // }

    // file_items = std::vector<Gtk::TreeModel::iterator>();

}

// ===========================================================
// Tree stuff
// ===========================================================

void BuildEditor::itemClicked(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto iter = file_store->get_iter(path);
    std::string out_fname = iter->get_value(columns.output_filename);
    editor->loadScene(proj->getFolder() / out_fname);
}

// ===========================================================
// Helpers
// ===========================================================

std::string BuildEditor::getTextDialog(const std::string& title, const std::string& message)
{
    entry_label->set_text(message);
    entry_dialog->set_title(title);
    entry_entry->set_text("");

    auto res = entry_dialog->run();
    switch (res)
    {
        case(Gtk::RESPONSE_OK):
        {
            entry_dialog->hide();
            return entry_entry->get_text();
            // break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            entry_dialog->hide();
            return "";
            // break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            entry_dialog->hide();
            return "";
            // break;
        }
    }
}

// ===========================================================
// Menu stuff
// ===========================================================

void BuildEditor::rebuildProject()
{
    proj->runBuild(true);
}

// Thanks Stack Overflow: https://stackoverflow.com/a/874160/7179625
bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void BuildEditor::openProject()
{
    // TODO: Use native dialogs
    Gtk::FileChooserDialog dialog("Open a project file",
          Gtk::FILE_CHOOSER_ACTION_OPEN);
    // dialog.set_do_overwrite_confirmation(true);
        // Gtk::FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME );
    
    dialog.set_transient_for(*window);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("FluxED Project Files");
    filter_any->add_mime_type("application/json");
    filter_any->add_pattern("*.proj.json");
    dialog.add_filter(filter_any);

    //Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Open", Gtk::RESPONSE_OK);

    // dialog.set_filename("untitled.proj.json");

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
            std::string filename = dialog.get_filename();

            // if (!hasEnding(filename, ".proj.json") || !hasEnding(filename, ".json"))
            // {
            //     filename = filename + ".proj.json";
            // }

            proj = new FluxProj::Project(filename);
            has_project = true;
            window->set_title(std::filesystem::path(filename).filename().string() + " - FluxED");

            // Load files
            for (auto i : proj->getPaths())
            {
                if (i.active)
                {
                    // this check was unnessesairy
                    auto iter = file_store->append();
                    (*iter)[columns.input_filename] = i.input.string();
                    (*iter)[columns.output_filename] = i.output.string();
                }
            }
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            break;
        }
    }
}

void BuildEditor::newProject()
{
    Gtk::FileChooserDialog dialog("Create a project file",
          Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_do_overwrite_confirmation(true);
        // Gtk::FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME );
    
    dialog.set_transient_for(*window);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("FluxED Project Files");
    filter_any->add_mime_type("application/json");
    filter_any->add_pattern("*.proj.json");
    dialog.add_filter(filter_any);

    //Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Create", Gtk::RESPONSE_OK);

    dialog.set_filename("untitled.proj.json");

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
            std::string filename = dialog.get_filename();

            if (!hasEnding(filename, ".proj.json") || !hasEnding(filename, ".json"))
            {
                filename = filename + ".proj.json";
            }

            proj = new FluxProj::Project(filename, true);
            has_project = true;
            window->set_title(std::filesystem::path(filename).filename().string() + " - FluxED");
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            break;
        }
    }
}

void BuildEditor::importFile()
{
    if (!has_project)
    {
        // TODO: Error box
        return;
    }

    Gtk::FileChooserDialog dialog("Open a project file",
          Gtk::FILE_CHOOSER_ACTION_OPEN);
    // dialog.set_do_overwrite_confirmation(true);
        // Gtk::FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME );
    
    dialog.set_transient_for(*window);
    dialog.set_current_folder(proj->getFolder());

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("All Files");
    // filter_any->add_mime_type("application/json");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);

    //Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Open", Gtk::RESPONSE_OK);

    // dialog.set_filename("untitled.proj.json");

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
            std::string filename = dialog.get_filename();

            // if (!hasEnding(filename, ".proj.json") || !hasEnding(filename, ".json"))
            // {
            //     filename = filename + ".proj.json";
            // }

            auto pp = proj->addFile(filename);
            if (pp.active)
            {
                auto iter = file_store->append();
                (*iter)[columns.input_filename] = pp.input.string();
                (*iter)[columns.output_filename] = pp.output.string();
            }
            
            updateFiles();
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            break;
        }
    }
}

void BuildEditor::removeFile()
{
    auto selection = tree->get_selection();
    auto row = selection->get_selected();
    
    proj->removeFile(row->get_value(columns.input_filename));

    file_store->erase(row);
}

void BuildEditor::sceneSaveAs()
{
    Gtk::FileChooserDialog dialog("Save Scene As",
          Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_do_overwrite_confirmation(true);
        // Gtk::FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME );
    
    dialog.set_transient_for(*window);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Flux Scenes");
    filter_any->add_pattern("*.farc");
    dialog.add_filter(filter_any);

    //Add response buttons the the dialog:
    dialog.add_button("_Save", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Create", Gtk::RESPONSE_OK);

    dialog.set_filename("untitled.farc");

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
            std::string filename = dialog.get_filename();

            if (!hasEnding(filename, ".farc"))
            {
                filename = filename + ".farc";
            }

            if (editor->sceneSaveAs(filename))
            {
                // Success! Add to project
                proj->addFile(filename);

                auto iter = file_store->append();
                (*iter)[columns.input_filename] = filename;
                (*iter)[columns.output_filename] = filename;
            }

            break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            break;
        }
    }
}

void BuildEditor::sceneSave()
{
    editor->sceneSave();
}

void BuildEditor::linkScene()
{
    Gtk::FileChooserDialog dialog("Open a scene file",
          Gtk::FILE_CHOOSER_ACTION_OPEN);
    // dialog.set_do_overwrite_confirmation(true);
        // Gtk::FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME );
    
    dialog.set_transient_for(*window);
    dialog.set_current_folder(proj->getFolder());

    auto filter_any = Gtk::FileFilter::create();
    // filter_any->add_mime_type("application/json");
    filter_any->set_name("Flux Scenes");
    filter_any->add_pattern("*.farc");
    dialog.add_filter(filter_any);

    //Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Open", Gtk::RESPONSE_OK);

    // dialog.set_filename("untitled.proj.json");

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
            std::string filename = dialog.get_filename();

            editor->linkScene(filename);
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            break;
        }
    }
}

void BuildEditor::newScene()
{
    Gtk::FileChooserDialog dialog("Create a Flux scene",
          Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_do_overwrite_confirmation(true);
        // Gtk::FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME );
    
    dialog.set_transient_for(*window);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Flux Scenes");
    filter_any->add_pattern("*.farc");
    dialog.add_filter(filter_any);

    //Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Create", Gtk::RESPONSE_OK);

    dialog.set_filename("untitled.farc");

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
            std::string filename = dialog.get_filename();

            if (!hasEnding(filename, ".farc"))
            {
                filename = filename + ".farc";
            }

            editor->newScene(filename);

            // Success! Add to project
            proj->addFile(filename);

            auto fn = std::filesystem::relative(filename, proj->getFolder());

            auto iter = file_store->append();
            (*iter)[columns.input_filename] = fn.string();
            (*iter)[columns.output_filename] = fn.string();
            break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
            // std::cout << "Cancel clicked." << std::endl;
            break;
        }
        default:
        {
            // std::cout << "Unexpected button clicked." << std::endl;
            break;
        }
    }
}

void BuildEditor::sceneRename()
{
    auto name = getTextDialog("Rename", "Enter new name for Entity:");
    editor->rename(name);
}