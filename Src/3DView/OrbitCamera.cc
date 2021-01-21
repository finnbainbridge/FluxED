#include "glm/gtc/matrix_transform.hpp"
#include <ostream>
#define GLM_FORCE_CTOR_INIT
#include "Flux/ECS.hh"
#include "Flux/Input.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "FluxED/3DView.hh"
#include <string>

using namespace FluxED;

View3D::OrbitCamera::OrbitCamera()
{
    // Create entities
    // center = ctx->createEntity();
    // Flux::Transform::giveTransform(center);
    // // Flux::Transform::rotate(center, glm::vec3(0, 1, 0), 1.5);

    // camera = ctx->createEntity();
    // Flux::Transform::giveTransform(camera);
    // Flux::Transform::setParent(camera, center);
    // Flux::Transform::translate(camera, glm::vec3(0, 0, 5));
    // Flux::Transform::setCamera(camera);
}

void View3D::OrbitCamera::addToCtx(Flux::ECSCtx *ctx)
{
    // Create entities
    auto center = ctx->createEntity();
    Flux::Transform::giveTransform(center);
    // Flux::Transform::rotate(center, glm::vec3(0, 1, 0), 1.5);

    auto camera = ctx->createEntity();
    Flux::Transform::giveTransform(camera);
    Flux::Transform::setParent(camera, center);
    Flux::Transform::translate(camera, glm::vec3(0, 0, 5));
    Flux::Transform::setCamera(camera);

    centers.push_back(center);
    cameras.push_back(camera);
}

void View3D::OrbitCamera::loop(float delta)
{
    // Check for the "move" button
    if (Flux::Input::isMouseButtonPressed(FLUX_MOUSE_BUTTON_3))
    {
        // Trap the mouse
        Flux::Input::setMouseMode(Flux::Input::MouseMode::Confined);
        auto offset = Flux::Input::getMouseOffset();

        if (Flux::Input::isKeyPressed(FLUX_KEY_LEFT_SHIFT))
        {
            // Move
            for (auto center: centers)
            {
                Flux::Transform::translate(center, glm::vec3(offset.x * 0.01, offset.y * -0.01, 0));
            }
        }
        else
        {
            // Rotate the global Y, but local X
            // Mouse x is 3d y, mouse y is 3d x
            for (auto center: centers)
            {
                Flux::Transform::rotate(center, glm::vec3(1, 0, 0), offset.y * 0.02f);
                Flux::Transform::rotateGlobalAxis(center, glm::vec3(0, 1, 0), offset.x * 0.02f);
            }
        }
    }
    else
    {
        // Untrap the mouse
        Flux::Input::setMouseMode(Flux::Input::MouseMode::Free);
    }

    // Zooming
    float off = Flux::Input::getScrollWheelOffset();
    if (off != 0)
    {
        for (auto camera: cameras)
        {
            auto pos = Flux::Transform::getTranslation(camera);
            float one = pos.z * 0.05;
            // LOG_INFO(std::to_string(pos.z));
            // LOG_INFO(std::to_string(off * one * -1.0f));
            Flux::Transform::translate(camera, glm::vec3(0, 0, one * off * -1.0f));
        }
    }
}

void View3D::OrbitCamera::setPosition(glm::vec3 pos)
{
    for (auto center : centers)
    {
        Flux::Transform::setTranslation(center, pos);
    }
}

glm::mat4 View3D::OrbitCamera::getViewMatrix()
{
    return glm::inverse(centers[0].getComponent<Flux::Transform::TransformCom>()->transformation * cameras[0].getComponent<Flux::Transform::TransformCom>()->transformation);
}

void View3D::OrbitCamera::destroy()
{
    for (auto camera : cameras)
    {
        camera.getCtx()->destroyEntity(camera);
    }

    for (auto center : centers)
    {
        center.getCtx()->destroyEntity(center);
    }
}

