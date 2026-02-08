#include "ContactListener.h"
#include "StickFigure.h"

void ContactListener::BeginContact(b2Contact* contact) {
    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    // Handle ground sensor contacts for jump detection
    bool sensorA = fixtureA->IsSensor();
    bool sensorB = fixtureB->IsSensor();

    if (sensorA || sensorB) {
        b2Fixture* sensorFixture = sensorA ? fixtureA : fixtureB;
        b2Fixture* otherFixture = sensorA ? fixtureB : fixtureA;

        // Check if the other fixture is a platform
        if (otherFixture->GetFilterData().categoryBits & CAT_PLATFORM) {
            b2Body* sensorBody = sensorFixture->GetBody();
            auto* player = reinterpret_cast<StickFigure*>(sensorBody->GetUserData().pointer);
            if (player) {
                player->addGroundContact();
            }
        }
    }

    if (m_beginCallback) {
        CollisionEvent event;
        event.bodyA = fixtureA->GetBody();
        event.bodyB = fixtureB->GetBody();

        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        event.contactPoint = manifold.points[0];
        event.normal = manifold.normal;

        m_beginCallback(event);
    }
}

void ContactListener::EndContact(b2Contact* contact) {
    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    bool sensorA = fixtureA->IsSensor();
    bool sensorB = fixtureB->IsSensor();

    if (sensorA || sensorB) {
        b2Fixture* sensorFixture = sensorA ? fixtureA : fixtureB;
        b2Fixture* otherFixture = sensorA ? fixtureB : fixtureA;

        if (otherFixture->GetFilterData().categoryBits & CAT_PLATFORM) {
            b2Body* sensorBody = sensorFixture->GetBody();
            auto* player = reinterpret_cast<StickFigure*>(sensorBody->GetUserData().pointer);
            if (player) {
                player->removeGroundContact();
            }
        }
    }

    if (m_endCallback) {
        CollisionEvent event;
        event.bodyA = fixtureA->GetBody();
        event.bodyB = fixtureB->GetBody();
        m_endCallback(event);
    }
}
