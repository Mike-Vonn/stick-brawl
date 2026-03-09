#include "ContactListener.h"

void ContactListener::processEvents(b2WorldId worldId) {
    // Box2D 3.x: poll contact events after each world step
    b2ContactEvents events = b2World_GetContactEvents(worldId);

    // Process begin-touch events
    for (int i = 0; i < events.beginCount; i++) {
        const b2ContactBeginTouchEvent& e = events.beginEvents[i];

        b2BodyId bodyA = b2Shape_GetBody(e.shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(e.shapeIdB);

        if (m_beginCallback) {
            CollisionEvent evt;
            evt.bodyA = bodyA;
            evt.bodyB = bodyB;
            if (e.manifold.pointCount > 0) {
                evt.contactPoint = e.manifold.points[0].point;
                evt.normal = e.manifold.normal;
            }
            m_beginCallback(evt);
        }
    }

    // Process end-touch events
    for (int i = 0; i < events.endCount; i++) {
        const b2ContactEndTouchEvent& e = events.endEvents[i];

        b2BodyId bodyA = b2Shape_GetBody(e.shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(e.shapeIdB);

        if (m_endCallback) {
            CollisionEvent evt;
            evt.bodyA = bodyA;
            evt.bodyB = bodyB;
            m_endCallback(evt);
        }
    }
}
