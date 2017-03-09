#include "utility/Include/LinearAlgebra.hpp"
#include "Camera.hpp"

Camera::Camera()
    : m_mousePanning(false), m_mousePanX(0), m_mousePanY(0),
      m_position({ 1.f, 1.f, 1.f })
{
    LookAt(0.0f, 0.0f, 0.0f);
}

void Camera::UpdateViewMatrix()
{
    m_viewMatrix = utility::Matrix::CreateViewMatrix(m_position, m_position + m_forward, m_up);
}

void Camera::Move(const utility::Vertex &position)
{
    m_position = position;

    UpdateViewMatrix();
}

void Camera::LookAt(const utility::Vertex &target)
{
    m_forward = utility::Vector3::Normalize(target - m_position);
    m_right = utility::Vector3::Normalize(utility::Vector3::CrossProduct(m_forward, { 0.f, 0.f, 1.f }));
    m_up = utility::Vector3::Normalize(utility::Vector3::CrossProduct(m_right, m_forward));

    UpdateViewMatrix();
}

void Camera::MoveUp(float delta)
{
    m_position += delta*m_up;

    UpdateViewMatrix();
}

void Camera::MoveIn(float delta)
{
    m_position += delta*m_forward;
    UpdateViewMatrix();
}

void Camera::MoveRight(float delta)
{
    m_position += delta*m_right;
    UpdateViewMatrix();
}

void Camera::MoveVertical(float delta)
{
    m_position.Z += delta;
    UpdateViewMatrix();
}

void Camera::Yaw(float delta)
{
    auto const rotate = utility::Matrix::CreateRotationZ(delta);
    m_forward = utility::Vector3::Normalize(utility::Vector3::Transform(m_forward, rotate));
    m_right = utility::Vector3::Normalize(utility::Vector3::Transform(m_right, rotate));
    m_up = utility::Vector3::Normalize(utility::Vector3::CrossProduct(m_right, m_forward));

    UpdateViewMatrix();
}

void Camera::Pitch(float delta)
{
    auto const rotate = utility::Matrix::CreateRotation(m_right, delta);

    auto newUp = utility::Vector3::Transform(m_up, rotate);

    if (newUp.Z < 0.0f)
        newUp.Z = 0.0f;

    m_up = utility::Vector3::Normalize(newUp);
    m_forward = utility::Vector3::Normalize(utility::Vector3::CrossProduct(m_up, m_right));

    UpdateViewMatrix();
}

void Camera::BeginMousePan(int screenX, int screenY)
{
    m_mousePanning = true;
    m_mousePanX = screenX;
    m_mousePanY = screenY;
}

void Camera::EndMousePan()
{
    m_mousePanning = false;
}

void Camera::UpdateMousePan(int newX, int newY)
{
    const int deltaX = newX - m_mousePanX;
    const int deltaY = newY - m_mousePanY;

    if (abs(deltaX) > 1)
        Yaw(-0.003f * static_cast<float>(deltaX));

    if (abs(deltaY) > 1)
        Pitch(-0.003f * static_cast<float>(deltaY));
}

void Camera::GetMousePanStart(int &x, int &y) const
{
    x = m_mousePanX;
    y = m_mousePanY;
}

void Camera::UpdateProjection(float vpX, float vpY, float width, float height, float minDepth, float maxDepth)
{
    m_viewportX = vpX;
    m_viewportY = vpY;

    m_viewportWidth = width;
    m_viewportHeight = height;

    m_viewportMinDepth = minDepth;
    m_viewportMaxDepth = maxDepth;

    m_projMatrix = utility::Matrix::CreateProjectionMatrix(PI / 4.f, width / height, 1.f, 10000.f);
}

utility::Vector3 Camera::ProjectPoint(const utility::Vector3& pos) const
{
    utility::Vector3 position = pos;
    position = utility::Vector3::Transform(position, m_viewMatrix.Transposed());
    position = utility::Vector3::Transform(position, m_projMatrix.Transposed());

    position.X = m_viewportX + (1.0f + position.X) * m_viewportWidth / 2.0f;
    position.Y = m_viewportY + (1.0f - position.Y) * m_viewportHeight / 2.0f;
    position.Z = m_viewportMinDepth + position.Z * (m_viewportMaxDepth - m_viewportMinDepth);
    return position;
}

utility::Vector3 Camera::UnprojectPoint(const utility::Vector3& pos) const
{
    utility::Vector3 position = pos;
    position.X = 2.0f * (position.X - m_viewportX) / m_viewportWidth - 1.0f;
    position.Y = 1.0f - 2.0f * (position.Y - m_viewportY) / m_viewportHeight;
    position.Z = (position.Z - m_viewportMinDepth) / (m_viewportMaxDepth - m_viewportMinDepth);

    position = utility::Vector3::Transform(position, m_projMatrix.ComputeInverse().Transposed());
    position = utility::Vector3::Transform(position, m_viewMatrix.ComputeInverse().Transposed());
    return position;
}
