#include "LinearAlgebra.hpp"
#include "Camera.hpp"

Camera::Camera()
    : m_projectionMatrix(utility::Matrix::CreateProjectionMatrix(PI / 4.f, 1200.f/800.f, 0.1f, 10000.f)),
      m_position({0.f, 0.f, 0.f}), m_lookNormal({0.f, 0.f, -1.f})
{}

void Camera::UpdateViewProjectionMatrix()
{
    auto view = utility::Matrix::CreateViewMatrixFromLookNormal(m_position, m_lookNormal, m_up);
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

void Camera::RotateAroundZ(float delta)
{
    auto r = utility::Matrix::CreateViewMatrixFromLookNormal(m_position, m_lookNormal, m_up) * utility::Matrix::CreateRotationX(delta);

}