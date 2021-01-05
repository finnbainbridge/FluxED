#define GLM_FORCE_CTOR_INIT
#include "glm/ext/matrix_transform.hpp"
#include <iostream>
#include "FluxGTK/FluxGTK.hh"
#include "Flux/Flux.hh"
#include "FluxED/FluxEd.hh"
#include "glm/matrix.hpp"

FluxED::BuildEditor* editor;

void init(int argc, char *argv[])
{
    editor = new FluxED::BuildEditor(argc, argv);

    auto a = glm::translate(glm::mat4(), glm::vec3(1, 1, 1));

    editor->run();
}

void loop(float delta) 
{
    editor->loop(delta);
}

void end() {}