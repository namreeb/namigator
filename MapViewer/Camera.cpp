#include "LinearAlgebra.hpp"
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
        Yaw(-0.002f * static_cast<float>(deltaX));

    if (abs(deltaY) > 1)
        Pitch(-0.002f * static_cast<float>(deltaY));
}

void Camera::GetMousePanStart(int &x, int &y) const
{
    x = m_mousePanX;
    y = m_mousePanY;
}