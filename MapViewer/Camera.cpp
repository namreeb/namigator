#include "LinearAlgebra.hpp"
#include "Camera.hpp"

Camera::Camera()
    : m_projectionMatrix(utility::Matrix<float>::CreateProjectionMatrix(PI / 4.f, 1200.f/800.f, 0.1f, 10000.f)),
      m_position({0.f, 0.f, 0.f}), m_lookAt({0.f, 0.f, 0.f})
{}

void Camera::UpdateViewProjectionMatrix()
{
    auto view = utility::Matrix<float>::CreateViewMatrix(m_position, m_lookAt, m_up);
    auto viewProjection = view*m_projectionMatrix;

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

void Camera::LookAt(float x, float y, float z)
{
    m_lookAt = { x, y, z };

    UpdateViewProjectionMatrix();
}

void Camera::LookAtLeftRight(float delta)
{
    m_lookAt.X += delta;

    UpdateViewProjectionMatrix();
}

void Camera::LookAtFrontBack(float delta)
{
    m_lookAt.Y += delta;

    UpdateViewProjectionMatrix();
}

void Camera::LookAtUpDown(float delta)
{
    m_lookAt.Z += delta;
}