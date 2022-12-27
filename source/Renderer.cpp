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
	m_Camera.Initialize(45.f, { 0.f,0.f,0.f }, m_Width / static_cast<float>(m_Height));

	m_MeshesWorld = { Mesh{} };

	m_pDiffuseTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pNormalTexture = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pGlossTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png");

	m_MeshesWorld[0].primitiveTopology = PrimitiveTopology::TriangleList;
	Utils::ParseOBJ("Resources/vehicle.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;

	delete m_pSpecularTexture;
	delete m_pNormalTexture;
	delete m_pGlossTexture;
	delete m_pDiffuseTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_ShouldRotate)
	{
		m_RotationAngle += pTimer->GetElapsed();
		m_MeshesWorld[0].worldMatrix = Matrix::CreateRotationY(m_RotationAngle) * Matrix::CreateTranslation(0.f, 0.f, 50.f);
	}
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

		const Matrix wordldViewProjectionMatrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
			
		Vector4 position{};

		for (size_t index{}; index < mesh.vertices.size(); ++index)
		{
			position = { mesh.vertices[index].position.x,mesh.vertices[index].position.y,mesh.vertices[index].position.z,0.f };
			mesh.vertices_out[index].position = wordldViewProjectionMatrix.TransformPoint(position);

			const float inverseW{ 1.f / mesh.vertices_out[index].position.w };

			//Positions
			mesh.vertices_out[index].position.x *= inverseW;
			mesh.vertices_out[index].position.y *= inverseW;
			mesh.vertices_out[index].position.z *= inverseW;
			mesh.vertices_out[index].position.w = inverseW;

			//Normals
			mesh.vertices_out[index].normal = mesh.worldMatrix.TransformVector(mesh.vertices[index].normal).Normalized();
			mesh.vertices_out[index].tangent = mesh.worldMatrix.TransformVector(mesh.vertices[index].tangent).Normalized();

			//View
			mesh.vertices_out[index].viewDirection = mesh.worldMatrix.TransformPoint(mesh.vertices[index].position) - m_Camera.origin;
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::ToggleRenderMode()
{
	if (m_CurrentRenderMode < RenderMode::Combined)
	{
		m_CurrentRenderMode = static_cast<RenderMode>(static_cast<int>(m_CurrentRenderMode) + 1);
	}
	else
	{
		m_CurrentRenderMode = RenderMode::ObservedArea;
	}
}

void dae::Renderer::ToggleRotation()
{
	m_ShouldRotate = !m_ShouldRotate;
}

void dae::Renderer::ToggleNormalMap()
{
	m_UseNormalMap = !m_UseNormalMap;
}

void Renderer::Render_W3_Part1()
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

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
					/*m_pBackBufferPixels[static_cast<int>(px) + (static_cast<int>(py) * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(255),
						static_cast<uint8_t>(255),
						static_cast<uint8_t>(255));			// boundingbox visualization
					continue;*/

					Vector3 vertexRatio{};

					//Rasterization
					if (!Utils::IsPixelInTriangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio, !isTriangleList && index & 0x01)) continue;


					//Attribute Interpolation
					const float currentDepth{ 1.f / ((vertexRatio.x / mesh.vertices_out[mesh.indices[index]].position.z) + (vertexRatio.y / mesh.vertices_out[mesh.indices[index + 1]].position.z) + (vertexRatio.z / mesh.vertices_out[mesh.indices[index + 2]].position.z)) };

					if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
					{
						m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

						const float wInterpolated{ 1.f / ((vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w) + (vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w) + (vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w)) };

						Vertex_Out pixel
						{
							Vector4//position
							{
								static_cast<float>(px),
								static_cast<float>(py),
								currentDepth,
								wInterpolated
							},
							ColorRGB //color
							{
								(mesh.vertices[mesh.indices[index]].color * vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w + mesh.vertices[mesh.indices[index + 1]].color * vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w + mesh.vertices[mesh.indices[index + 2]].color * vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].color) * wInterpolated
							},
							Vector2 //uv
							{
								(mesh.vertices[mesh.indices[index]].uv * vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w + mesh.vertices[mesh.indices[index + 1]].uv * vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w + mesh.vertices[mesh.indices[index + 2]].uv * vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w) * wInterpolated
							},
							Vector3 //normal
							{
								((mesh.vertices_out[mesh.indices[index]].normal * vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w + mesh.vertices_out[mesh.indices[index + 1]].normal * vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w + mesh.vertices_out[mesh.indices[index + 2]].normal * vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w) * wInterpolated).Normalized()
							},
							Vector3 //tangent
							{
								((mesh.vertices_out[mesh.indices[index]].tangent * vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w + mesh.vertices_out[mesh.indices[index + 1]].tangent * vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w + mesh.vertices_out[mesh.indices[index + 2]].tangent * vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w) * wInterpolated).Normalized()
							},
							Vector3 //viewDirection
							{
								((mesh.vertices_out[mesh.indices[index]].viewDirection * vertexRatio.x * mesh.vertices_out[mesh.indices[index]].position.w + mesh.vertices_out[mesh.indices[index + 1]].viewDirection * vertexRatio.y * mesh.vertices_out[mesh.indices[index + 1]].position.w + mesh.vertices_out[mesh.indices[index + 2]].viewDirection * vertexRatio.z * mesh.vertices_out[mesh.indices[index + 2]].position.w) * wInterpolated).Normalized()
							}
						};

						PixelShading(pixel);
					}
				}
			}
		}
	}
}

