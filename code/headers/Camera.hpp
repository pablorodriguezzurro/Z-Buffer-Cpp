#pragma once

#include "declarations/Math.hpp"

#include "Scene_Object.hpp"

namespace przurro
{
	class Camera : public Scene_Object
	{
	private:

		Projection_Matrix3f projectionMatrix;
		 
		Vector3f * viewTarget;
		Vector3f defaultViewDirection; 

		size_t width;
		size_t height;

	public:

		Camera(float nearPlaneD, float farPlaneD, float fovDegrees, size_t inputWidth, size_t inputHeight, Vector3f * target = nullptr);

	public:

		Matrix44f look_at(const Vector3f& at);

		Matrix44f look_at()
		{
			if (!viewTarget)
			{
				return look_at(defaultViewDirection);
			}

			return look_at(*viewTarget);
		}

	public:

		void set_target(Vector3f * target)
		{
			viewTarget = target;
		}

	public:

		const Projection_Matrix3f & get_projection_matrix() const
		{
			return projectionMatrix;
		}

		size_t get_width()
		{
			return width;
		}
		size_t get_height()
		{
			return height;
		}
	};
}