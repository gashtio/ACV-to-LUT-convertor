#define NOMINMAX

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <functional>
#include "D3DXVolumeTextureSaver.h"

#define LITTLEENDIANIZE(shortnum) \
	shortnum = (short)(((shortnum & 0xFF) << 8) | (shortnum >> 8))

#define READ_BIG_ENDIAN_USHORT(stream, ush) \
	stream.read(reinterpret_cast<char*>(&ush), sizeof(ush)); \
	if (!stream.good()) \
	{ \
		std::cerr << "Unable to read data from file! (" << __LINE__ << ")" << std::endl; \
		return false; \
	} \
	LITTLEENDIANIZE(ush); \

//
// http://www.adobe.com/devnet-apps/photoshop/fileformatashtml/PhotoshopFileFormats.htm#50577411_pgfId-1056330
//  Curves file format
// 
// 	Length		Description
// 
// 	2			Version ( = 1 or = 4)
// 	2			Count of curves in the file.
// 
// 	The following is the data for each curve specified by count above
// 
// 	2			Count of points in the curve (short integer from 2...19)
// 
// 	point		Curve points.Each curve point is a pair of short integers where the
//  cnt * 4		first number is the output value (vertical coordinate on the Curves
// 				dialog graph) and the second is the input value. All coordinates have
// 				range 0 to 255. See also "Null curves" below.
//

template <typename T>
static T clamp(const T& value, const T& min, const T& max)
{
	if (value > max)
	{
		return max;
	}
	else if (value < min)
	{
		return min;
	}
	else
	{
		return value;
	}
}

template <typename T>
static T saturate(const T& value)
{
	return clamp(value, T(0), T(1));
}

typedef std::vector<std::pair<float, float>> CurvePoints;

class CubicSpline
{
public:
	// Interpolation of natural splines
	static CubicSpline InterpolateCubicSplineFromCurvePoints(const CurvePoints& curve)
	{
		std::vector<float> y2(curve.size()); // second derivatives
		std::vector<float> u(curve.size());

		y2[0] = 0;

		for (size_t i = 1; i < curve.size() - 1; ++i)
		{
			float sig = (curve[i].first - curve[i - 1].first) / (curve[i + 1].first - curve[i - 1].first);
			float p = sig * y2[i - 1] + 2.0f;
			y2[i] = (sig - 1.0f) / p;
			u[i] = (curve[i+1].second - curve[i].second)/(curve[i+1].first - curve[i].first) - (curve[i].second - curve[i-1].second)/(curve[i].first - curve[i-1].first);
			u[i] = (6.0f * u[i] / (curve[i+1].first - curve[i-1].first) - sig*u[i-1]) / p;
		}

		y2[curve.size() - 1] = 0;

		for (int i = (int)curve.size() - 2; i >= 0; --i)
		{
			y2[i] = y2[i] * y2[i + 1] + u[i];
		}

		CubicSpline spline(curve, std::move(y2));
		return spline;
	}

	float ComputeAtPoint(float x)
	{
		int lo = 0;
		int hi = (int)(m_Curve.size() - 1);
		while (hi - lo > 1)
		{
			int k = (hi + lo) / 2;
			if (m_Curve[k].first > x)
			{
				hi = k;
			}
			else
			{
				lo = k;
			}
		}

		float xhi = m_Curve[hi].first;
		float xlo = m_Curve[lo].first;
		float yhi = m_Curve[hi].second;
		float ylo = m_Curve[lo].second;
		float y2hi = m_Y2[hi];
		float y2lo = m_Y2[lo];

		float h = xhi - xlo;
		if (h == 0) throw "Not a curve!";

		float a = (xhi - x) / h;
		float b = (x - xlo) / h;
		float y = a*ylo + b*yhi + ((a*a*a - a)*y2lo + (b*b*b - b)*y2hi)*h*h/6.0f;
		return y;
	}

private:
	CubicSpline(const CurvePoints& curve, std::vector<float>&& y2)
		: m_Curve(curve)
		, m_Y2(std::forward<std::vector<float>>(y2))
	{}

private:
	CurvePoints m_Curve;
	std::vector<float> m_Y2;
};

static bool ReadACVFile(const wchar_t* filename, std::vector<CubicSpline>& outCubicSplines)
{
	std::ifstream ifs(filename, std::ios_base::binary);
	if (!ifs.good())
	{
		std::wcerr << L"Unable to open file: " << filename << std::endl;
		return false;
	}

	unsigned short version;
	READ_BIG_ENDIAN_USHORT(ifs, version);

	if (version != 0x4)
	{
		std::cerr << "Unsupported ACV version - only version 4 is supported (cubic spline interpolation)." << std::endl;
		return false;
	}

	unsigned short curvesCount;
	READ_BIG_ENDIAN_USHORT(ifs, curvesCount);

	for (short i = 0; i < curvesCount; ++i)
	{
		unsigned short curvePointsCount;
		READ_BIG_ENDIAN_USHORT(ifs, curvePointsCount);

		CurvePoints curvePoints(curvePointsCount);

		for (int j = 0; j < curvePointsCount; ++j)
		{
			unsigned short x, y;
			READ_BIG_ENDIAN_USHORT(ifs, y);
			READ_BIG_ENDIAN_USHORT(ifs, x);

			curvePoints[j]  = std::make_pair((float)x, (float)y);
		}

		outCubicSplines.push_back(CubicSpline::InterpolateCubicSplineFromCurvePoints(curvePoints));
	}

	return true;
}

int wmain(int argc, wchar_t* argv[])
{
	std::vector<CubicSpline> cubicSplines;

	if (argc != 3)
	{
		std::cout << "Usage: " << argv[0] << " acv_filename output_filename" << std::endl;
		return -1;
	}

	if (!ReadACVFile(argv[1], cubicSplines))
	{
		return -2;
	}

	if (cubicSplines.size() != 5)
	{
		std::cerr << "ACV file contains an extraordinary amount of curves (" << cubicSplines.size() << ")" << std::endl;
		return -3;
	}

	D3DXVolumeTextureSaver saver;

	const size_t CUBE_SIZE = 16;
	std::vector<float> red(CUBE_SIZE);
	std::vector<float> green(CUBE_SIZE);
	std::vector<float> blue(CUBE_SIZE);

	for (size_t i = 0; i < CUBE_SIZE; ++i)
	{
		float input = (float)i / (float)(CUBE_SIZE - 1);
		float input255 = input * 255.0f;

		float redValue   = clamp(cubicSplines[1].ComputeAtPoint(input255), 0.0f, 255.0f);
		float greenValue = clamp(cubicSplines[2].ComputeAtPoint(input255), 0.0f, 255.0f);
		float blueValue  = clamp(cubicSplines[3].ComputeAtPoint(input255), 0.0f, 255.0f);

		red[i]   = saturate(cubicSplines[0].ComputeAtPoint(redValue) / 255.0f);
		green[i] = saturate(cubicSplines[0].ComputeAtPoint(greenValue) / 255.0f);
		blue[i]  = saturate(cubicSplines[0].ComputeAtPoint(blueValue) / 255.0f);
	}

	saver.SaveToVolumeTexture(red, green, blue, argv[2]);
}