void Renderer::PixelShading(const Vertex_Out& v)
{
	ColorRGB finalColor{};

	const Vector3 binominal{ Vector3::Cross(v.normal,v.tangent) };
	const Matrix tangentSpaceAxis{ v.tangent,binominal,v.normal,Vector3::Zero };

	const ColorRGB normalMapSample{ m_pNormalTexture->Sample(v.uv) };

	Vector3 sampledNormal{ 2.f * normalMapSample.m_pRed - 1.f,2.f * normalMapSample.m_pGreen - 1.f,2.f * normalMapSample.m_pBlue - 1.f };
	sampledNormal = (m_UseNormalMap ? tangentSpaceAxis.TransformVector(sampledNormal).Normalized() : v.normal);

	const float observedArea{ Vector3::Dot(sampledNormal,-m_LightDirection) };

	if (observedArea > 0.f)
	{
		const ColorRGB diffuse{ (m_LightIntensity * m_pDiffuseTexture->Sample(v.uv)) / PI };

		ColorRGB specular{ (m_pSpecularTexture->Sample(v.uv)) * powf(std::max(Vector3::Dot(-m_LightDirection - (2.f * std::max(Vector3::Dot(sampledNormal, -m_LightDirection), 0.f) * sampledNormal), v.viewDirection), 0.f), m_Shininess * m_pGlossTexture->Sample(v.uv).m_pRed) };

		specular.m_pRed = std::max(0.f, specular.m_pRed);
		specular.m_pGreen = std::max(0.f, specular.m_pGreen);
		specular.m_pBlue = std::max(0.f, specular.m_pBlue);

		switch (m_CurrentRenderMode)
		{
		case dae::Renderer::RenderMode::Combined:
			finalColor = observedArea * (diffuse + specular + m_Ambient);
			break;
		case dae::Renderer::RenderMode::ObservedArea:
			finalColor = { observedArea,observedArea,observedArea };
			break;
		case dae::Renderer::RenderMode::Diffuse:
			finalColor = observedArea * diffuse;
			break;
		case dae::Renderer::RenderMode::Specular:
			finalColor = observedArea * specular;
			break;
		}
	}

	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBackBufferPixels[static_cast<int>(v.position.x) + (static_cast<int>(v.position.y) * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(finalColor.m_pRed * 255),
		static_cast<uint8_t>(finalColor.m_pGreen * 255),
		static_cast<uint8_t>(finalColor.m_pBlue * 255));
}
