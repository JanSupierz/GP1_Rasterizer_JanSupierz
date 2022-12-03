#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void ToggleRenderMode();
		void ToggleRotation();
		void ToggleNormalMap();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		//Textures
		Texture* m_pDiffuseTexture;
		Texture* m_pGlossTexture;
		Texture* m_pNormalTexture;
		Texture* m_pSpecularTexture;

		//Shading
		const Vector3 m_LightDirection{ 0.577f,-0.577f,0.577f };
		const float m_LightIntensity{ 7.f };
		const float m_Shininess{ 25.f };
		const ColorRGB m_Ambient{ 0.025f,0.025f,0.025f };

		//Window Size
		int m_Width{};
		int m_Height{};

		std::vector<Mesh> m_MeshesWorld;

		//Rotation
		bool m_ShouldRotate{ true };
		float m_RotationAngle{};

		//Normal Map
		bool m_UseNormalMap{ true };

		enum class RenderMode { ObservedArea, Diffuse, Specular, Combined };
		RenderMode m_CurrentRenderMode{ RenderMode::Combined };

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(std::vector<Mesh>& meshes); //W2 version

		void Render_W3_Part1();

		void PixelShading(const Vertex_Out& v);
	};
}
