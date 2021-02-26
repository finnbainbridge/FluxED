#ifndef FLUXED_3DVIEW_HH
#define FLUXED_3DVIEW_HH

#include "Flux/ECS.hh"
#include <Flux/Flux.hh>

namespace FluxED 
{
namespace View3D
{
    /**
    A small orbit camera class. Easy to setup and dump in any Flux Scene.
    This class needs a renderer, and transform systems
    */
    class OrbitCamera
    {
    public:
        OrbitCamera();

        /** Add the orbit camera to another ECSCtx. These should all be done before the first loop */
        void addToCtx(Flux::ECSCtx* ctx);

        /** Tells the OrbitCamera to do it's job. Should be called every frame */
        void loop(float delta);

        /** Sets the center position of the orbit camera (what it orbits around) */
        void setPosition(glm::vec3 pos);

        /** Destroyes the OrbitCamera's entities */
        void destroy();

        glm::mat4 getViewMatrix();

    private:
        // Flux::ECSCtx* ctx;

        std::vector<Flux::EntityRef> centers;
        std::vector<Flux::EntityRef> cameras;
    };

    /**
    An X-line, Y-line, and Z-line that can be moved 
    around and stuck to entites - even if those entities
    are in a different ecsctx!

    Requires Shaders from FluxED: basic.vert and basic.frag
    */
    class AxisLines
    {
    public:
        AxisLines(Flux::ECSCtx* ctx, float full = 1, float empty = 0);

        void targetEntity(Flux::EntityRef entity);

        void setVisible(bool vis);
    
    private:

        Flux::EntityRef center;
        Flux::EntityRef x_axis;
        Flux::EntityRef y_axis;
        Flux::EntityRef z_axis;
    };

    /**
    Circles on the X, Y and Z axis
    Stuck to entites - even if those entities
    are in a different ecsctx!

    Requires Shaders from FluxED: basic.vert and basic.frag
    */
    class AxisRings
    {
    public:
        AxisRings(Flux::ECSCtx* ctx, float full = 1, float empty = 0);

        void targetEntity(Flux::EntityRef entity);

        void setVisible(bool vis);

        /* Parent entity and center of thing */
        Flux::EntityRef center;
    private:

        Flux::EntityRef x_axis;
        Flux::EntityRef y_axis;
        Flux::EntityRef z_axis;
    };
}
}

#endif