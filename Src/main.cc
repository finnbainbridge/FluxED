#define GLM_FORCE_CTOR_INIT
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "FluxGTK/FluxGTK.hh"
#include "Flux/Flux.hh"
#include "FluxED/FluxEd.hh"
#include "glm/matrix.hpp"

FluxED::BuildEditor* editor = nullptr;

void init(int argc, char *argv[])
{
    auto ctx = new Flux::ECSCtx();

    auto center = ctx->createEntity();
    Flux::Transform::giveTransform(center);
    Flux::Transform::rotate(center, glm::vec3(0, 1, 0), 1.5);

    ctx->destroyAllEntities();
    delete ctx;

    editor = new FluxED::BuildEditor(argc, argv);

    // auto a = glm::translate(glm::mat4(), glm::vec3(1, 1, 1));

    editor->run();
}

void loop(float delta) 
{
    editor->loop(delta);
}

void end() {}