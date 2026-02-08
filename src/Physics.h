#pragma once
#include <box2d/box2d.h>

// Pixels-to-meters conversion
constexpr float PPM = 30.0f;
inline float toMeters(float px) { return px / PPM; }
inline float toPixels(float m)  { return m * PPM; }

// Collision categories
enum CollisionCategory : uint64_t {
    CAT_PLATFORM   = 0x0001,
    CAT_PLAYER     = 0x0002,
    CAT_PROJECTILE = 0x0004,
    CAT_PICKUP     = 0x0008,
};

class Physics {
public:
    Physics();
    ~Physics();

    void setGravity(float gx, float gy);
    void step(float dt);

    b2WorldId getWorldId() const { return m_worldId; }

    // Helper to create a static platform box (center x/y, half-extents)
    b2BodyId createStaticBox(float cx, float cy, float halfW, float halfH, uint64_t categoryBits = CAT_PLATFORM);

    // Helper to create dynamic bodies
    b2BodyId createDynamicCircle(float cx, float cy, float radius, float density = 1.0f,
                                  uint64_t categoryBits = CAT_PLAYER, uint64_t maskBits = 0xFFFFFFFF);
    b2BodyId createDynamicBox(float cx, float cy, float halfW, float halfH, float density = 1.0f,
                               uint64_t categoryBits = CAT_PLAYER, uint64_t maskBits = 0xFFFFFFFF);

private:
    b2WorldId m_worldId;
    int m_subStepCount = 4;
};
