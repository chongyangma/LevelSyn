//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-08-17
//  email:  chongyangm@gmail.com
//  info: class declaration of a graph chain
// --------------------------------------------------------------

#ifndef GRAPHCHAIN_H
#define GRAPHCHAIN_H

#include <iostream>
#include <vector>

class CGraphChain
{
public:
	CGraphChain() {}

	CGraphChain(std::vector<int>& indices, bool flag)
	{
		m_indices = indices;
		m_flagCyclic = flag;
	}

	std::vector<int>& GetIndices() { return m_indices; }

	void AddIndex(int idx) { m_indices.push_back(idx); }

	void ClearIndices() { m_indices.clear(); }

	int GetChainSize() { return int(m_indices.size()); }

	bool IsEmpty() { return m_indices.empty(); }

	bool GetFlagCyclic() { return m_flagCyclic; }

	void SetFlagCyclic(bool flag) { m_flagCyclic = flag; }

private:
	std::vector<int> m_indices; // graph nodes that form a chain
	bool m_flagCyclic;
};

#endif // GRAPHCHAIN_H
