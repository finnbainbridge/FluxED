#include <iostream>
#include "FluxGTK/FluxGTK.hh"
#include "Flux/Flux.hh"
#include "FluxED/FluxEd.hh"

void init(int argc, char *argv[])
{
    auto editor = FluxED::BuildEditor(argc, argv);

    editor.run();
}

void loop(float delta) {}

void end() {}