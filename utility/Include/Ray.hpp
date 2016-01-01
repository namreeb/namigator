#pragma once

#include "LinearAlgebra.hpp"
#include "BoundingBox.hpp"

namespace utility
{
    class Ray {
    public:
        Ray(const Vector3& start, const Vector3& end)
            : m_startPoint(start)
            , m_endPoint(end) {
        }

        Ray() = default;
        ~Ray() = default;

    public:
        void setHitPoint(float distance);

        bool intersectTriangle(const Vector3* verts, float* distance = 0) const;
        bool intersectBoundingBox(const BoundingBox& bbox, float* distance = 0) const;

    public:
        float getLength() const {
            return getVector().Length();
        }

        Vector3 getVector() const {
            return m_endPoint - m_startPoint;
        }

        Vector3 getDirection() const {
            return Vector3::Normalize(getVector());
        }

        Vector3 getHitPoint() const {
            return m_startPoint + getVector() * m_hitDistance;
        }

        bool hasHit() const {
            return m_hitDistance < 1.0f;
        }

        float getDistance() const {
            return m_hitDistance;
        }

    private:
        Vector3 m_startPoint;
        Vector3 m_endPoint;

        float m_hitDistance = 1.0f;
    };
}
