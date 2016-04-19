//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-06-06
//  email:  chongyangm@gmail.com
//  info: class declaration of a graph node
// --------------------------------------------------------------

#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <iostream>
#include <vector>
#include <string>

class CGraphNode
{
public:
	CGraphNode()
	{
		RandomlyInitPos();
		m_flagVisited = false;
		m_flagFixed = false;
		m_type = 0;
		m_boundaryType = 0;
	}

	CGraphNode(std::string name)
	{
		m_name = name;
		RandomlyInitPos();
	}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	void RandomlyInitPos()
	{
		for ( int i=0; i<2; i++ )
		{
			float p = rand() / float(RAND_MAX);
			p -= 0.5f;
			p *= 1.5f;
			m_pos[i] = p;
		}
	}

	v2f GetPos() { return m_pos; }
	void SetPos(v2f pos) { m_pos = pos; }
	void SetPos(float px, float py)
	{
		m_pos[0] = px;
		m_pos[1] = py;
	}

	void ClearNeighbors() { m_neighbors.clear(); }
	void AddNeighbor(int idx) { m_neighbors.push_back(idx); }
	std::vector<int>& GetNeighbors() { return m_neighbors; }
	bool IsNeighbor(int idx)
	{
		for ( int i=0; i<int(m_neighbors.size()); i++ )
		{
			if ( m_neighbors[i] == idx )
			{
				return true;
			}
		}
		return false;
	}

	bool GetFlagVisited() { return m_flagVisited; }
	void SetFlagVisited(bool flagVisited) { m_flagVisited = flagVisited; }

	bool GetFlagFixed() const { return m_flagFixed; }
	void SetFlagFixed(bool flagFixed) { m_flagFixed = flagFixed; }

	int GetType() { return m_type; }
	void SetType(int type) { m_type = type; }

	int GetBoundaryType() const { return m_boundaryType; }
	void SetBoundaryType(int type) { m_boundaryType = type; }

private:
	std::string m_name;
	v2f m_pos;
	std::vector<int> m_neighbors;
	bool m_flagVisited;
	bool m_flagFixed;
	int m_type; // index of the room template
	int m_boundaryType;
};

#endif // GRAPHNODE_H
