#include "FluxED/FluxEd.hh"

#include "FluxGTK/FluxGTK.hh"

using namespace FluxED;

Editor3D::Editor3D(Gtk::GLArea* glarea, Gtk::EventBox* event_box)
{
    Flux::GTK::startRenderer(event_box, glarea);
}