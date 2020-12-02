#include "ConfigSpace.h"

#include "LevelConfig.h"

v2f CConfigLine::RandomlySampleConfigLine()
{
    float wt1 = rand() / float(RAND_MAX);
    float wt2 = 1.f - wt1;
    v2f pos = wt1 * m_pos1 + wt2 * m_pos2;
    return pos;
}

v2f CConfigLine::RandomlySampleConfigLineDiscrete()
{
    float r = rand() / float(RAND_MAX);
    v2f pos = (r >= 0.5f) ? m_pos1 : m_pos2;
    return pos;
}

float CConfigLine::GetConfigLineLength() const
{
    return mag(m_pos1 - m_pos2);
}

float CConfigLine::GetConfigLineSqLength() const
{
    return mag2(m_pos1 - m_pos2);
}

void CConfigLine::PrintConfigLine()
{
    std::cout << "p1: " << m_pos1 << "; p2: " << m_pos2 << std::endl;
}

void CConfigLine::TranslateConfigLine(v2f trans)
{
    m_pos1 = m_pos1 + trans;
    m_pos2 = m_pos2 + trans;
}

std::vector<std::vector<CConfigSpace>> CConfigSpace::m_precomputedTable;
bool CConfigSpace::m_flagPrecomputed = false;

#define ACCURATE_CONFIG_SPACE // New on 09/20/2013

CConfigSpace::CConfigSpace(const CRoom& room1, const CRoom& room2)
{
    int type1 = room1.GetTemplateType();
    int type2 = room2.GetTemplateType();
    if (m_flagPrecomputed == true && type1 >= 0 && type2 >= 0)
    {
        CConfigSpace cs = m_precomputedTable[type1][type2];
        cs.TranslateConfigSpace(room1.GetRoomCenter());
        *this = cs;
        return;
    }
    //cout << "Construct config space on the fly for room pair (" << type1 << ", " << type2 << ")...\n";
    CClipperWrapper wrapper;
    const float contactThresh = CLevelConfig::m_roomContactThresh * 0.5f; // Chongyang: why this 0.5 makes everything better?
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
            v3f cp = cross(edge1.GetDirection3D(), edge2.GetDirection3D());
            if (mag2(cp) > g_numericalTolerance)
            {
                continue;
            }
            std::vector<v2f> vecPr(4);
            vecPr[0] = edge1.GetPos1() - edge2.GetPos1();
            vecPr[1] = edge1.GetPos1() - edge2.GetPos2();
            vecPr[2] = edge1.GetPos2() - edge2.GetPos1();
            vecPr[3] = edge1.GetPos2() - edge2.GetPos2();
#ifdef ACCURATE_CONFIG_SPACE
            v2f dir = edge1.GetDirection();
            dir = normalize(dir);
            v2f shift = dir * contactThresh;
            for (int k = 0; k < 4; k++)
            {
                vecPr.push_back(vecPr[k] + shift);
                vecPr.push_back(vecPr[k] - shift);
            }
#endif
            SortVecPr(vecPr);
            for (int k = 1; k < int(vecPr.size()); k++)
            {
                v2f pr1 = vecPr[k];
                v2f pr2 = vecPr[k - 1];
                if (mag2(pr2 - pr1) < g_numericalTolerance)
                {
                    continue;
                }
                v2f pr3 = (pr1 + pr2) * 0.5f;
                CRoom room2n1 = room2;
                room2n1.TranslateRoom(pr1);
                if (wrapper.ComputeCollideArea(room1, room2n1) > g_numericalTolerance)
                {
                    continue;
                }
#ifdef ACCURATE_CONFIG_SPACE
                if (RoomContact(room1, room2n1) < contactThresh - g_numericalTolerance)
                {
                    continue;
                }
#endif
                CRoom room2n2 = room2;
                room2n2.TranslateRoom(pr2);
                if (wrapper.ComputeCollideArea(room1, room2n2) > g_numericalTolerance)
                {
                    continue;
                }
#ifdef ACCURATE_CONFIG_SPACE
                if (RoomContact(room1, room2n2) < contactThresh - g_numericalTolerance)
                {
                    continue;
                }
#endif
                CRoom room2n3 = room2;
                room2n3.TranslateRoom(pr3);
                if (wrapper.ComputeCollideArea(room1, room2n3) > g_numericalTolerance)
                {
                    continue;
                }
#ifdef ACCURATE_CONFIG_SPACE
                if (RoomContact(room1, room2n3) < contactThresh - g_numericalTolerance)
                {
                    continue;
                }
#endif
                v2f pos1 = room2.GetRoomCenter() + pr1;
                v2f pos2 = room2.GetRoomCenter() + pr2;
                CConfigLine line(pos1, pos2);
                AddConfigLine(line);
            }
        }
    }
    SelfMerge();
}

