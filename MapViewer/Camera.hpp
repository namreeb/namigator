#pragma once

#include "utility/Include/LinearAlgebra.hpp"

class Camera
{
    private:
        // camera position in world coordinates
        utility::Vertex m_position;

        // camera view forward direction in world coordinate system
        utility::Vertex m_forward;

        // camera view up direction in world coordinate system
        utility::Vector3 m_up;

        // camera view right direction in world coordinate system
        utility::Vector3 m_right;

        utility::Matrix m_viewMatrix;

        void UpdateViewMatrix();

        bool m_mousePanning;
        int m_mousePanX;
        int m_mousePanY;

    public:
        Camera();

        void Move(float x, float y, float z) { Move({ x, y, z }); }
        void Move(const utility::Vertex &position);

        void LookAt(float x, float y, float z) { LookAt(utility::Vertex(x, y, z)); }
        void LookAt(const utility::Vertex &target);

        void MoveUp(float delta);
        void MoveIn(float delta);
        void MoveRight(float delta);
        void MoveVertical(float delta);

        void Yaw(float delta);
        void Pitch(float delta);

        bool IsMousePanning() const { return m_mousePanning; }

        void BeginMousePan(int screenX, int screenY);
        void EndMousePan();
        void UpdateMousePan(int newX, int newY);
        void GetMousePanStart(int &x, int &y) const;

        const utility::Matrix& GetViewMatrix() const { return m_viewMatrix; }
};