
#include "clipperWrapper.h"

float CClipperWrapper::m_scalingFactor = 1e10;

Paths CClipperWrapper::FindIntersection(const CRoom& room1, const CRoom& room2)
{
	Path p1, p2;
	p1.resize(room1.GetNumOfVertices());
	p2.resize(room2.GetNumOfVertices());

	for ( int i=0; i<room1.GetNumOfVertices(); i++ )
	{
		v2f pi = room1.GetVertex(i);
		p1[i].X = ConvertFloatToLong64(pi[0]);
		p1[i].Y = ConvertFloatToLong64(pi[1]);
	}
	for ( int i=0; i<room2.GetNumOfVertices(); i++ )
	{
		v2f pi = room2.GetVertex(i);
		p2[i].X = ConvertFloatToLong64(pi[0]);
		p2[i].Y = ConvertFloatToLong64(pi[1]);
	}

	ClipType ct = ctIntersection;
	PolyFillType pft = pftNonZero;
	Paths sub, clp, sol;

	Clipper c;
	c.AddPath(p1, ptSubject, true);
	c.AddPath(p2, ptClip, true);
	c.Execute(ct, sol, pft, pft);

	return sol;
}

float CClipperWrapper::ComputeCollideArea(const CRoom& room1, const CRoom& room2)
{
	Paths sol = FindIntersection(room1, room2);
	float collideArea = 0.f;
	for ( int i=0; i<int(sol.size()); i++ )
	{
		double a = Area(sol[i]);
		collideArea += std::abs(ConvertDoubleAreaToFloat(a));
	}
	if (collideArea < 10e-4)
	{
		collideArea = 0;
	}
	return collideArea;
}

float CClipperWrapper::ComputeRoomArea(const CRoom& room)
{
	return ComputeCollideArea(room, room);
}

long64 CClipperWrapper::ConvertFloatToLong64(float f)
{
	long64 i = long64(f * m_scalingFactor + 0.5f);
	return i;
}

float CClipperWrapper::ConvertLong64ToFloat(long64 i)
{
	float f = float(i) / m_scalingFactor;
	return f;
}

float CClipperWrapper::ConvertDoubleAreaToFloat(double a)
{
	a = a / double(m_scalingFactor * m_scalingFactor);
	float f = float(a);
	return f;
}
