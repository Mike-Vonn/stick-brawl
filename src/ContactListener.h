#pragma once
#include "Physics.h"
#include <functional>
#include <vector>

struct CollisionEvent {
    b2BodyId bodyA;
    b2BodyId bodyB;
    b2Vec2 contactPoint = {0.0f, 0.0f};
    b2Vec2 normal = {0.0f, 0.0f};
};

// Box2D 3.x uses a polling event model instead of callback listeners.
// Call processEvents() each frame after b2World_Step to handle collisions.
class ContactListener {
public:
    using BeginCallback = std::function<void(const CollisionEvent&)>;
    using EndCallback   = std::function<void(const CollisionEvent&)>;

    void setBeginCallback(BeginCallback cb) { m_beginCallback = std::move(cb); }
    void setEndCallback(EndCallback cb)     { m_endCallback = std::move(cb); }

    // Call after b2World_Step each frame
    void processEvents(b2WorldId worldId);

private:
    BeginCallback m_beginCallback;
    EndCallback   m_endCallback;
};
