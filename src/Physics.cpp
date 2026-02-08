#include "Physics.h"

Physics::Physics() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, -20.0f};
    m_worldId = b2CreateWorld(&worldDef);
}

Physics::~Physics() {
    b2DestroyWorld(m_worldId);
}

void Physics::setGravity(float gx, float gy) {
    b2World_SetGravity(m_worldId, {gx, gy});
}

void Physics::step(float dt) {
    b2World_Step(m_worldId, dt, m_subStepCount);
}

b2BodyId Physics::createStaticBox(float cx, float cy, float halfW, float halfH, uint64_t categoryBits) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {cx, cy};
    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    b2Polygon box = b2MakeBox(halfW, halfH);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material.friction = 0.6f;
    shapeDef.filter.categoryBits = categoryBits;
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    return bodyId;
}

b2BodyId Physics::createDynamicCircle(float cx, float cy, float radius, float density,
                                       uint64_t categoryBits, uint64_t maskBits) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {cx, cy};
    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    b2Circle circle = {{0.0f, 0.0f}, radius};
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = density;
    shapeDef.material.friction = 0.3f;
    shapeDef.material.restitution = 0.1f;
    shapeDef.filter.categoryBits = categoryBits;
    shapeDef.filter.maskBits = maskBits;
    b2CreateCircleShape(bodyId, &shapeDef, &circle);

    return bodyId;
}

b2BodyId Physics::createDynamicBox(float cx, float cy, float halfW, float halfH, float density,
                                    uint64_t categoryBits, uint64_t maskBits) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {cx, cy};
    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

    b2Polygon box = b2MakeBox(halfW, halfH);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = density;
    shapeDef.material.friction = 0.3f;
    shapeDef.material.restitution = 0.1f;
    shapeDef.filter.categoryBits = categoryBits;
    shapeDef.filter.maskBits = maskBits;
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    return bodyId;
}
