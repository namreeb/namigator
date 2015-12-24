#pragma once

#include "LinearAlgebra.hpp"

class Camera
{
    private:
        const utility::Vertex m_up = { 0.f, 1.f, 0.f };
        const utility::Matrix m_projectionMatrix;

        utility::Vertex m_position;
        utility::Vertex m_lookNormal;

        float m_viewProjectionMatrix[16];

        void UpdateViewProjectionMatrix();

    public:
        Camera();

        void Move(float x, float y, float z);
        void MoveVertical(float delta);

        void RotateAroundZ(float delta);

        inline const float *GetProjectionMatrix() const { return m_viewProjectionMatrix; }
};