CConfigSpace::CConfigSpace(std::vector<CConfigLine>& vecConfigLines)
{
    m_vecConfigLine = vecConfigLines;
    SelfMerge();
}

v2f CConfigSpace::RandomlySampleConfigSpace()
{
    float r = rand() / float(RAND_MAX);
    v2f pos = (r >= 0.5f) ? RandomlySampleConfigSpaceContinuous() : RandomlySampleConfigSpaceDiscrete();
    return pos;
}

v2f CConfigSpace::RandomlySampleConfigSpaceContinuous()
{
    int numOfLines = GetNumOfLines();
    int lineIndex = int(rand() / float(RAND_MAX) * numOfLines);
    lineIndex = lineIndex % numOfLines;
    v2f pos = m_vecConfigLine[lineIndex].RandomlySampleConfigLine();
    return pos;
}

v2f CConfigSpace::RandomlySampleConfigSpaceDiscrete()
{
    int numOfLines = GetNumOfLines();
    int lineIndex = int(rand() / float(RAND_MAX) * numOfLines);
    lineIndex = lineIndex % numOfLines;
    v2f pos = m_vecConfigLine[lineIndex].RandomlySampleConfigLineDiscrete();
    return pos;
}

std::vector<v2f> CConfigSpace::SmartlySampleConfigSpace()
{
    int numOfLines = GetNumOfLines();
    std::vector<v2f> vecPos(numOfLines);
    for (int i = 0; i < numOfLines; i++)
    {
        CConfigLine& configLine = GetConfigLine(i);
        float r = rand() / float(RAND_MAX);
        v2f pos = (r >= 0.5f) ? configLine.RandomlySampleConfigLine() : configLine.RandomlySampleConfigLineDiscrete();
        vecPos[i] = pos;
    }
    return vecPos;
}

