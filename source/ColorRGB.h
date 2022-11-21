#pragma once
#include "MathHelpers.h"

namespace dae
{
	struct ColorRGB
	{
		float m_pRed{};
		float m_pGreen{};
		float m_pBlue{};

		void MaxToOne()
		{
			const float maxValue = std::max(m_pRed, std::max(m_pGreen, m_pBlue));
			if (maxValue > 1.f)
				*this /= maxValue;
		}

		static ColorRGB Lerp(const ColorRGB& c1, const ColorRGB& c2, float factor)
		{
			return { Lerpf(c1.m_pRed, c2.m_pRed, factor), Lerpf(c1.m_pGreen, c2.m_pGreen, factor), Lerpf(c1.m_pBlue, c2.m_pBlue, factor) };
		}

		#pragma region ColorRGB (Member) Operators
		const ColorRGB& operator+=(const ColorRGB& c)
		{
			m_pRed += c.m_pRed;
			m_pGreen += c.m_pGreen;
			m_pBlue += c.m_pBlue;

			return *this;
		}

		ColorRGB operator+(const ColorRGB& c) const
		{
			return { m_pRed + c.m_pRed, m_pGreen + c.m_pGreen, m_pBlue + c.m_pBlue };
		}

		const ColorRGB& operator-=(const ColorRGB& c)
		{
			m_pRed -= c.m_pRed;
			m_pGreen -= c.m_pGreen;
			m_pBlue -= c.m_pBlue;

			return *this;
		}

		ColorRGB operator-(const ColorRGB& c) const
		{
			return { m_pRed - c.m_pRed, m_pGreen - c.m_pGreen, m_pBlue - c.m_pBlue };
		}

		const ColorRGB& operator*=(const ColorRGB& c)
		{
			m_pRed *= c.m_pRed;
			m_pGreen *= c.m_pGreen;
			m_pBlue *= c.m_pBlue;

			return *this;
		}

		ColorRGB operator*(const ColorRGB& c) const
		{
			return { m_pRed * c.m_pRed, m_pGreen * c.m_pGreen, m_pBlue * c.m_pBlue };
		}

		const ColorRGB& operator/=(const ColorRGB& c)
		{
			m_pRed /= c.m_pRed;
			m_pGreen /= c.m_pGreen;
			m_pBlue /= c.m_pBlue;

			return *this;
		}

		const ColorRGB& operator*=(float s)
		{
			m_pRed *= s;
			m_pGreen *= s;
			m_pBlue *= s;

			return *this;
		}

		ColorRGB operator*(float s) const
		{
			return { m_pRed * s, m_pGreen * s,m_pBlue * s };
		}

		const ColorRGB& operator/=(float s)
		{
			m_pRed /= s;
			m_pGreen /= s;
			m_pBlue /= s;

			return *this;
		}

		ColorRGB operator/(float s) const
		{
			return { m_pRed / s, m_pGreen / s,m_pBlue / s };
		}
		#pragma endregion
	};

	//ColorRGB (Global) Operators
	inline ColorRGB operator*(float s, const ColorRGB& c)
	{
		return c * s;
	}

	namespace colors
	{
		static ColorRGB Red{ 1,0,0 };
		static ColorRGB Blue{ 0,0,1 };
		static ColorRGB Green{ 0,1,0 };
		static ColorRGB Yellow{ 1,1,0 };
		static ColorRGB Cyan{ 0,1,1 };
		static ColorRGB Magenta{ 1,0,1 };
		static ColorRGB White{ 1,1,1 };
		static ColorRGB Black{ 0,0,0 };
		static ColorRGB Gray{ 0.5f,0.5f,0.5f };
	}
}