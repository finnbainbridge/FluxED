#include "Flux/ECS.hh"
#include "Flux/Renderer.hh"
#include "FluxED/3DView.hh"
#include "FluxED/FluxEd.hh"
#include "gtkmm/adjustment.h"
#include "gtkmm/widget.h"

class LightComponentPlugin : public FluxED::ComPlugin
{
public:
    LightComponentPlugin() {};

    void init(FluxED::Editor3D* be) override;

    void removed() override {loaded = false;};

    Gtk::Widget* getForEntity(Flux::EntityRef e) override;

    void loop(float delta) override;

    // Callback
    void updateValues();

private:
    Gtk::Widget* widget;
    Glib::RefPtr<Gtk::Builder> refBuilder;

    std::vector<Glib::RefPtr<Gtk::Adjustment> > rgb_values;

    Glib::RefPtr<Gtk::Adjustment> radius_value;

    Flux::EntityRef current_entity;

    FluxED::Editor3D* editor;

    bool loaded;
};

void LightComponentPlugin::init(FluxED::Editor3D *be)
{
    editor = be;
    refBuilder = Gtk::Builder::create();
    try
    {
        refBuilder->add_from_file("Components/Light.glade");
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
    
    refBuilder->get_widget("LightCom", widget);
    widget->show();

    rgb_values.resize(3);

    rgb_values[0] = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(refBuilder->get_object("AdjR"));

    rgb_values[1] = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(refBuilder->get_object("AdjG"));

    rgb_values[2] = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(refBuilder->get_object("AdjB"));

    radius_value = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(refBuilder->get_object("Radius"));

    radius_value->signal_value_changed().connect(sigc::mem_fun(this, &LightComponentPlugin::updateValues));

    rgb_values[0]->signal_value_changed().connect(sigc::mem_fun(this, &LightComponentPlugin::updateValues));
    rgb_values[1]->signal_value_changed().connect(sigc::mem_fun(this, &LightComponentPlugin::updateValues));
    rgb_values[2]->signal_value_changed().connect(sigc::mem_fun(this, &LightComponentPlugin::updateValues));
}

Gtk::Widget* LightComponentPlugin::getForEntity(Flux::EntityRef e)
{
    if (!e.hasComponent<Flux::Renderer::LightCom>())
    {
        return nullptr;
    }

    // Load in values
    auto lc = e.getComponent<Flux::Renderer::LightCom>();
    
    rgb_values[0]->set_value(lc->color.r);
    rgb_values[1]->set_value(lc->color.g);
    rgb_values[2]->set_value(lc->color.b);

    radius_value->set_value(lc->radius);

    current_entity = e;
    loaded = true;

    return widget;
}

void LightComponentPlugin::updateValues()
{
    if (!loaded)
    {
        return;
    }

    auto lc = current_entity.getComponent<Flux::Renderer::LightCom>();

    lc->color = glm::vec3(rgb_values[0]->get_value(), rgb_values[1]->get_value(), rgb_values[2]->get_value());
    lc->radius = radius_value->get_value();

    // Set the transform to changed to force a light update
    current_entity.getComponent<Flux::Transform::TransformCom>()->has_changed = true;
}

struct LCPTagCom : public Flux::Component
{
    FLUX_COMPONENT(LCPTagCom, LCPTagCom);

    FluxED::View3D::AxisRings* radius_identifier;

    ~LCPTagCom()
    {
        delete radius_identifier;
    }
};

void LightComponentPlugin::loop(float delta)
{
    // for (auto i : editor->entities)
    // {
    //     if (i.hasComponent<Flux::Renderer::LightCom>())
    //     {
    //         if (!i.hasComponent<LCPTagCom>())
    //         {
    //             auto lcpc = new LCPTagCom;
    //             lcpc->radius_identifier = new FluxED::View3D::AxisRings(editor->gizmo_ecs);
    //             i.addComponent(lcpc);
    //         }

    //         auto lcpc = i.getComponent<LCPTagCom>();
    //         lcpc->radius_identifier->targetEntity(i);

    //         // TODO: Make show radius
    //     }
    // }
}

static bool l = FluxED::addPreset("Light", "Presets/Light.farc");
static bool a = FluxED::addComponentPlugin("LightCom", new LightComponentPlugin);