CConfigSpace CConfigSpace::FindIntersection(CConfigSpace& configSpace1, CConfigSpace& configSpace2)
{
    CConfigSpace intersectSpace;
    for (int i1 = 0; i1 < configSpace1.GetNumOfLines(); i1++)
    {
        CConfigLine& configLine1 = configSpace1.GetConfigLine(i1);
        for (int i2 = 0; i2 < configSpace2.GetNumOfLines(); i2++)
        {
            CConfigLine& configLine2 = configSpace2.GetConfigLine(i2);
            if (configLine1.GetConfigLineSqLength() < g_numericalToleranceSq && configLine2.GetConfigLineSqLength() < g_numericalToleranceSq)
            {
                if (mag2(configLine1.GetPos1() - configLine2.GetPos1()) < g_numericalToleranceSq)
                {
                    intersectSpace.AddConfigLine(configLine1);
                }
                continue;
            }
            else if (configLine1.GetConfigLineSqLength() < g_numericalToleranceSq)
            {
                CRoomEdge edge(configLine2.GetPos1(), configLine2.GetPos2());
                if (PointToSegmentSqDistance(configLine1.GetPos1(), edge) < g_numericalToleranceSq)
                {
                    intersectSpace.AddConfigLine(configLine1);
                }
                continue;
            }
            else if (configLine2.GetConfigLineSqLength() < g_numericalToleranceSq)
            {
                CRoomEdge edge(configLine1.GetPos1(), configLine1.GetPos2());
                if (PointToSegmentSqDistance(configLine2.GetPos1(), edge) < g_numericalToleranceSq)
                {
                    intersectSpace.AddConfigLine(configLine2);
                }
                continue;
            }
            v2f p11 = configLine1.GetPos1();
            v2f p12 = configLine1.GetPos2();
            v2f p21 = configLine2.GetPos1();
            v2f p22 = configLine2.GetPos2();
            v3f pe1 = v3f(p12[0] - p11[0], p12[1] - p11[1], 0.f);
            v3f pe2 = v3f(p22[0] - p21[0], p22[1] - p21[1], 0.f);
            v3f cp = cross(pe1, pe2);
            if (mag2(cp) > g_numericalTolerance)
            {
                // Not parallel...
                v2f pi;
                bool flagIntersect = SegmentIntersection(p11, p12, p21, p22, pi);
                if (flagIntersect == true)
                {
                    CConfigLine intersectLine(pi);
                    intersectSpace.AddConfigLine(intersectLine);
                }
            }
            else
            {
                // Parallel...
                v2f posMin1 = min_union(p11, p12);
                v2f posMax1 = max_union(p11, p12);
                v2f posMin2 = min_union(p21, p22);
                v2f posMax2 = max_union(p21, p22);
                bool flagOverlap = true;
                for (int j = 0; j < 2; j++)
                {
                    if (posMax1[j] < posMin2[j] - g_numericalTolerance || posMin1[j] > posMax2[j] + g_numericalTolerance)
                    {
                        flagOverlap = false;
                        break;
                    }
                }
                if (flagOverlap == false)
                {
                    continue;
                }
                float d1 = PointToLineSqDistance(p21, p12, p11);
                float d2 = PointToLineSqDistance(p22, p12, p11);
                if (d1 > g_numericalToleranceSq || d2 > g_numericalToleranceSq)
                {
                    flagOverlap = false;
                }
                if (flagOverlap == false)
                {
                    continue;
                }
                v2f p1, p2;
                for (int d = 0; d < 2; d++)
                {
                    p1[d] = max(min(p11[d], p12[d]), min(p21[d], p22[d]));
                    p2[d] = min(max(p11[d], p12[d]), max(p21[d], p22[d]));
                }
                CConfigLine intersectLine(p1, p2);
                intersectSpace.AddConfigLine(intersectLine);
            }
        }
    }
    return intersectSpace;
}

