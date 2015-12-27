#pragma once

#include "LinearAlgebra.hpp"

class Camera
{
    private:
        const utility::Matrix m_projectionMatrix;

        // camera position in world coordinates
        utility::Vertex m_position;

        // point looked at by camera in world coordinates
        utility::Vertex m_target;

        // camera view up direction in world coordinate system
        utility::Vector3 m_up;

        float m_viewProjectionMatrix[16];

        void UpdateViewProjectionMatrix();

    public:
        Camera();

        void Move(float x, float y, float z);

        void LookAt(float x, float y, float z) { LookAt(utility::Vertex(x, y, z)); }
        void LookAt(const utility::Vertex &target);

        void MoveVertical(float delta);

        void Yaw(float delta);
        void Pitch(float delta);

        inline const float *GetProjectionMatrix() const { return m_viewProjectionMatrix; }
};