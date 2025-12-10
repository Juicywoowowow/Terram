#include "Physics.hpp"
#include <cmath>
#include <algorithm>

namespace terram {

bool Physics::rectanglesOverlap(float x1, float y1, float w1, float h1,
                                 float x2, float y2, float w2, float h2) {
    return x1 < x2 + w2 && x1 + w1 > x2 &&
           y1 < y2 + h2 && y1 + h1 > y2;
}

bool Physics::circlesOverlap(float x1, float y1, float r1,
                              float x2, float y2, float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float radiusSum = r1 + r2;
    return (dx * dx + dy * dy) <= (radiusSum * radiusSum);
}

bool Physics::pointInRectangle(float px, float py,
                                float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw &&
           py >= ry && py <= ry + rh;
}

bool Physics::pointInCircle(float px, float py,
                             float cx, float cy, float r) {
    float dx = px - cx;
    float dy = py - cy;
    return (dx * dx + dy * dy) <= (r * r);
}

bool Physics::rectangleCircleOverlap(float rx, float ry, float rw, float rh,
                                      float cx, float cy, float cr) {
    // Find closest point on rectangle to circle center
    float closestX = std::clamp(cx, rx, rx + rw);
    float closestY = std::clamp(cy, ry, ry + rh);

    // Check distance from closest point to circle center
    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx * dx + dy * dy) <= (cr * cr);
}

float Physics::distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

float Physics::distanceSquared(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

} // namespace terram
