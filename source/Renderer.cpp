//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,5.f,-30.f }, m_Width / static_cast<float>(m_Height));

	//m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");

	//m_MeshesWorld = 
	//{
	//	Mesh
	//	{
	//		{
	//			Vertex{{-3.f, 3.f, -2.f},colors::White, {0,0}},
	//			Vertex{{0.f, 3.f, -2.f},colors::White,  {0.5,0}},
	//			Vertex{{3.f, 3.f, -2.f},colors::White,  {1,0}},
	//			Vertex{{-3.f, 0.f, -2.f},colors::White, {0,0.5}},
	//			Vertex{{0.f, 0.f, -2.f},colors::White,  {0.5,0.5}},
	//			Vertex{{3.f, 0.f, -2.f},colors::White,  {1,0.5}},
	//			Vertex{{-3.f, -3.f, -2.f},colors::White,{0,1}},
	//			Vertex{{0.f, -3.f, -2.f},colors::White, {0.5,1}},
	//			Vertex{{3.f, -3.f, -2.f},colors::White, {1,1}}
	//		},

	//	//indices for triangleList
	//		{
	//			3, 0, 1,    1, 4, 3,    4, 1, 2,
	//			2, 5, 4,    6, 3, 4,    4, 7, 6,
	//			7, 4, 5,    5, 8, 7
	//		},

	//		PrimitiveTopology::TriangleList

	//	//indices for triangleStrip
	//		//{
	//		//	3, 0, 4, 1, 5, 2,
	//		//	2, 6,
	//		//	6, 3, 7, 4, 8, 5
	//		//},

	//		//PrimitiveTopology::TriangleStrip
	//	}
	//};

	m_MeshesWorld = { Mesh{} };

	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");

	m_MeshesWorld[0].primitiveTopology = PrimitiveTopology::TriangleList;
	Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	m_MeshesWorld[0].worldMatrix = Matrix::CreateRotationY(pTimer->GetTotal());
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	Render_W3_Part1();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes)
{
	for (Mesh& mesh : meshes)
	{
		mesh.vertices_out.resize(mesh.vertices.size());

		Matrix wordldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;
			
		Vector4 position{};

		for (size_t index{}; index < mesh.vertices.size(); ++index)
		{
			position = { mesh.vertices[index].position.x,mesh.vertices[index].position.y,mesh.vertices[index].position.z,0.f };
			mesh.vertices_out[index].position = wordldViewProjectionMatrix.TransformPoint(position);

			const float inverseW{ 1.f / mesh.vertices_out[index].position.w };

			mesh.vertices_out[index].position.x *= inverseW;
			mesh.vertices_out[index].position.y *= inverseW;
			mesh.vertices_out[index].position.z *= inverseW;
			mesh.vertices_out[index].position.w = inverseW;
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::ToggleRenderMode()
{
	if (m_CurrentRenderMode < RenderMode::depth)
	{
		m_CurrentRenderMode = static_cast<RenderMode>(static_cast<int>(m_CurrentRenderMode) + 1);
	}
	else
	{
		m_CurrentRenderMode = RenderMode::texture;
	}
}

void Renderer::Render_W3_Part1()
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));

	VertexTransformationFunction(m_MeshesWorld);

	for (Mesh& mesh : m_MeshesWorld)
	{
		for (size_t index{}; index < mesh.vertices.size(); ++index)
		{
			//NDC space -> Raster space
			mesh.vertices_out[index].position.x = 0.5f * (mesh.vertices_out[index].position.x + 1.f) * m_Width;
			mesh.vertices_out[index].position.y = 0.5f * (1.f - mesh.vertices_out[index].position.y) * m_Height;
		}

		const bool isTriangleList{ mesh.primitiveTopology == PrimitiveTopology::TriangleList };

		const int increment{ isTriangleList * 3 + !isTriangleList * 1 };
		const int maxCount{ static_cast<int>(mesh.indices.size()) + !isTriangleList * (-2) };  //Max = nrIndices + 0 bij triangleStrip of -2 bij triangleList

		for (size_t index{}; index < maxCount; index += increment)
		{
			//Frustrum culling
			if (mesh.vertices_out[mesh.indices[index]].position.z < 0.f || mesh.vertices_out[mesh.indices[index]].position.z > 1.f ||
				mesh.vertices_out[mesh.indices[index + 1]].position.z < 0.f || mesh.vertices_out[mesh.indices[index + 1]].position.z > 1.f ||
				mesh.vertices_out[mesh.indices[index + 2]].position.z < 0.f || mesh.vertices_out[mesh.indices[index + 2]].position.z > 1.f) continue;

			if (mesh.indices[index] == mesh.indices[index + 1] || mesh.indices[index] == mesh.indices[index + 2] || mesh.indices[index + 1] == mesh.indices[index + 2]) continue;

			const Vector2 v0{ mesh.vertices_out[mesh.indices[index]].position.GetXY() };
			const Vector2 v1{ mesh.vertices_out[mesh.indices[index + 1]].position.GetXY() };
			const Vector2 v2{ mesh.vertices_out[mesh.indices[index + 2]].position.GetXY() };

			Vector2 min{ std::min(v0.x, v1.x),std::min(v0.y, v1.y) };
			min.x = std::min(min.x, v2.x);
			min.y = std::min(min.y, v2.y);

			Vector2 max{ std::max(v0.x, v1.x),std::max(v0.y, v1.y) };
			max.x = std::max(max.x, v2.x);
			max.y = std::max(max.y, v2.y);


			//RENDER LOGIC
			for (int px{ std::max(0,static_cast<int>(min.x)) }; px <= std::min(m_Width - 1, static_cast<int>(max.x)); ++px)
			{
				for (int py{ std::max(0,static_cast<int>(min.y)) }; py <= std::min(m_Height - 1, static_cast<int>(max.y)); ++py)
				{
					Vector3 vertexRatio{};

					//Rasterization
					if (!Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio, !isTriangleList && index & 0x01)) continue;

					//Attribute Interpolation
					const float currentDepth{ 1.f / ((vertexRatio.x / mesh.vertices_out[mesh.indices[index]].position.z) + (vertexRatio.y / mesh.vertices_out[mesh.indices[index + 1]].position.z) + (vertexRatio.z / mesh.vertices_out[mesh.indices[index + 2]].position.z)) };

					if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
					{
						m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

						const float wInterpolated{ 1.f / ((vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w) + (vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w) + (vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w)) };

						ColorRGB finalColor{};

						switch (m_CurrentRenderMode)
						{
						case dae::Renderer::RenderMode::texture:
							finalColor = m_pTexture->Sample((mesh.vertices[mesh.indices[index]].uv * vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w + mesh.vertices[mesh.indices[index + 1]].uv * vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w + mesh.vertices[mesh.indices[index + 2]].uv * vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w) * wInterpolated);
							break;
						case dae::Renderer::RenderMode::depth:
							const float remappedDepth{ Remap(currentDepth,0.995f) };
							finalColor = ColorRGB{ remappedDepth,remappedDepth,remappedDepth };
							break;
						}

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.m_pRed * 255),
							static_cast<uint8_t>(finalColor.m_pGreen * 255),
							static_cast<uint8_t>(finalColor.m_pBlue * 255));
					}
				}
			}
		}
	}
}
