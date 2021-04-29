#pragma once

#include "utility/BoundingBox.hpp"
#include "utility/Vector.hpp"

namespace math
{
class Ray
{
public:
    Ray(const Vector3& start, const Vector3& end)
        : m_startPoint(start), m_endPoint(end)
    {
    }

    Ray() = default;
    ~Ray() = default;

public:
    void SetHitPoint(float distance);

    bool IntersectTriangle(const Vector3* verts, float* distance = 0) const;
    bool IntersectTriangle(const Vector3& p0, const Vector3& p1,
                           const Vector3& p2, float* distance = 0) const;

    bool IntersectBoundingBox(const BoundingBox& bbox,
                              float* distance = 0) const;

public:
    float GetLength() const { return GetVector().Length(); }

    Vector3 GetVector() const { return m_endPoint - m_startPoint; }

    Vector3 GetDirection() const { return Vector3::Normalize(GetVector()); }

    Vector3 GetHitPoint() const
    {
        return m_startPoint + GetVector() * m_hitDistance;
    }

    bool HasHit() const { return m_hitDistance < 1.0f; }

    float GetDistance() const { return m_hitDistance; }

    const Vector3& GetStartPoint() const { return m_startPoint; }

    const Vector3& GetEndPoint() const { return m_endPoint; }

private:
    Vector3 m_startPoint;
    Vector3 m_endPoint;

    float m_hitDistance = 1.0f;
};
} // namespace math