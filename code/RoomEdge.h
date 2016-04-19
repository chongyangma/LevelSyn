//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-08-22
//  email:  chongyangm@gmail.com
//  info: class declaration of a room edge
// --------------------------------------------------------------

#ifndef ROOMEDGE_H
#define ROOMEDGE_H

#include "LineBase.h"

class CRoomEdge : public CLineBase
{
public:
	CRoomEdge()
	{
		m_idx1 = -1;
		m_idx2 = -1;
		m_doorFlag = true;
	}

	CRoomEdge(v2f pos1, v2f pos2) : CLineBase(pos1, pos2)
	{
		m_idx1 = -1;
		m_idx2 = -1;
		m_doorFlag = true;
	}

	CRoomEdge(v2f pos1, v2f pos2, int idx1, int idx2) : CLineBase(pos1, pos2), m_idx1(idx1), m_idx2(idx2)
	{
		m_doorFlag = true;
	}

	v2f GetDirection() { return (m_pos2 - m_pos1); }

	v3f GetDirection3D() { return v3f(GetDirection()[0], GetDirection()[1], 0.f); }

	int GetIdx1() const { return m_idx1; }
	int GetIdx2() const { return m_idx2; }

	void SetIdx1(int idx1) { m_idx1 = idx1; }
	void SetIdx2(int idx2) { m_idx2 = idx2; }

	void SetDoorFlag(bool flag) { m_doorFlag = flag; }
	bool GetDoorFlag() { return m_doorFlag; }

private:
	int m_idx1;
	int m_idx2;

	bool m_doorFlag;
};

#endif // ROOMEDGE_H
