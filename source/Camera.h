#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		float nearPlane{ 0.1f }, farPlane{ 100.f };

		float aspectRatio;

		bool hasMoved{ true };
		bool hasChangedFov{ true };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f }, float _aspectRatio = 16.f / 9.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			aspectRatio = _aspectRatio;

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			Matrix rotationMatrix{ Matrix::CreateRotation(totalPitch,totalYaw,0.f) };
			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();

			//viewMatrix = inverse cameraToWorld
			viewMatrix = Matrix::CreateLookAtLH(origin, forward, Vector3::UnitY);

			//invViewMatrix = cameraToWorld			
			invViewMatrix = Matrix::Inverse(viewMatrix);

			right = invViewMatrix.GetAxisX();
			up = invViewMatrix.GetAxisY();

			hasMoved = false;
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
			 
			hasChangedFov = false;
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			float movementSpeed{ 5.f };
			float rotationSpeed{ 0.5f };

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				const float factor{ 4.f };

				movementSpeed *= factor;
				rotationSpeed *= factor;
			}

			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
			{
				origin += forward * movementSpeed * deltaTime;
				hasMoved = true;
			}

			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
			{
				origin -= forward * movementSpeed * deltaTime;
				hasMoved = true;
			}

			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
			{
				origin += right * movementSpeed * deltaTime;
				hasMoved = true;
			}

			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
			{
				origin -= right * movementSpeed * deltaTime;
				hasMoved = true;
			}

			if (pKeyboardState[SDL_SCANCODE_E])
			{
				fovAngle -= movementSpeed * deltaTime;
				fovAngle = std::max(fovAngle, 1.f);

				hasChangedFov = true;
			}

			if (pKeyboardState[SDL_SCANCODE_Q])
			{
				fovAngle += movementSpeed * deltaTime;
				fovAngle = std::min(fovAngle, 179.f);

				hasChangedFov = true;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (mouseX != 0 || mouseY != 0)
			{
				const float directionX{ static_cast<float>(mouseX) }, directionY{ static_cast<float>(mouseY) };

				if ((mouseState & SDL_BUTTON_LMASK) && (mouseState & SDL_BUTTON_RMASK))
				{
					origin += right * movementSpeed * directionX * deltaTime;
					origin -= up * movementSpeed * directionY * deltaTime;
					hasMoved = true;
				}
				else if (mouseState & SDL_BUTTON_LMASK)
				{
					totalYaw += rotationSpeed * directionX * deltaTime;
					origin -= forward * movementSpeed * directionY * deltaTime;
					hasMoved = true;
				}
				else if (mouseState & SDL_BUTTON_RMASK)
				{
					totalYaw += rotationSpeed * directionX * deltaTime;
					totalPitch -= rotationSpeed * directionY * deltaTime;
					hasMoved = true;
				}
				else if (mouseState)
				{
					origin -= forward * movementSpeed * directionY * deltaTime;
					hasMoved = true;
				}
			}

			//Update Matrices
			if (hasMoved)
			{
				CalculateViewMatrix();
			}

			if (hasChangedFov)
			{
				fov = tanf((fovAngle * TO_RADIANS) / 2.f);

				CalculateProjectionMatrix();
			}
		}
	};
}
