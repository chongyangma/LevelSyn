//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-06-06
//  email:  chongyangm@gmail.com
//  info: class declaration of a graph face
// --------------------------------------------------------------

#ifndef GRAPHFACE_H
#define GRAPHFACE_H

#include <iostream>
#include <vector>

class CGraphFace
{
public:
    CGraphFace() {}

    CGraphFace(std::vector<int>& indices)
    {
        m_indices = indices;
    }

    std::vector<int>& GetIndices() { return m_indices; }

    void AddIndex(int idx) { m_indices.push_back(idx); }

    void ClearIndices() { m_indices.clear(); }

    int GetFaceSize() { return int(m_indices.size()); }

    bool IsEmpty() { return m_indices.empty(); }

private:
    std::vector<int> m_indices; // graph nodes that form a face
};

#endif // GRAPHFACE_H