View3D::AxisLines::AxisLines(Flux::ECSCtx* ctx, float full, float empty)
{
    // Create Gizmos
    x_axis = ctx->createEntity();
    y_axis = ctx->createEntity();
    z_axis = ctx->createEntity();
    auto shaders = Flux::Renderer::createShaderResource("shaders/basic.vert", "shaders/basic.frag");
    // Flux::Transform::giveTransform(x_axis);

    // Create X Axis mesh
    Flux::Renderer::MeshRes* m = new Flux::Renderer::MeshRes;
    m->indices = new uint32_t[2] {0, 1};
    m->indices_length = 2;
    m->vertices = new Flux::Renderer::Vertex[2];
    m->vertices[0] = Flux::Renderer::Vertex {-10000, 0, 0, 0, 0, 0, 0, 0};
    m->vertices[1] = Flux::Renderer::Vertex {10000, 0, 0, 0, 0, 0, 0, 0};
    m->vertices_length = 2;
    m->draw_mode = Flux::Renderer::DrawMode::Lines;
    auto mesh_res = Flux::Resources::createResource(m);

    // Create material
    auto mat_res = Flux::Renderer::createMaterialResource(shaders);
    Flux::Renderer::setUniform(mat_res, "color", glm::vec3(full, empty, empty));

    Flux::Renderer::addMesh(x_axis, mesh_res, mat_res);

    // Create Y Axis mesh
    Flux::Renderer::MeshRes* my = new Flux::Renderer::MeshRes;
    my->indices = new uint32_t[2] {0, 1};
    my->indices_length = 2;
    my->vertices = new Flux::Renderer::Vertex[2];
    my->vertices[0] = Flux::Renderer::Vertex {0, -10000, 0, 0, 0, 0, 0, 0};
    my->vertices[1] = Flux::Renderer::Vertex {0, 10000, 0, 0, 0, 0, 0, 0};
    my->vertices_length = 2;
    my->draw_mode = Flux::Renderer::DrawMode::Lines;
    auto mesh_resy = Flux::Resources::createResource(my);

    // Create material
    auto mat_resy = Flux::Renderer::createMaterialResource(shaders);
    Flux::Renderer::setUniform(mat_resy, "color", glm::vec3(empty, full, empty));

    Flux::Renderer::addMesh(y_axis, mesh_resy, mat_resy);

    // Create Y Axis mesh
    Flux::Renderer::MeshRes* mz = new Flux::Renderer::MeshRes;
    mz->indices = new uint32_t[2] {0, 1};
    mz->indices_length = 2;
    mz->vertices = new Flux::Renderer::Vertex[2];
    mz->vertices[0] = Flux::Renderer::Vertex {0, 0, -10000, 0, 0, 0, 0, 0};
    mz->vertices[1] = Flux::Renderer::Vertex {0, 0, 10000, 0, 0, 0, 0, 0};
    mz->vertices_length = 2;
    mz->draw_mode = Flux::Renderer::DrawMode::Lines;
    auto mesh_resz = Flux::Resources::createResource(mz);

    // Create material
    auto mat_resz = Flux::Renderer::createMaterialResource(shaders);
    Flux::Renderer::setUniform(mat_resz, "color", glm::vec3(empty, empty, full));

    Flux::Renderer::addMesh(z_axis, mesh_resz, mat_resz);

    center = ctx->createEntity();
    Flux::Transform::giveTransform(center);

    Flux::Transform::setParent(x_axis, center);
    Flux::Transform::setParent(y_axis, center);
    Flux::Transform::setParent(z_axis, center);
}

void View3D::AxisLines::targetEntity(Flux::EntityRef entity)
{
    auto trans = Flux::Transform::getParentTransform(entity);
    center.getComponent<Flux::Transform::TransformCom>()->transformation = trans;
}

void View3D::AxisLines::setVisible(bool vis)
{
    Flux::Transform::setVisible(center, vis);
}

View3D::AxisRings::AxisRings(Flux::ECSCtx* ctx, float full, float empty)
{
    // Create Gizmos
    x_axis = ctx->createEntity();
    y_axis = ctx->createEntity();
    z_axis = ctx->createEntity();
    auto shaders = Flux::Renderer::createShaderResource("shaders/basic.vert", "shaders/basic.frag");

    const int poly_count = 36;
    const float poly_rot = 6.283185f/(float)poly_count;
    glm::mat4 rot_mat;

    // Create Ring mesh
    Flux::Renderer::MeshRes* m = new Flux::Renderer::MeshRes;
    m->indices = new uint32_t[poly_count];
    m->indices_length = poly_count;
    m->vertices = new Flux::Renderer::Vertex[poly_count];
    m->vertices_length = poly_count;
    m->draw_mode = Flux::Renderer::DrawMode::Lines;
    auto mesh_res = Flux::Resources::createResource(m);

    for (int i = 0; i < poly_count; i++)
    {
        glm::vec4 pos = rot_mat * glm::vec4(0.0, 2.5, 0.0, 1);
        // std::cout << pos.x << " " << pos.y << " " << pos.z << std::endl;
        m->vertices[i] = Flux::Renderer::Vertex {pos.x, pos.y, pos.z, 0, 0, 0, 0, 0};
        m->indices[i] = i;

        rot_mat = glm::rotate(rot_mat, poly_rot, glm::vec3(1, 0, 0));
    }

    // Create X axis
    auto mat_res = Flux::Renderer::createMaterialResource(shaders);
    Flux::Renderer::setUniform(mat_res, "color", glm::vec3(full, empty, empty));

    Flux::Renderer::addMesh(x_axis, mesh_res, mat_res);

    // Create Y axis
    auto mat_resy = Flux::Renderer::createMaterialResource(shaders);
    Flux::Renderer::setUniform(mat_resy, "color", glm::vec3(empty, full, empty));

    Flux::Renderer::addMesh(y_axis, mesh_res, mat_resy);
    Flux::Transform::rotate(y_axis, glm::vec3(0, 0, 1), 1.570796f);

    // Create Z axis
    auto mat_resz = Flux::Renderer::createMaterialResource(shaders);
    Flux::Renderer::setUniform(mat_resz, "color", glm::vec3(empty, empty, full));

    Flux::Renderer::addMesh(z_axis, mesh_res, mat_resz);
    Flux::Transform::rotate(z_axis, glm::vec3(0, 1, 0), 1.570796f);

    center = ctx->createEntity();
    Flux::Transform::giveTransform(center);

    Flux::Transform::setParent(x_axis, center);
    Flux::Transform::setParent(y_axis, center);
    Flux::Transform::setParent(z_axis, center);
}

void View3D::AxisRings::targetEntity(Flux::EntityRef entity)
{
    auto trans = Flux::Transform::getParentTransform(entity);
    center.getComponent<Flux::Transform::TransformCom>()->transformation = trans;
}

void View3D::AxisRings::setVisible(bool vis)
{
    Flux::Transform::setVisible(center, vis);
}