//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-06-15
//  email:  chongyangm@gmail.com
//  info: wrapper for basic math operations
// --------------------------------------------------------------

#ifndef LEVELMATH_H
#define LEVELMATH_H

#include "Room.h"

namespace level_math
{
typedef struct PrSort
{
    v2f m_pr;
    float m_dp; // dot product
} PrSort;

const float g_numericalTolerance = 1e-4f; //1e-6;

const float g_numericalToleranceSq = g_numericalTolerance * g_numericalTolerance;

float PointToSegmentSqDistance(const v2f& pt, const CLineBase& line);

float PointToLineSqDistance(const v2f& pt, const CLineBase& line);

float PointToLineSqDistance(const v2f& pt, const v2f& p1, const v2f& p2);

float RoomPerimeter(const CRoom& room1);

float RoomContact(const CRoom& room1, const CRoom& room2);

float RoomContact(const CRoom& room1, const CRoom& room2, int& edgeIdx1, int& edgeIdx2);

float EdgeContact(const CLineBase& line1, const CLineBase& line2);

float RoomDistance(const CRoom& room1, const CRoom& room2);

bool SegmentIntersection(v2f pa, v2f pb, v2f pc, v2f pd, v2f& pi);

bool SegmentIntersection(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Dx, float Dy, float& Ix, float& Iy);

bool LineIntersection(v2f pa, v2f pb, v2f pc, v2f pd, v2f& pi);

bool LineIntersection(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Dx, float Dy, float& Ix, float& Iy);

bool ComparePrSmallerFirst(const PrSort& pr1, const PrSort& pr2);

void SortVecPr(std::vector<v2f>& vecPr);

v3f randomColorFromIndex(int idx);
} // namespace level_math

#endif // LEVELMATH_H
