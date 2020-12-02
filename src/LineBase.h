//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-08-22
//  email:  chongyangm@gmail.com
//  info: base class declaration of a line-shaped object
// --------------------------------------------------------------

#ifndef LINEBASE_H
#define LINEBASE_H

#include "vec.h"

class CLineBase
{
public:
    CLineBase()
    {
        m_pos1 = v2f(0.f);
        m_pos2 = v2f(0.f);
    }

    CLineBase(v2f pos) :
        m_pos1(pos), m_pos2(pos)
    {
    }

    CLineBase(v2f pos1, v2f pos2) :
        m_pos1(pos1), m_pos2(pos2)
    {
    }

    float GetLength() const { return mag(m_pos2 - m_pos1); }

    float GetSqLength() const { return mag2(m_pos2 - m_pos1); }

    v2f GetPos1() const { return m_pos1; }
    v2f GetPos2() const { return m_pos2; }

    void SetPos1(v2f pos1) { m_pos1 = pos1; }
    void SetPos2(v2f pos2) { m_pos2 = pos2; }

protected:
    v2f m_pos1;
    v2f m_pos2;
};

#endif // LINEBASE_H
