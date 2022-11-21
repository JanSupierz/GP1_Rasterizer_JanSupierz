#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
		m_pRed = new Uint8;
		m_pGreen = new Uint8;
		m_pBlue = new Uint8;
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}

		delete m_pRed;
		delete m_pGreen;
		delete m_pBlue;
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		return new Texture(IMG_Load(path.c_str()));
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Sample the correct texel for the given uv

		SDL_GetRGB(m_pSurfacePixels[static_cast<Uint32>(int(uv.x * m_pSurface->w) + int(uv.y * m_pSurface->h) * m_pSurface->w)], m_pSurface->format, m_pRed, m_pGreen, m_pBlue);

		return { *m_pRed / 255.f, *m_pGreen / 255.f, *m_pBlue / 255.f };
	}
}