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
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Render_W1_Part1();
	//Render_W1_Part2();
	//Render_W1_Part3();
	//Render_W1_Part4();
	//Render_W1_Part5();

	//Render_W2_Part1();
	//Render_W2_Part2();
	Render_W2_Part3();
	
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	for (Mesh& mesh : meshes)
	{
		mesh.vertices_out.resize(mesh.vertices.size());

		const float aspectRatio{ m_Width / static_cast<float>(m_Height) };

		for (size_t index{}; index < mesh.vertices.size(); ++index)
		{
			//View Space
			mesh.vertices_out[index].position = m_Camera.viewMatrix.TransformPoint(mesh.vertices[index].position);

			//Perspective divide -> Projection Space
			mesh.vertices_out[index].position.x = mesh.vertices_out[index].position.x / mesh.vertices_out[index].position.z;
			mesh.vertices_out[index].position.y = mesh.vertices_out[index].position.y / mesh.vertices_out[index].position.z;

			//Projection Stage --- Aspect Ratio
			mesh.vertices_out[index].position.x = mesh.vertices_out[index].position.x / (aspectRatio * m_Camera.fov);
			mesh.vertices_out[index].position.y = mesh.vertices_out[index].position.y / m_Camera.fov;

			//Screen Space
			mesh.vertices_out[index].position.x = 0.5f * (mesh.vertices_out[index].position.x + 1.f) * m_Width;
			mesh.vertices_out[index].position.y = 0.5f * (1.f - mesh.vertices_out[index].position.y) * m_Height;
		}
	}
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	const float aspectRatio{ m_Width / static_cast<float>(m_Height) };

	for (size_t index{}; index < vertices_in.size(); ++index)
	{
		vertices_out[index].color = vertices_in[index].color;

		//View Space
		vertices_out[index].position = m_Camera.viewMatrix.TransformPoint(vertices_in[index].position);

		//Perspective divide -> Projection Space
		vertices_out[index].position.x = vertices_out[index].position.x / vertices_out[index].position.z;
		vertices_out[index].position.y = vertices_out[index].position.y / vertices_out[index].position.z;
		
		//Projection Stage --- Aspect Ratio
		vertices_out[index].position.x = vertices_out[index].position.x / (aspectRatio * m_Camera.fov);
		vertices_out[index].position.y = vertices_out[index].position.y / m_Camera.fov;

		//Screen Space
		vertices_out[index].position.x = 0.5f * (vertices_out[index].position.x + 1.f) * m_Width;
		vertices_out[index].position.y = 0.5f * (1.f - vertices_out[index].position.y) * m_Height;

	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::Render_W2_Part3() const
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3.f, 3.f, -2.f},colors::White, {0,0}},
				Vertex{{0.f, 3.f, -2.f},colors::White,  {0.5,0}},
				Vertex{{3.f, 3.f, -2.f},colors::White,  {1,0}},
				Vertex{{-3.f, 0.f, -2.f},colors::White, {0,0.5}},
				Vertex{{0.f, 0.f, -2.f},colors::White,  {0.5,0.5}},
				Vertex{{3.f, 0.f, -2.f},colors::White,  {1,0.5}},
				Vertex{{-3.f, -3.f, -2.f},colors::White,{0,1}},
				Vertex{{0.f, -3.f, -2.f},colors::White, {0.5,1}},
				Vertex{{3.f, -3.f, -2.f},colors::White, {1,1}}
			},

		//1 of the 2 below indices has to be uncommitted together with the PrimitiveTropology::...

		//indices for triangleList
			//{
			//	3, 0, 1,    1, 4, 3,    4, 1, 2,
			//	2, 5, 4,    6, 3, 4,    4, 7, 6,
			//	7, 4, 5,    5, 8, 7
			//},

			//PrimitiveTopology::TriangleList

		//indices for triangleStrip
			{
				3, 0, 4, 1, 5, 2,
				2, 6,
				6, 3, 7, 4, 8, 5
			},

			PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_world);

	for (Mesh& mesh : meshes_world)
	{
		const int increment{ mesh.primitiveTopology == PrimitiveTopology::TriangleList ? 3 : 1 };
		const int maxCount{ mesh.primitiveTopology == PrimitiveTopology::TriangleList ? static_cast<int>(mesh.indices.size()) : static_cast<int>(mesh.indices.size()) - 2 };

		for (size_t index{}; index < maxCount; index += increment)
		{
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
			for (int px{}; px < m_Width; ++px)
			{
				if (px >= min.x && px <= max.x)
				{
					for (int py{}; py < m_Height; ++py)
					{
						if (py >= min.y && py <= max.y)
						{
							Vector3 vertexRatio{};

							if (increment == 1 && index & 0x01)
							{
								if (!Utils::HitTest_TriangleOdd(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio)) continue;
							}
							else
							{
								if (!Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio)) continue;
							}

							const float currentDepth{ 1.f / ((vertexRatio.x / mesh.vertices_out[mesh.indices[index]].position.z) + (vertexRatio.y / mesh.vertices_out[mesh.indices[index + 1]].position.z) + (vertexRatio.z / mesh.vertices_out[mesh.indices[index + 2]].position.z)) };

							if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
							{
								m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

								ColorRGB finalColor{ m_pTexture->Sample((mesh.vertices[mesh.indices[index]].uv * vertexRatio.x / mesh.vertices_out[mesh.indices[index]].position.z + mesh.vertices[mesh.indices[index + 1]].uv * vertexRatio.y / mesh.vertices_out[mesh.indices[index + 1]].position.z + mesh.vertices[mesh.indices[index + 2]].uv * vertexRatio.z / mesh.vertices_out[mesh.indices[index + 2]].position.z) * currentDepth) };

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
	}
}

void Renderer::Render_W2_Part2() const
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3.f, 3.f, -2.f},colors::White, {0,0}},
				Vertex{{0.f, 3.f, -2.f},colors::White,  {0.5,0}},
				Vertex{{3.f, 3.f, -2.f},colors::White,  {1,0}},
				Vertex{{-3.f, 0.f, -2.f},colors::White, {0,0.5}},
				Vertex{{0.f, 0.f, -2.f},colors::White,  {0.5,0.5}},
				Vertex{{3.f, 0.f, -2.f},colors::White,  {1,0.5}},
				Vertex{{-3.f, -3.f, -2.f},colors::White,{0,1}},
				Vertex{{0.f, -3.f, -2.f},colors::White, {0.5,1}},
				Vertex{{3.f, -3.f, -2.f},colors::White, {1,1}}
			},

		//1 of the 2 below indices has to be uncommitted together with the PrimitiveTropology::...

		//indices for triangleList
			//{
			//	3, 0, 1,    1, 4, 3,    4, 1, 2,
			//	2, 5, 4,    6, 3, 4,    4, 7, 6,
			//	7, 4, 5,    5, 8, 7
			//},

			//PrimitiveTopology::TriangleList

		//indices for triangleStrip
			{
				3, 0, 4, 1, 5, 2,
				2, 6,
				6, 3, 7, 4, 8, 5
			},

			PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_world);

	for (Mesh& mesh : meshes_world)
	{
		const int increment{ mesh.primitiveTopology == PrimitiveTopology::TriangleList ? 3 : 1 };
		const int maxCount{ mesh.primitiveTopology == PrimitiveTopology::TriangleList ? static_cast<int>(mesh.indices.size()) : static_cast<int>(mesh.indices.size()) - 2 };

		for (size_t index{}; index < maxCount; index += increment)
		{
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
			for (int px{}; px < m_Width; ++px)
			{
				if (px >= min.x && px <= max.x)
				{
					for (int py{}; py < m_Height; ++py)
					{
						if (py >= min.y && py <= max.y)
						{
							Vector3 vertexRatio{};

							if (increment == 1 && index & 0x01)
							{
								if (!Utils::HitTest_TriangleOdd(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio)) continue;
							}
							else
							{
								if (!Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio)) continue;
							}

							const float currentDepth{ mesh.vertices_out[mesh.indices[index]].position.z * vertexRatio.x + mesh.vertices_out[mesh.indices[index + 1]].position.z * vertexRatio.y + mesh.vertices_out[mesh.indices[index + 2]].position.z * vertexRatio.z };

							if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
							{
								m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

								ColorRGB finalColor{ m_pTexture->Sample(mesh.vertices[mesh.indices[index]].uv * vertexRatio.x + mesh.vertices[mesh.indices[index + 1]].uv * vertexRatio.y + mesh.vertices[mesh.indices[index + 2]].uv * vertexRatio.z) };

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
	}
}

void Renderer::Render_W2_Part1() const
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3.f, 3.f, -2.f}},
				Vertex{{0.f, 3.f, -2.f}},
				Vertex{{3.f, 3.f, -2.f}},
				Vertex{{-3.f, 0.f, -2.f}},
				Vertex{{0.f, 0.f, -2.f}},
				Vertex{{3.f, 0.f, -2.f}},
				Vertex{{-3.f, -3.f, -2.f}},
				Vertex{{0.f, -3.f, -2.f}},
				Vertex{{3.f, -3.f, -2.f}}
			},

		//1 of the 2 below indices has to be uncommitted together with the PrimitiveTropology::...

		//indices for triangleList
			//{
			//	3, 0, 1,    1, 4, 3,    4, 1, 2,
			//	2, 5, 4,    6, 3, 4,    4, 7, 6,
			//	7, 4, 5,    5, 8, 7
			//},

			//PrimitiveTopology::TriangleList

		//indices for triangleStrip
			{
				3, 0, 4, 1, 5, 2,
				2, 6,
				6, 3, 7, 4, 8, 5
			},

			PrimitiveTopology::TriangleStrip
		}
	};

	VertexTransformationFunction(meshes_world);

	for (Mesh& mesh : meshes_world)
	{
		const int increment{ mesh.primitiveTopology == PrimitiveTopology::TriangleList ? 3 : 1 };
		const int maxCount{ mesh.primitiveTopology == PrimitiveTopology::TriangleList ? static_cast<int>(mesh.indices.size()) : static_cast<int>(mesh.indices.size()) - 2 };

		for (size_t index{}; index < maxCount; index += increment)
		{
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
			for (int px{}; px < m_Width; ++px)
			{
				if (px >= min.x && px <= max.x)
				{
					for (int py{}; py < m_Height; ++py)
					{
						if (py >= min.y && py <= max.y)
						{
							Vector3 vertexRatio{};

							if (index & 0x01 && increment == 1)
							{
								if (!Utils::HitTest_TriangleOdd(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio)) continue;
							}
							else
							{
								if (!Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio)) continue;
							}

							float currentDepth = mesh.vertices_out[mesh.indices[index]].position.z * vertexRatio.x + mesh.vertices_out[mesh.indices[index + 1]].position.z * vertexRatio.y + mesh.vertices_out[mesh.indices[index + 2]].position.z * vertexRatio.z;

							if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
							{

								m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

								ColorRGB finalColor{ mesh.vertices[mesh.indices[index]].color * vertexRatio.x + mesh.vertices[mesh.indices[index + 1]].color * vertexRatio.y + mesh.vertices[mesh.indices[index + 2]].color * vertexRatio.z };

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
	}
}

void Renderer::Render_W1_Part5() const
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{ {0.f,2.f,0.f},    {1,0,0} },
		{ {1.5f,-1.f,0.f},  {1,0,0} },
		{ {-1.5f,-1.f,0.f}, {1,0,0} },

		//Triangle 1
		{ {0.f,4.f,2.f},   {1,0,0} },
		{ {3.f,-2.f,2.f},  {0,1,0} },
		{ {-3.f,-2.f,2.f}, {0,0,1} }
	};

	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	for (size_t index{}; index < vertices_screen.size(); index += 3)
	{
		const Vector2 v0{ vertices_screen[index].position.GetXY() };
		const Vector2 v1{ vertices_screen[index + 1].position.GetXY() };
		const Vector2 v2{ vertices_screen[index + 2].position.GetXY() };

		Vector2 min{ std::min(v0.x, v1.x),std::min(v0.y, v1.y) };
		min.x = std::min(min.x, v2.x);
		min.y = std::min(min.y, v2.y);

		Vector2 max{ std::max(v0.x, v1.x),std::max(v0.y, v1.y) };
		max.x = std::max(max.x, v2.x);
		max.y = std::max(max.y, v2.y);


		//RENDER LOGIC
		for (int px{}; px < m_Width; ++px)
		{
			if (px > min.x && px < max.x)
			{
				for (int py{}; py < m_Height; ++py)
				{
					if (py > min.y && py < max.y)
					{
						Vector3 vertexRatio{};

						if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio))
						{

							float currentDepth = vertices_screen[index].position.z * vertexRatio.x + vertices_screen[index + 1].position.z * vertexRatio.y + vertices_screen[index + 2].position.z * vertexRatio.z;

							if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
							{

								m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

								ColorRGB finalColor{ vertices_screen[index].color * vertexRatio.x + vertices_screen[index + 1].color * vertexRatio.y + vertices_screen[index + 2].color * vertexRatio.z };

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
	}
}

void Renderer::Render_W1_Part4() const
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{ {0.f,2.f,0.f},    {1,0,0} },
		{ {1.5f,-1.f,0.f},  {1,0,0} },
		{ {-1.5f,-1.f,0.f}, {1,0,0} },

		//Triangle 1
		{ {0.f,4.f,2.f},   {1,0,0} },
		{ {3.f,-2.f,2.f},  {0,1,0} },
		{ {-3.f,-2.f,2.f}, {0,0,1} }
	};

	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	for (size_t index{}; index < vertices_screen.size(); index += 3)
	{
		const Vector2 v0{ vertices_screen[index].position.GetXY() };
		const Vector2 v1{ vertices_screen[index + 1].position.GetXY() };
		const Vector2 v2{ vertices_screen[index + 2].position.GetXY() };

		//RENDER LOGIC
		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				Vector3 vertexRatio{};

				if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, vertexRatio))
				{

					float currentDepth = vertices_screen[index].position.z * vertexRatio.x + vertices_screen[index + 1].position.z * vertexRatio.y + vertices_screen[index + 2].position.z * vertexRatio.z;

					if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
					{

						m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

						ColorRGB finalColor{ vertices_screen[index].color * vertexRatio.x + vertices_screen[index + 1].color * vertexRatio.y + vertices_screen[index + 2].color * vertexRatio.z };

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

void Renderer::Render_W1_Part3() const
{
	std::vector<Vertex> vertices_world{ { {0.f,4.f,2.f},{1,0,0} },{ {3.f,-2.f,2.f}, {0,1,0} },{ {-3.f,-2.f,2.f}, {0,0,1} } };
	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	const Vector2 v0{ vertices_screen[0].position.GetXY() };
	const Vector2 v1{ vertices_screen[1].position.GetXY() };
	const Vector2 v2{ vertices_screen[2].position.GetXY() };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{


			Vector3 colorRatio{};
			ColorRGB finalColor{};

			if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, colorRatio))
			{
				finalColor = vertices_screen[0].color * colorRatio.x + vertices_screen[1].color * colorRatio.y + vertices_screen[2].color * colorRatio.z;
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

void Renderer::Render_W1_Part2() const
{
	std::vector<Vertex> vertices_world{ {{0.f,2.f,0.f}},{{1.f,0.f,0.f}},{{-1.f,0.f,0.f}} };
	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	const Vector2 v0{ vertices_screen[0].position.GetXY() };
	const Vector2 v1{ vertices_screen[1].position.GetXY() };
	const Vector2 v2{ vertices_screen[2].position.GetXY() };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{


			Vector3 colorRatio{};
			ColorRGB finalColor{};

			if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2, colorRatio))
			{
				 finalColor = vertices_screen[0].color * colorRatio.x + vertices_screen[1].color * colorRatio.y + vertices_screen[2].color * colorRatio.z;
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

void Renderer::Render_W1_Part1() const
{
	std::vector<Vector3> vertices_ndc{ {0.f,0.5f,1.f},{0.5f,-0.5f,1.f},{-0.5f,-0.5f,1.f} };

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			Vector2 v0{ 0.5f * (vertices_ndc[0].x + 1.f) * m_Width, 0.5f * (1.f - vertices_ndc[0].y) * m_Height };
			Vector2 v1{ 0.5f * (vertices_ndc[1].x + 1.f) * m_Width, 0.5f * (1.f - vertices_ndc[1].y) * m_Height };
			Vector2 v2{ 0.5f * (vertices_ndc[2].x + 1.f) * m_Width, 0.5f * (1.f - vertices_ndc[2].y) * m_Height };

			Vector3 colorRatio{};
			float gradient = 0.f;

			if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px),static_cast<float>(py) }, v0, v1, v2,colorRatio))
			{
				gradient = 1.f;
			}

			ColorRGB finalColor{ gradient, gradient, gradient };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.m_pRed * 255),
				static_cast<uint8_t>(finalColor.m_pGreen * 255),
				static_cast<uint8_t>(finalColor.m_pBlue * 255));

		}
	}
}
