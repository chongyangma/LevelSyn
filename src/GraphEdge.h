//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-06-06
//  email:  chongyangm@gmail.com
//  info: class declaration of a graph edge
// --------------------------------------------------------------

#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include <iostream>
#include <vector>

class CGraphEdge
{
public:
	CGraphEdge(int idx0, int idx1)
	{
		m_indices.resize(2);
		m_indices[0] = idx0;
		m_indices[1] = idx1;
	}

	std::vector<int>& GetIndices() { return m_indices; }

	int GetIdx0() const { return m_indices[0]; }
	int GetIdx1() const { return m_indices[1]; }

	void SetIdx0(int idx0) { m_indices[0] = idx0; }
	void SetIdx1(int idx1) { m_indices[1] = idx1; }

private:
	std::vector<int> m_indices;
};

#endif // GRAPHEDGE_H
