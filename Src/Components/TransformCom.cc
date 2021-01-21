#include "Flux/Input.hh"
#include "Flux/Resources.hh"
#include "FluxED/3DView.hh"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_FORCE_CTOR_INIT
#include "Flux/ECS.hh"
#include "Flux/Log.hh"
#include "Flux/Renderer.hh"
#include "FluxED/FluxEd.hh"
#include "glibmm/refptr.h"
#include "glm/gtc/quaternion.hpp"
#include "gtkmm/adjustment.h"
#include "gtkmm/checkbutton.h"
#include "gtkmm/spinbutton.h"
#include "sigc++/functors/mem_fun.h"
#include <glm/gtx/matrix_decompose.hpp>
// static Glib::RefPtr<class Gtk::Builder> refBuilder;
// static Gtk::Box* widget = nullptr;

enum DoMode
{
    Nothing,
    Grab,
    Rotate,
    Scale
};

class TransformComPlugin: public FluxED::ComPlugin
{
public:
    TransformComPlugin() {};

    void init(FluxED::Editor3D* be) override;

    void removed() override;

    Gtk::Widget* getForEntity(Flux::EntityRef e) override;

    void loop(float delta) override;

    // Callbacks :(
    void onVisibleToggled();

    /** Read the transform from the GUI and apply it to the active entity */
    void updateTransform();

private:

    /** Set all the spinboxes to be the values in the "values" section */
    void resetDisplay();

    Gtk::Widget* widget;
    Glib::RefPtr<Gtk::Builder> refBuilder;

    std::vector<Glib::RefPtr<Gtk::Adjustment> > inputs;

    Gtk::CheckButton* visible;

    FluxED::Editor3D* editor;

    DoMode mode;
    glm::vec3 axis;

    // Values
    Flux::EntityRef entity;
    bool has_entity;
    bool is_moving;

    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale;
    bool is_visible;

    // Gizmos
    FluxED::View3D::AxisLines* center_lines;
    FluxED::View3D::AxisLines* object_lines;

    FluxED::View3D::AxisRings* object_rings;
};

