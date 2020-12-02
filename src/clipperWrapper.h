//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-06-24
//  email:  chongyangm@gmail.com
//  info: wrapper of the Clipper library
// --------------------------------------------------------------

#ifndef CLIPPERWRAPPER_H
#define CLIPPERWRAPPER_H

#include "clipper.hpp"
using namespace ClipperLib;

#include "LevelMath.h"
#include "RoomLayout.h"
using namespace level_math;

class CClipperWrapper
{
public:
    Paths FindIntersection(const CRoom& room1, const CRoom& room2);

    float ComputeCollideArea(const CRoom& room1, const CRoom& room2);

    float ComputeRoomArea(const CRoom& room);

    static float m_scalingFactor;

private:
    long64 ConvertFloatToLong64(float f);

    float ConvertLong64ToFloat(long64 i);

    float ConvertDoubleAreaToFloat(double a);
};

#endif //CLIPPERWRAPPER_H
