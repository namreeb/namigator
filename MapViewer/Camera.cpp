#include "LinearAlgebra.hpp"
#include "Camera.hpp"

Camera::Camera()
    : m_projectionMatrix(utility::Matrix::CreateProjectionMatrix(PI / 4.f, 1200.f/800.f, 0.1f, 10000.f)),
      m_mousePanning(false), m_mousePanX(0), m_mousePanY(0),
      m_position({ 0.f,  0.f,  0.f }),
      m_target(  { 0.f,  0.f, -1.f }),
      m_up(      { 1.f,  0.f,  0.f }),
      m_right(   { 0.f, -1.f,  0.f })
{}

void Camera::UpdateViewProjectionMatrix()
{
    auto const viewProjection = utility::Matrix::CreateViewMatrix(m_position, m_target, m_up) * m_projectionMatrix;

    viewProjection.PopulateArray(m_viewProjectionMatrix);
}

void Camera::Move(float x, float y, float z)
{
    m_position = { x, y, z };

    UpdateViewProjectionMatrix();
}

void Camera::LookAt(const utility::Vertex &target)
{
    m_target = target;

    auto const forward = utility::Vector3::Normalize(m_target - m_position);

    m_right = utility::Vector3::Normalize(utility::Vector3::CrossProduct(forward, { 0.f, 0.f, 1.f }));
    m_up = utility::Vector3::Normalize(utility::Vector3::CrossProduct(m_right, forward));

    UpdateViewProjectionMatrix();
}

void Camera::MoveVertical(float delta)
{
    m_position.Z += delta;
    m_target.Z += delta;

    UpdateViewProjectionMatrix();
}

void Camera::MoveIn(float delta)
{
    auto const direction = utility::Vector3::Normalize(m_target - m_position);
    
    m_position += delta*direction;

    UpdateViewProjectionMatrix();
}

void Camera::MoveRight(float delta)
{
    m_position += delta*m_right;
    m_target += delta*m_right;

    UpdateViewProjectionMatrix();
}

void Camera::Yaw(float delta)
{
    auto const rotate = utility::Matrix::CreateRotationZ(delta);
    auto const lookAtTarget = utility::Vector3::Transform(m_target - m_position, rotate);

    m_up = utility::Vector3::Normalize(utility::Vector3::Transform(m_up, rotate));
    m_right = utility::Vector3::Normalize(utility::Vector3::Transform(m_right, rotate));
    m_target = m_position + lookAtTarget;

    UpdateViewProjectionMatrix();
}

void Camera::Pitch(float delta)
{
    auto const rotate = utility::Matrix::CreateRotation(m_right, delta);

    auto newUp = utility::Vector3::Transform(m_up, rotate);

    if (newUp.Z < 0.0f)
        newUp.Z = 0.0f;

    m_up = utility::Vector3::Normalize(newUp);

    auto const lookAtLength = (m_target - m_position).Length();
    m_target = m_position + lookAtLength * utility::Vector3::Normalize(utility::Vector3::CrossProduct(m_up, m_right));

    UpdateViewProjectionMatrix();
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
        Yaw(0.001f * static_cast<float>(deltaX));

    if (abs(deltaY) > 1)
        Pitch(0.001f * static_cast<float>(deltaY));
}

void Camera::GetMousePanStart(int &x, int &y) const
{
    x = m_mousePanX;
    y = m_mousePanY;
}