CConfigSpace CConfigSpace::FindUnion(CConfigSpace& configSpace, CConfigLine& configLine)
{
    CConfigSpace configSpaceNew = configSpace;
    if (configSpace.GetNumOfLines() == 0)
    {
        configSpaceNew.AddConfigLine(configLine);
        return configSpaceNew;
    }
    bool mergeFlag = false;
    for (int i = 0; i < configSpace.GetNumOfLines(); i++)
    {
        CConfigLine& line = configSpaceNew.GetConfigLine(i);
        CRoomEdge edge1(line.GetPos1(), line.GetPos2());
        CRoomEdge edge2(configLine.GetPos1(), configLine.GetPos2());
        float sqlength1 = edge1.GetSqLength();
        float sqlength2 = edge2.GetSqLength();
        if (sqlength1 >= g_numericalTolerance && sqlength2 >= g_numericalTolerance)
        {
            v3f cp = cross(edge1.GetDirection3D(), edge2.GetDirection3D());
            if (mag2(cp) > g_numericalTolerance)
            {
                continue;
            }
        }
        else if (sqlength1 < g_numericalTolerance && sqlength2 > g_numericalTolerance)
        {
            continue;
        }
        else if (sqlength1 < g_numericalTolerance && sqlength2 < g_numericalTolerance)
        {
            if (mag2(edge1.GetPos1() - edge2.GetPos1()) < g_numericalTolerance)
            {
                mergeFlag = true;
                break;
            }
            continue;
        }
        if (PointToSegmentSqDistance(edge1.GetPos1(), edge2) < g_numericalToleranceSq || PointToSegmentSqDistance(edge1.GetPos2(), edge2) < g_numericalToleranceSq)
        {
            v2f posMin1 = min_union(edge1.GetPos1(), edge1.GetPos2());
            v2f posMax1 = max_union(edge1.GetPos1(), edge1.GetPos2());
            v2f posMin2 = min_union(edge2.GetPos1(), edge2.GetPos2());
            v2f posMax2 = max_union(edge2.GetPos1(), edge2.GetPos2());
            v2f posMin = min_union(posMin1, posMin2);
            v2f posMax = max_union(posMax1, posMax2);
            v2f pos1, pos2;
            for (int j = 0; j < 2; j++)
            {
                pos1[j] = (line.GetPos1()[j] == posMin1[j]) ? posMin[j] : posMax[j];
                pos2[j] = (line.GetPos1()[j] == posMin1[j]) ? posMax[j] : posMin[j];
            }
            line.SetPos1(pos1);
            line.SetPos2(pos2);
            mergeFlag = true;
            break;
        }
    }
    if (mergeFlag == false)
    {
        configSpaceNew.AddConfigLine(configLine);
    }
    return configSpaceNew;
}

void CConfigSpace::SelfMerge()
{
    sort(m_vecConfigLine.begin(), m_vecConfigLine.end(), CompareConfigLineLength);
    CConfigSpace configSpaceNew;
    for (int i = 0; i < GetNumOfLines(); i++)
    {
        configSpaceNew = FindUnion(configSpaceNew, GetConfigLine(i));
    }
    SetConfigLines(configSpaceNew.GetConfigLines());
}

float CConfigSpace::GetConfigSpaceSize()
{
    float sz = 0.f;
    for (int i = 0; i < GetNumOfLines(); i++)
    {
        sz += GetConfigLine(i).GetConfigLineLength();
    }
    return sz;
}

void CConfigSpace::PrintConfigSpace()
{
    for (int i = 0; i < GetNumOfLines(); i++)
    {
        std::cout << "The " << i << "th line:\n";
        GetConfigLine(i).PrintConfigLine();
    }
}

void CConfigSpace::TranslateConfigSpace(v2f trans)
{
    for (int i = 0; i < GetNumOfLines(); i++)
    {
        GetConfigLine(i).TranslateConfigLine(trans);
    }
}

bool CConfigSpace::CompareConfigLineLength(const CConfigLine& line1, const CConfigLine& line2)
{
    return (line1.GetConfigLineSqLength() > line2.GetConfigLineSqLength());
}

void CConfigSpace::PrecomputeTable(const std::vector<CRoom>& vecRooms)
{
    m_flagPrecomputed = false;
    m_precomputedTable.clear();
    int numOfRooms = int(vecRooms.size());
    m_precomputedTable.resize(numOfRooms);
    //cout << "Pre-compute configuration space table for " << numOfRooms << " rooms...\n";
    for (int i = 0; i < numOfRooms; i++)
    {
        std::vector<CConfigSpace> vecConfigSpace(numOfRooms);
        CRoom room1 = vecRooms[i];
        v2f centerPos = room1.GetRoomCenter();
        room1.TranslateRoom(-centerPos);
        for (int j = 0; j < numOfRooms; j++)
        {
            vecConfigSpace[j] = CConfigSpace(room1, vecRooms[j]);
        }
        m_precomputedTable[i] = vecConfigSpace;
    }
    m_flagPrecomputed = true;
}
