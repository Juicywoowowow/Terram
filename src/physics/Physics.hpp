#pragma once

namespace terram {

class Physics {
public:
    // Collision detection
    static bool rectanglesOverlap(float x1, float y1, float w1, float h1,
                                   float x2, float y2, float w2, float h2);

    static bool circlesOverlap(float x1, float y1, float r1,
                                float x2, float y2, float r2);

    static bool pointInRectangle(float px, float py,
                                  float rx, float ry, float rw, float rh);

    static bool pointInCircle(float px, float py,
                               float cx, float cy, float r);

    static bool rectangleCircleOverlap(float rx, float ry, float rw, float rh,
                                        float cx, float cy, float cr);

    // Distance utilities
    static float distance(float x1, float y1, float x2, float y2);
    static float distanceSquared(float x1, float y1, float x2, float y2);
};

} // namespace terram
