#pragma once

#include "utility/Vector.hpp"
#include "utility/Matrix.hpp"

class Camera
{
    private:
        // camera position in world coordinates
        math::Vertex m_position;

        // camera view forward direction in world coordinate system
        math::Vertex m_forward;

        // camera view up direction in world coordinate system
        math::Vector3 m_up;

        // camera view right direction in world coordinate system
        math::Vector3 m_right;

        math::Matrix m_viewMatrix;
        math::Matrix m_projMatrix;

        void UpdateViewMatrix();

        bool m_mousePanning;
        int m_mousePanX;
        int m_mousePanY;

        float m_viewportX;
        float m_viewportY;

        float m_viewportWidth;
        float m_viewportHeight;

        float m_viewportMinDepth;
        float m_viewportMaxDepth;

    public:
        Camera();

        void Move(float x, float y, float z) { Move({ x, y, z }); }
        void Move(const math::Vertex &position);

        void LookAt(float x, float y, float z) { LookAt(math::Vertex(x, y, z)); }
        void LookAt(const math::Vertex &target);

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

        void UpdateProjection(float vpX, float vpY, float width, float height, float minDepth, float maxDepth);

        math::Vertex ProjectPoint(const math::Vector3& pos) const;
        math::Vertex UnprojectPoint(const math::Vector3& pos) const;

        const math::Vector3& GetPosition() const { return m_position; }
        const math::Vector3& GetForward() const { return m_forward; }
        const math::Matrix& GetViewMatrix() const { return m_viewMatrix; }
        const math::Matrix& GetProjMatrix() const { return m_projMatrix; }
};