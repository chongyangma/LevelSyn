#include "LevelMath.h"

namespace level_math
{
float PointToSegmentSqDistance(const v2f& pt, const CLineBase& line)
{
    if (line.GetSqLength() < g_numericalTolerance * g_numericalTolerance)
    {
        return mag2(pt - line.GetPos1());
    }
    float d1 = mag2(pt - line.GetPos1());
    float d2 = mag2(pt - line.GetPos2());
    v2f pe = line.GetPos2() - line.GetPos1();
    v2f pd = pt - line.GetPos1();
    float dp = dot(pe, pd);
    float r = dp / mag2(pe);
    float d;
    if (r >= 1.f)
    {
        d = d2;
    }
    else if (r <= 0.f)
    {
        d = d1;
    }
    else
    {
        v2f peNew = v2f(pe[1], -pe[0]);
        d = std::abs(dot(pd, peNew) / mag(peNew));
        d = d * d;
    }
    return d;
}

float PointToLineSqDistance(const v2f& pt, const CLineBase& line)
{
    return PointToLineSqDistance(pt, line.GetPos2(), line.GetPos1());
}

float PointToLineSqDistance(const v2f& pt, const v2f& p1, const v2f& p2)
{
    v2f pe = p2 - p1;
    v2f peNorm = normalize(pe);
    v2f pr = pt - p1;
    v3f peNew = v3f(peNorm[0], peNorm[1], 0.f);
    v3f prNew = v3f(pr[0], pr[1], 0.f);
    v3f cp = cross(peNew, prNew);
    float d = mag2(cp);
    return d;
}

float RoomPerimeter(const CRoom& room1)
{
    float contactArea = 0.f;
    for (int i = 0; i < room1.GetNumOfEdges(); i++)
    {
        CRoomEdge edge1 = room1.GetEdge(i);
        contactArea += edge1.GetLength();
    }

    return contactArea;
}

float RoomContact(const CRoom& room1, const CRoom& room2)
{
    float contactArea = 0.f;
    for (int i = 0; i < room1.GetNumOfEdges(); i++)
    {
        CRoomEdge edge1 = room1.GetEdge(i);
        for (int j = 0; j < room2.GetNumOfEdges(); j++)
        {
            CRoomEdge edge2 = room2.GetEdge(j);
            if (edge1.GetDoorFlag() == false || edge2.GetDoorFlag() == false)
            {
                continue;
            }
            float contactAreaTmp = EdgeContact(edge1, edge2);
            contactArea += contactAreaTmp;
        }
    }

    return contactArea;
}

float RoomContact(const CRoom& room1, const CRoom& room2, int& edgeIdx1, int& edgeIdx2)
{
    float contactAreaMax = 0.f;
    for (int i = 0; i < room1.GetNumOfEdges(); i++)
    {
        CRoomEdge edge1 = room1.GetEdge(i);
        for (int j = 0; j < room2.GetNumOfEdges(); j++)
        {
            CRoomEdge edge2 = room2.GetEdge(j);
            if (edge1.GetDoorFlag() == false || edge2.GetDoorFlag() == false)
            {
                continue;
            }
            float contactAreaTmp = EdgeContact(edge1, edge2);
            if (contactAreaTmp > contactAreaMax)
            {
                contactAreaMax = contactAreaTmp;
                edgeIdx1 = i;
                edgeIdx2 = j;
            }
        }
    }

    return contactAreaMax;
}

float EdgeContact(const CLineBase& line1, const CLineBase& line2)
{
    const float numericalTolerance = g_numericalTolerance * 100.f;
    const float numericalToleranceSq = numericalTolerance * numericalTolerance;
    v2f pr1 = line1.GetPos2() - line1.GetPos1();
    v2f pr2 = line2.GetPos2() - line2.GetPos1();
    v3f pe1 = v3f(pr1[0], pr1[1], 0.f);
    v3f pe2 = v3f(pr2[0], pr2[1], 0.f);
    v3f cp = cross(pe1, pe2);
    if (mag2(cp) > numericalTolerance)
    {
        return 0.f;
    }
    v2f posMin1 = min_union(line1.GetPos1(), line1.GetPos2());
    v2f posMax1 = max_union(line1.GetPos1(), line1.GetPos2());
    v2f posMin2 = min_union(line2.GetPos1(), line2.GetPos2());
    v2f posMax2 = max_union(line2.GetPos1(), line2.GetPos2());
    for (int j = 0; j < 2; j++)
    {
        if (posMax1[j] < posMin2[j] - numericalTolerance || posMin1[j] > posMax2[j] + numericalTolerance)
        {
            return 0.f;
        }
    }
    float d1 = PointToLineSqDistance(line2.GetPos1(), line1);
    float d2 = PointToLineSqDistance(line2.GetPos2(), line1);
    if (d1 > numericalToleranceSq || d2 > numericalToleranceSq)
    {
        return 0.f;
    }
    // Now the two edges should in the same line anyway...
    float len1 = mag(pe1);
    float len2 = mag(pe2);
    float d11 = mag2(line1.GetPos1() - line2.GetPos1());
    float d21 = mag2(line1.GetPos2() - line2.GetPos1());
    float d12 = mag2(line1.GetPos1() - line2.GetPos2());
    float d22 = mag2(line1.GetPos2() - line2.GetPos2());
    float dMax = sqrt(max(max(d11, d21), max(d12, d22)));
    dMax = max(dMax, max(len1, len2));
    float contactArea = len1 + len2 - dMax;
    contactArea = max(contactArea, 0.f);
    return contactArea;
}

float RoomDistance(const CRoom& room1, const CRoom& room2)
{
    float d = 1e10;
    for (int i = 0; i < room1.GetNumOfVertices(); i++)
    {
        v2f pt = room1.GetVertex(i);
        for (int j = 0; j < room2.GetNumOfEdges(); j++)
        {
            CRoomEdge edge = room2.GetEdge(j);
            float dTmp = PointToSegmentSqDistance(pt, edge);
            d = min(d, dTmp);
        }
    }
    d = sqrt(d);
    return d;
}

bool SegmentIntersection(v2f pa, v2f pb, v2f pc, v2f pd, v2f& pi)
{
    return SegmentIntersection(pa[0], pa[1], pb[0], pb[1], pc[0], pc[1], pd[0], pd[1], pi[0], pi[1]);
}

// Based on the example under http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
bool SegmentIntersection(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Dx, float Dy, float& Ix, float& Iy)
{
    float Rx = Bx - Ax;
    float Ry = By - Ay;
    float Sx = Dx - Cx;
    float Sy = Dy - Cy;
    float QPx = Cx - Ax;
    float QPy = Cy - Ay;
    float rs = Rx * Sy - Ry * Sx;
    if (rs == 0.f)
    {
        return false;
    }
    float t = (QPx * Sy - QPy * Sx) / rs;
    float u = (QPx * Ry - QPy * Rx) / rs;
    if (t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f)
    {
        Ix = Ax + t * Rx;
        Iy = Ay + t * Ry;
        return true;
    }
    else
    {
        return false;
    }
}

bool LineIntersection(v2f pa, v2f pb, v2f pc, v2f pd, v2f& pi)
{
    return LineIntersection(pa[0], pa[1], pb[0], pb[1], pc[0], pc[1], pd[0], pd[1], pi[0], pi[1]);
}

bool LineIntersection(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Dx, float Dy, float& Ix, float& Iy)
{
    float Rx = Bx - Ax;
    float Ry = By - Ay;
    float Sx = Dx - Cx;
    float Sy = Dy - Cy;
    float QPx = Cx - Ax;
    float QPy = Cy - Ay;
    float rs = Rx * Sy - Ry * Sx;
    if (rs == 0.f)
    {
        return false;
    }
    float t = (QPx * Sy - QPy * Sx) / rs;
    Ix = Ax + t * Rx;
    Iy = Ay + t * Ry;
    return true;
}

bool ComparePrSmallerFirst(const PrSort& pr1, const PrSort& pr2)
{
    return (pr1.m_dp < pr2.m_dp);
}

void SortVecPr(std::vector<v2f>& vecPr)
{
    if (vecPr.size() < 2)
    {
        return;
    }
    v2f pd = vecPr[1] - vecPr[0];
    std::vector<PrSort> vecPrSort(vecPr.size());
    for (int i = 0; i < int(vecPrSort.size()); i++)
    {
        vecPrSort[i].m_pr = vecPr[i];
        vecPrSort[i].m_dp = dot(pd, vecPr[i] - vecPr[0]);
    }
    sort(vecPrSort.begin(), vecPrSort.end(), ComparePrSmallerFirst);
    for (int i = 0; i < int(vecPrSort.size()); i++)
    {
        vecPr[i] = vecPrSort[i].m_pr;
    }
}

v3f randomColorFromIndex(int idx)
{
    static std::vector<v3f> clrs;
    if (clrs.empty())
    {
        clrs.resize(256);
        for (int c = 0; c < clrs.size(); c++)
        {
            clrs[c] = v3f(rand(), rand(), rand());
            clrs[c] = clrs[c] / max(clrs[c]);
        }
    }
    return (clrs[idx & 255]);
}
} // namespace level_math
