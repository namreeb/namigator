#include "Ray.hpp"

#include <math.h>
#include <assert.h>

#include <algorithm>

namespace utility
{
void Ray::setHitPoint(float distance) {
    assert(distance >= 0.0f);
    assert(distance <= 1.0f);

    m_hitDistance = distance;
}

bool Ray::intersectTriangle(const Vector3* verts, float* distance) const {
    return intersectTriangle(verts[0], verts[1], verts[2], distance);
}

bool Ray::intersectTriangle(const Vector3& p0, const Vector3& p1, const Vector3& p2, float* distance) const {
    // To increase precision for very small world scales
    const float upscaleFactor = 10.0f;

    Vector3 rayDir = getDirection() * upscaleFactor;
    Vector3 v0 = p0 * upscaleFactor;
    Vector3 v1 = p1 * upscaleFactor - v0;
    Vector3 v2 = p2 * upscaleFactor - v0;

    Vector3 p = Vector3::CrossProduct(rayDir, v2);
    float det = Vector3::DotProduct(v1, p);

    if (fabsf(det) < 1e-5) {
        return false;
    }

    Vector3 t = m_startPoint * upscaleFactor - v0;
    float e1 = Vector3::DotProduct(t, p) / det;

    if (e1 < 0.0f || e1 > 1.0f) {
        return false;
    }

    Vector3 q = Vector3::CrossProduct(t, v1);
    float e2 = Vector3::DotProduct(rayDir, q) / det;

    if (e2 < 0.0f || (e1 + e2) > 1.0f) {
        return false;
    }

    float d = Vector3::DotProduct(v2, q) / det;
    if (d < 1e-5) {
        return false;
    }

    if (distance) {
        *distance = d / getLength();
    }
    return true;
}

bool Ray::intersectBoundingBox(const BoundingBox& bbox, float* distance) const {
    Vector3 rayDir = getDirection();
    Vector3 invDir{
        /* x = */ 1.0f / rayDir.X,
        /* y = */ 1.0f / rayDir.Y,
        /* z = */ 1.0f / rayDir.Z
    };

    Vector3 minimum = bbox.getMinimum();
    Vector3 maximum = bbox.getMaximum();

    float t1 = (minimum.X - m_startPoint.X) * invDir.X;
    float t2 = (maximum.X - m_startPoint.X) * invDir.X;
    float t3 = (minimum.Y - m_startPoint.Y) * invDir.Y;
    float t4 = (maximum.Y - m_startPoint.Y) * invDir.Y;
    float t5 = (minimum.Z - m_startPoint.Z) * invDir.Z;
    float t6 = (maximum.Z - m_startPoint.Z) * invDir.Z;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
    if (tmax < 0) {
        return false;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return false;
    }

    if (distance) {
        *distance = tmin / getLength();
    }
    return true;
}
}