void TransformComPlugin::init(FluxED::Editor3D* be)
{
    center_lines = new FluxED::View3D::AxisLines(be->gizmo_ecs);
    object_lines = new FluxED::View3D::AxisLines(be->gizmo_ecs, 0.8, 0.4);
    object_lines->setVisible(false);
    object_rings = new FluxED::View3D::AxisRings(be->gizmo_ecs);
    object_rings->setVisible(false);

    has_entity = false;
    is_moving = false;
    editor = be;
    axis = glm::vec3(0, 0, 0);
    if (widget == nullptr)
    {
        refBuilder = Gtk::Builder::create();
        try
        {
            refBuilder->add_from_file("Components/Transform.glade");
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
        
        refBuilder->get_widget("Transform", widget);
        widget->show();

        // Load "inputs"
        std::vector<std::string> names = {"TX", "TY", "TZ",
                                          "RX", "RY", "RZ",
                                          "SX", "SY", "SZ"};
        
        for (auto input : names)
        {
            Gtk::SpinButton* sb = nullptr;
            refBuilder->get_widget(input, sb);
            auto ad = Gtk::Adjustment::create(0, -10000, 10000, 0.01);
            inputs.push_back(ad);

            sb->set_adjustment(ad);
            sb->set_digits(3);
            sb->signal_changed().connect(sigc::mem_fun(this, &TransformComPlugin::updateTransform));
        }

        refBuilder->get_widget("Visible", visible);

        // Signals
        visible->signal_toggled().connect(sigc::mem_fun(this, &TransformComPlugin::onVisibleToggled));
    }
}

void TransformComPlugin::removed()
{
    has_entity = false;
}

void TransformComPlugin::loop(float delta)
{
    if (has_entity)
    {
        switch (mode)
        {
            case Nothing:
            {
                if (Flux::Input::isKeyPressed(FLUX_KEY_G))
                {
                    mode = Grab;
                    axis = glm::vec3(editor->orbit_cam->getViewMatrix() * glm::vec4(0, 0, 0, 1));
                    is_moving = true;
                    Flux::Input::setMouseMode(Flux::Input::MouseMode::Confined);
                }
                else if (Flux::Input::isKeyPressed(FLUX_KEY_R))
                {
                    mode = Rotate;
                    axis = glm::vec3(editor->orbit_cam->getViewMatrix() * glm::vec4(0, 0, 0, 1));
                    is_moving = true;
                    Flux::Input::setMouseMode(Flux::Input::MouseMode::Confined);
                }

                // TODO: Fix
                // else if (Flux::Input::isKeyPressed(FLUX_KEY_S))
                // {
                //     mode = Scale;
                //     axis = glm::vec3(Flux::Input::getMousePosition(), 1);
                //     is_moving = true;
                //     Flux::Input::setMouseMode(Flux::Input::MouseMode::Confined);
                // }

                break;
            }

            case Grab:
            {
                if (Flux::Input::isKeyPressed(FLUX_KEY_ESCAPE) || Flux::Input::isMouseButtonPressed(FLUX_MOUSE_BUTTON_1))
                {
                    mode = Nothing;
                    axis = glm::vec3(0, 0, 0);
                    is_moving = false;
                    Flux::Input::setMouseMode(Flux::Input::MouseMode::Free);
                    return;
                }

                if (Flux::Input::isKeyPressed(FLUX_KEY_X))
                {
                    axis = glm::vec3(1, 0, 0);
                }

                if (Flux::Input::isKeyPressed(FLUX_KEY_Y))
                {
                    axis = glm::vec3(0, 1, 0);
                }

                if (Flux::Input::isKeyPressed(FLUX_KEY_Z))
                {
                    axis = glm::vec3(0, 0, 1);
                }

                Flux::Transform::translate(entity, axis * (Flux::Input::getMouseOffset().x * -0.01f));
                resetDisplay();

                break;
            }

            case Rotate:
            {
                if (Flux::Input::isKeyPressed(FLUX_KEY_ESCAPE) || Flux::Input::isMouseButtonPressed(FLUX_MOUSE_BUTTON_1))
                {
                    mode = Nothing;
                    axis = glm::vec3(0, 0, 0);
                    is_moving = false;
                    resetDisplay();
                    Flux::Input::setMouseMode(Flux::Input::MouseMode::Free);
                    return;
                }

                if (Flux::Input::isKeyPressed(FLUX_KEY_X))
                {
                    axis = glm::vec3(1, 0, 0);
                }

                if (Flux::Input::isKeyPressed(FLUX_KEY_Y))
                {
                    axis = glm::vec3(0, 1, 0);
                }

                if (Flux::Input::isKeyPressed(FLUX_KEY_Z))
                {
                    axis = glm::vec3(0, 0, 1);
                }

                Flux::Transform::rotate(entity, axis , Flux::Input::getMouseOffset().x * -0.01f);
                resetDisplay();

                break;
            }

            case Scale:
            {
                if (Flux::Input::isKeyPressed(FLUX_KEY_ESCAPE) || Flux::Input::isMouseButtonPressed(FLUX_MOUSE_BUTTON_1))
                {
                    mode = Nothing;
                    axis = glm::vec3(0, 0, 0);
                    is_moving = false;
                    resetDisplay();
                    Flux::Input::setMouseMode(Flux::Input::MouseMode::Free);
                    return;
                }

                int scalar = Flux::Input::getMouseOffset().x * 0.1f;

                if (scalar != 0)
                {
                    Flux::Transform::scale(entity, glm::vec3(scalar));
                }
                resetDisplay();

                break;
            }
        }
    }
}

Gtk::Widget* TransformComPlugin::getForEntity(Flux::EntityRef e)
{
    if (e.hasComponent<Flux::Transform::TransformCom>())
    {
        entity = e;

        resetDisplay();

        mode = Nothing;

        has_entity = true;

        object_lines->targetEntity(e);
        object_lines->setVisible(true);
        object_rings->targetEntity(e);
        object_rings->setVisible(true);

        return widget;
    }
    else
    {
        object_lines->setVisible(false);
        object_rings->setVisible(false);
        return nullptr;
    }
}

void TransformComPlugin::resetDisplay()
{
    // This is the local transformation
    // TODO: Global transformation
    auto tc = entity.getComponent<Flux::Transform::TransformCom>();

    // Load in transformation
    glm::quat rotation_q;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(tc->transformation, scale, rotation_q, translation, skew, perspective);

    // Apparently the quaternion is incorrect...?
    rotation_q = glm::conjugate(rotation_q);
    rotation = glm::eulerAngles(rotation_q);

    is_visible = tc->visible;

    inputs[0]->set_value(translation.x);
    inputs[1]->set_value(translation.y);
    inputs[2]->set_value(translation.z);

    inputs[3]->set_value(glm::degrees(rotation.x));
    inputs[4]->set_value(glm::degrees(rotation.y));
    inputs[5]->set_value(glm::degrees(rotation.z));

    inputs[6]->set_value(scale.x);
    inputs[7]->set_value(scale.y);
    inputs[8]->set_value(scale.z);

    visible->set_active(is_visible);

    // Set the ui thing it!
    object_lines->targetEntity(entity);
    object_rings->targetEntity(entity);
}

void TransformComPlugin::onVisibleToggled()
{
    is_visible = visible->get_active();
    auto tc = entity.getComponent<Flux::Transform::TransformCom>();
    tc->visible = is_visible;
}

void TransformComPlugin::updateTransform()
{
    if (!has_entity || is_moving)
    {
        return;
    }

    // Reset the entity's transformation
    auto tc = entity.getComponent<Flux::Transform::TransformCom>();
    tc->transformation = glm::mat4();

    // Set the random variables it!
    translation = glm::vec3(inputs[0]->get_value(),
                            inputs[1]->get_value(),
                            inputs[2]->get_value());
    
    rotation = glm::vec3(glm::radians(inputs[3]->get_value()),
                         glm::radians(inputs[4]->get_value()),
                         glm::radians(inputs[5]->get_value()));

    scale = glm::vec3(inputs[6]->get_value(),
                            inputs[7]->get_value(),
                            inputs[8]->get_value());

    // Rotate it!
    // We're rotating X-Y-Z Euler
    Flux::Transform::rotate(entity, glm::vec3(1, 0, 0), rotation.x);
    Flux::Transform::rotate(entity, glm::vec3(0, 1, 0), rotation.y);
    Flux::Transform::rotate(entity, glm::vec3(0, 0, 1), rotation.z);

    // Translate it!
    Flux::Transform::translate(entity, translation);

    // Scale it!
    Flux::Transform::scale(entity, scale);

    // Set the ui thing it!
    object_lines->targetEntity(entity);
}

static bool a = FluxED::addComponentPlugin("TransformCom", new TransformComPlugin);