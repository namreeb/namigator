#include "LinearAlgebra.hpp"
#include "Camera.hpp"

Camera::Camera()
    : m_projectionMatrix(utility::Matrix::CreateProjectionMatrix(PI / 4.f, 1200.f/800.f, 0.1f, 10000.f)),
      m_position({0.f, 0.f, 0.f}), m_target({0.f, 0.f, -1.f}), m_up({ 0.f, 1.f, 0.f })
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

void Camera::MoveVertical(float delta)
{
    m_position.Z += delta;

    UpdateViewProjectionMatrix();
}

void Camera::Yaw(float delta)
{
    auto const rotate = utility::Matrix::CreateRotationZ(delta) * utility::Matrix::CreateTranslationMatrix(m_position);

    m_target = utility::Vertex::Transform(m_target, rotate);
    m_up = utility::Vector3::Normalize(utility::Vector3::Transform(m_up, rotate));

    UpdateViewProjectionMatrix();
}

void Camera::LookAt(const utility::Vertex &target)
{
    m_target = target;

    UpdateViewProjectionMatrix();
}