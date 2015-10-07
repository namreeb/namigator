#pragma once

#include "LinearAlgebra.hpp"

class Camera
{
    private:
        const utility::Vertex m_up = { 0.f, 1.f, 0.f };
        const utility::Matrix<float> m_projectionMatrix;

        utility::Vertex m_position;
        utility::Vertex m_lookAt;

        float m_viewProjectionMatrix[16];

        void UpdateViewProjectionMatrix();

    public:
        Camera();

        void Move(float x, float y, float z);
        void MoveVertical(float delta);

        void LookAt(float x, float y, float z);

        // FIXME: These will be removed once I figure out how I want the camera to work
        void LookAtLeftRight(float delta);
        void LookAtFrontBack(float delta);
        void LookAtUpDown(float delta);

        inline const float *GetProjectionMatrix() const { return m_viewProjectionMatrix; }
};