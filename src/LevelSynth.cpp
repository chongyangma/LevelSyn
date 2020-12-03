#include "LevelSynth.h"

#include <math.h>

int Random2(int max)
{
    if (max < 1 || max >= RAND_MAX)
        return 0;
    else
        return (int)rand() / (RAND_MAX / max + 1);
}

void CurrentState::MoveRoomsToSceneCenter(CPlanarGraph* ptrGraph)
{
    v2f pMin(1e10);
    v2f pMax(-1e10);
    for (int j = 0; j < ptrGraph->GetNumOfNodes(); j++)
    {
        if (ptrGraph->GetNode(j).GetFlagVisited() == false)
        {
            continue;
        }
        v2f pj = m_stateRoomPositions[j];
        for (int k = 0; k < 2; k++)
        {
            pMin[k] = min(pMin[k], pj[k]);
            pMax[k] = max(pMax[k], pj[k]);
        }
    }
    v2f posCen = (pMin + pMax) * 0.5f;
    for (int j = 0; j < ptrGraph->GetNumOfNodes(); j++)
    {
        m_stateRoomPositions[j] = m_stateRoomPositions[j] - posCen;
    }
}

void CurrentState::Move1DchainToSceneCenter(std::vector<int>& indices)
{
    v2f pMin(1e10);
    v2f pMax(-1e10);
    for (int j = 0; j < int(indices.size()); j++)
    {
        int idx = indices[j];
        v2f pj = m_stateRoomPositions[idx];
        for (int k = 0; k < 2; k++)
        {
            pMin[k] = min(pMin[k], pj[k]);
            pMax[k] = max(pMax[k], pj[k]);
        }
    }
    v2f posCen = (pMin + pMax) * 0.5f;
    for (int j = 0; j < int(indices.size()); j++)
    {
        int idx = indices[j];
        m_stateRoomPositions[idx] = m_stateRoomPositions[idx] - posCen;
    }
}

float CurrentState::GetStateDifference(CurrentState& otherState, CPlanarGraph* ptrGraph)
{
    float stateDiff = 0.f;
    for (int j = 0; j < ptrGraph->GetNumOfNodes(); j++)
    {
        // ptrGraph->GetNode(pickedRoomIndex).SetType(typeNew);

        if (ptrGraph->GetNode(j).GetFlagVisited() == false)
        {
            continue;
        }
        /*
		if (ptrGraph->GetNode(j).GetType() != otherState.m_stateGraph.GetNode(j).GetType())
		{
		stateDiff += 2 * CLevelConfig::m_stateDiffThresh;
		}
		*/
        v2f p1 = m_stateRoomPositions[j];
        v2f p2 = otherState.m_stateRoomPositions[j];
        stateDiff += mag2(p1 - p2);
    }
    return stateDiff;
}

bool CurrentState::InsertToNewStates(std::vector<CurrentState>& newStates, CPlanarGraph* ptrGraph)
{
    float stateDiffThresh = CLevelConfig::m_stateDiffThresh;
    if (stateDiffThresh <= 0.f)
    {
        newStates.push_back(*this);
        return true;
    }
    for (int i = 0; i < int(newStates.size()); i++)
    {
        if (m_stateEnergy < newStates[i].m_stateEnergy)
        {
            continue;
        }
        float stateDiff = GetStateDifference(newStates[i], ptrGraph);
        if (stateDiff <= stateDiffThresh)
        {
            return false;
        }
    }
    newStates.push_back(*this);
    return true;
}

CLevelSynth::CLevelSynth()
{
    m_ptrGraph = NULL;
    m_ptrTemplates = NULL;
    m_solutionCount = 0;
    m_bestSolCount = 0;
    m_chainCount = 0;
    m_backTrackCount = 0;
    m_backTrackLevel = 0;
}

CLevelSynth::CLevelSynth(CPlanarGraph* ptrGraph, CRoomTemplates* ptrTemplates)
{
    SetGraphAndTemplates(ptrGraph, ptrTemplates);
}

void CLevelSynth::SetGraphAndTemplates(CPlanarGraph* ptrGraph, CRoomTemplates* ptrTemplates)
{
    m_solutionCount = 0;
    m_bestSolCount = 0;
    ptrGraph->MoveGraphToSceneCenter();
    ptrGraph->ScaleGraphNodePositions(CLevelConfig::m_graphScaling);
    SetGraph(ptrGraph);
    m_ptrTemplates = ptrTemplates;
    m_ptrGraph->SetNumOfTypes(m_ptrTemplates->GetNumOfTemplates());
    m_ptrGraph->RandomInitTypes();

    //m_ptrGraph->RandomInitPositions();
    InitScene();
    SynthesizeScene();
}

void CLevelSynth::SetGraph(CPlanarGraph* ptrGraph)
{
    m_ptrGraph = ptrGraph;
    m_roomPositions.resize(m_ptrGraph->GetNumOfNodes());
    for (int i = 0; i < m_ptrGraph->GetNumOfNodes(); i++)
    {
        v2f pi = m_ptrGraph->GetNodePos(i);
        m_roomPositions[i] = pi;
    }
}

bool CLevelSynth::MovePickedGraphNode(float& dx, float& dy)
{
    m_ptrGraph->MovePickedNode(dx, dy);
    InitScene();
    bool flag = false;
    //flag = AdjustPickedRoom(dx, dy);
    return flag;
}

bool CLevelSynth::AdjustPickedRoom(float& dx, float& dy)
{
    int pickedNodeIndex = m_ptrGraph->GetPickedNodeIndex();
    bool flag = false;
    if (pickedNodeIndex < 0)
    {
        return flag;
    }
    CRoom& pickedRoom = m_layout.GetRoom(pickedNodeIndex);
    for (int i = 0; i < pickedRoom.GetNumOfEdges(); i++)
    {
        CRoomEdge edge = pickedRoom.GetEdge(i);
        v2f pr2 = edge.GetPos2() - edge.GetPos1();
        v3f pr = v3f(pr2[0], pr2[1], 0.f);
        v2f norm = v2f(pr[1], -pr[0]);
        norm = normalize(norm);
        float distMin = 1e10;
        CRoomEdge edgeNearest;
        for (int j = 0; j < m_layout.GetNumOfRooms(); j++)
        {
            if (j == pickedNodeIndex)
            {
                continue;
            }
            CRoom& otherRoom = m_layout.GetRoom(j);
            for (int k = 0; k < otherRoom.GetNumOfEdges(); k++)
            {
                CRoomEdge otherEdge = otherRoom.GetEdge(k);
                v2f otherPr2 = otherEdge.GetPos2() - otherEdge.GetPos1();
                v3f otherPr = v3f(otherPr2[0], otherPr2[1], 0.f);
                v3f cp = cross(pr, otherPr);
                if (mag2(cp) > 0.0001f)
                {
                    continue;
                }
                v2f prTmp = otherEdge.GetPos1() - edge.GetPos1();
                float distTmp = std::abs(dot(norm, prTmp));
                if (distTmp < distMin)
                {
                    distMin = distTmp;
                    edgeNearest = otherEdge;
                }
            }
        }
        if (distMin < 0.05f)
        {
            v2f pr = edgeNearest.GetPos1() - edge.GetPos1();
            float d = dot(norm, pr);
            v2f dp = d * norm;
            m_ptrGraph->MovePickedNode(dp);
            InitScene();
            dx += dp[0];
            dy += dp[1];
            flag = true;
        }
    }
    return flag;
}

void CLevelSynth::InitScene()
{
    m_layout.ClearLayout();
    int numOfRooms = m_ptrGraph->GetNumOfNodes();
    int numOfTemplates = m_ptrTemplates->GetNumOfTemplates();
    for (int i = 0; i < numOfRooms; i++)
    {
        //int idx = int(rand() / float(RAND_MAX) * numOfTemplates);
        int idx = m_ptrGraph->GetNode(i).GetType();
        idx = idx % numOfTemplates;
        CRoom room = m_ptrTemplates->GetRoom(idx);
        //room.ScaleRoom(0.5f);
        v2f pi = m_ptrGraph->GetNodePos(i);
        v2f c = room.GetRoomCenter();
        v2f trans = pi - c;
        room.TranslateRoom(trans);
        v3f color = randomColorFromIndex(i);
        if (mag2(color) > 2.5f)
        {
            color = color * 0.5f;
        }
        room.SetColor(color);
        m_layout.AddRoom(room);
    }
}

CRoomLayout CLevelSynth::GetLayout(CPlanarGraph* ptrGraph, std::vector<v2f>& roomPositions)
{
    CRoomLayout layout;
    int numOfRooms = ptrGraph->GetNumOfNodes();
    int numOfTemplates = m_ptrTemplates->GetNumOfTemplates();
    for (int i = 0; i < numOfRooms; i++)
    {
        int idx = ptrGraph->GetNode(i).GetType();
        idx = idx % numOfTemplates;
        CRoom room = m_ptrTemplates->GetRoom(idx);
        v2f pi = roomPositions[i];
        v2f c = room.GetRoomCenter();
        v2f trans = pi - c;
        room.TranslateRoom(trans);
        v3f color = randomColorFromIndex(i);
        if (mag2(color) > 2.5f)
        {
            color = color * 0.5f;
        }
        room.SetColor(color);
        room.SetFlagFixed(ptrGraph->GetNode(i).GetFlagFixed());
        layout.AddRoom(room);
    }
    return layout;
}

void CLevelSynth::SynthesizeScene()
{
    SynthesizeSceneViaMainLoop();
    UpdateGraphFromLayout();
}

void CLevelSynth::UpdateGraphFromLayout()
{
    int numOfRooms = m_ptrGraph->GetNumOfNodes();
    for (int i = 0; i < numOfRooms; i++)
    {
        v2f roomCenter = m_layout.GetRoom(i).GetRoomCenter();
        m_ptrGraph->GetNode(i).SetPos(roomCenter);
    }
}

bool CLevelSynth::PostProcessing(CRoomLayout& layout, CPlanarGraph* ptrGraph)
{
    for (int i = 0; i < layout.GetNumOfRooms(); i++)
    {
        std::vector<int> neighbors;
        for (int j = 0; j < ptrGraph->GetNumOfEdges(); j++)
        {
            CGraphEdge& edge = ptrGraph->GetEdge(j);
            if (edge.GetIdx0() == i)
            {
                neighbors.push_back(edge.GetIdx1());
            }
            else if (edge.GetIdx1() == i)
            {
                neighbors.push_back(edge.GetIdx0());
            }
        }
        if (neighbors.empty() == true)
        {
            continue;
        }
        CConfigSpace configSpace0(layout.GetRoom(neighbors[0]), layout.GetRoom(i));
        for (int j = 1; j < int(neighbors.size()); j++)
        {
            CConfigSpace configSpace1(layout.GetRoom(neighbors[j]), layout.GetRoom(i));
            configSpace0 = CConfigSpace::FindIntersection(configSpace0, configSpace1);
        }
        configSpace0.SelfMerge();
        std::cout << "Size of configuration space for the " << i << "th room: " << configSpace0.GetConfigSpaceSize() << std::endl;
        configSpace0.PrintConfigSpace();
    }
    return true;
}

bool CLevelSynth::OpenDoors(CRoomLayout& layout, CPlanarGraph* ptrGraph, bool flagPartial /* = false */)
{
    for (int i = 0; i < layout.GetNumOfRooms(); i++)
    {
        layout.GetRoom(i).InitWalls();
    }
    for (int i = 0; i < ptrGraph->GetNumOfEdges(); i++)
    {
        CGraphEdge& ge = ptrGraph->GetEdge(i);
        int roomIdx1 = ge.GetIdx0();
        int roomIdx2 = ge.GetIdx1();
        CRoom& room1 = layout.GetRoom(roomIdx1);
        CRoom& room2 = layout.GetRoom(roomIdx2);
        if (room1.GetFlagFixed() == true || room2.GetFlagFixed() == true)
        {
            continue;
        }
        int edgeIdx1, edgeIdx2;
        float contact = RoomContact(room1, room2, edgeIdx1, edgeIdx2);
        if (contact < CLevelConfig::m_roomContactThresh) // just to double check
        {
            if (flagPartial == false)
            {
                std::cout << "Failed to open the door on the wall between Room " << roomIdx1 << " and Room " << roomIdx2 << " (case 1)!\n";
                return false;
            }
            else
            {
                continue;
            }
        }
        std::vector<v2f> vecPos(4);
        CRoomEdge edge1 = room1.GetEdge(edgeIdx1);
        CRoomEdge edge2 = room2.GetEdge(edgeIdx2);
        vecPos[0] = edge1.GetPos1();
        vecPos[1] = edge1.GetPos2();
        vecPos[2] = edge2.GetPos1();
        vecPos[3] = edge2.GetPos2();
        SortVecPr(vecPos);
        RoomDoor door(vecPos[1], vecPos[2]);
        bool flag1 = OpenDoor(room1, door);
        bool flag2 = OpenDoor(room2, door);
        if (flag1 == false || flag2 == false)
        {
            std::cout << "Failed to open the door on the wall between Room " << roomIdx1 << " and Room " << roomIdx2 << " (case 2)!\n";
            return false;
        }
    }
    return true;
}

bool CLevelSynth::OpenDoor(CRoom& room, RoomDoor& door, float width /* = -1.f */)
{
    const float numericalTolerance = g_numericalTolerance * 100.f;
    const float numericalToleranceSq = numericalTolerance * numericalTolerance;
    const float doorWidth = (width > 0.f) ? width : (CLevelConfig::m_roomContactThresh * 0.8f);
    bool openFlag = false;
    for (int i = 0; i < room.GetNumOfWalls(); i++)
    {
        RoomWall wall = room.GetWall(i);
        float d1 = PointToSegmentSqDistance(door.GetPos1(), wall);
        float d2 = PointToSegmentSqDistance(door.GetPos2(), wall);
        if (d1 > numericalToleranceSq || d2 > numericalToleranceSq)
        {
            continue;
        }
        room.EraseWall(i);
        float d11 = mag2(door.GetPos1() - wall.GetPos1());
        float d12 = mag2(door.GetPos2() - wall.GetPos1());
        v2f p1 = (d11 < d12) ? door.GetPos1() : door.GetPos2();
        v2f p2 = (d11 < d12) ? door.GetPos2() : door.GetPos1();
        v2f pMean = (p1 + p2) * 0.5f;
        v2f pd = p2 - p1;
        pd = normalize(pd) * doorWidth * 0.5f;
        p1 = pMean - pd;
        p2 = pMean + pd;
        RoomWall wall1(wall.GetPos1(), p1);
        RoomWall wall2(p2, wall.GetPos2());
        room.InsertWall(wall1);
        room.InsertWall(wall2);
        openFlag = true;
        break;
    }
    return openFlag;
}

bool CLevelSynth::OpenDoors(CRoomLayout& layout, CRoomLayout& layoutShrinked, CPlanarGraph* ptrGraph, float thrinkDist)
{
    for (int i = 0; i < layout.GetNumOfRooms(); i++)
    {
        layout.GetRoom(i).InitWalls();
        layoutShrinked.GetRoom(i).InitWalls();
    }
    float doorWidth = CLevelConfig::m_roomContactThresh * 0.8f;
    for (int i = 0; i < ptrGraph->GetNumOfEdges(); i++)
    {
        CGraphEdge& ge = ptrGraph->GetEdge(i);
        int roomIdx1 = ge.GetIdx0();
        int roomIdx2 = ge.GetIdx1();
        CRoom& room1 = layout.GetRoom(roomIdx1);
        CRoom& room2 = layout.GetRoom(roomIdx2);
        if (room1.GetFlagFixed() == true || room2.GetFlagFixed() == true)
        {
            continue;
        }
        int edgeIdx1, edgeIdx2;
        float contact = RoomContact(room1, room2, edgeIdx1, edgeIdx2);
        if (contact < doorWidth) // just to double check
        {
            std::cout << "Failed to open the door on the wall between Room " << roomIdx1 << " and Room " << roomIdx2 << " (case 1)!\n";
            return false;
        }
        std::vector<v2f> vecPos(4);
        CRoomEdge edge1 = room1.GetEdge(edgeIdx1);
        CRoomEdge edge2 = room2.GetEdge(edgeIdx2);
        vecPos[0] = edge1.GetPos1();
        vecPos[1] = edge1.GetPos2();
        vecPos[2] = edge2.GetPos1();
        vecPos[3] = edge2.GetPos2();
        SortVecPr(vecPos);
        v2f p1 = vecPos[1];
        v2f p2 = vecPos[2];
        v2f pr = p2 - p1;
        pr = normalize(pr);
        v2f prOrtho = v2f(pr[1], -pr[0]);
        pr = pr * doorWidth * 0.5f;
        v2f pAve = (p1 + p2) * 0.5f;
        p1 = pAve - pr;
        p2 = pAve + pr;
        v2f p3 = p1 + prOrtho * thrinkDist;
        v2f p4 = p1 - prOrtho * thrinkDist;
        v2f p5 = p2 + prOrtho * thrinkDist;
        v2f p6 = p2 - prOrtho * thrinkDist;
        CorridorWall wall1(p3, p4);
        CorridorWall wall2(p5, p6);
        layoutShrinked.InsertCorridorWall(wall1);
        layoutShrinked.InsertCorridorWall(wall2);
        RoomDoor door1(p3, p5);
        RoomDoor door2(p4, p6);
        bool flag1 = OpenDoor(layoutShrinked.GetRoom(roomIdx1), door1, doorWidth) || OpenDoor(layoutShrinked.GetRoom(roomIdx1), door2, doorWidth);
        bool flag2 = OpenDoor(layoutShrinked.GetRoom(roomIdx2), door1, doorWidth) || OpenDoor(layoutShrinked.GetRoom(roomIdx2), door2, doorWidth);
        if (flag1 == false || flag2 == false)
        {
            std::cout << "Failed to open the door on the wall between Room " << roomIdx1 << " and Room " << roomIdx2 << " (case 2)!\n";
            return false;
        }
    }
    return true;
}

void CLevelSynth::ShrinkRooms(CRoomLayout& layout, float dist)
{
    if (dist <= 0.f)
    {
        return;
    }
    for (int i = 0; i < layout.GetNumOfRooms(); i++)
    {
        ShrinkRoom(layout.GetRoom(i), dist);
    }
}

void CLevelSynth::ShrinkRoom(CRoom& room, float dist)
{
    if (dist <= 0.f)
    {
        return;
    }
    const int numOfEdges = room.GetNumOfEdges();
    std::vector<CRoomEdge> vecRoomEdge(numOfEdges);
    for (int i = 0; i < numOfEdges; i++)
    {
        vecRoomEdge[i] = room.GetEdge(i);
    }
    std::vector<CRoomEdge> vecRoomEdge1 = vecRoomEdge;
    std::vector<CRoomEdge> vecRoomEdge2 = vecRoomEdge;
    for (int i = 0; i < numOfEdges; i++)
    {
        v2f pr = vecRoomEdge[i].GetPos2() - vecRoomEdge[i].GetPos1();
        v2f prNew = v2f(pr[1], -pr[0]);
        prNew = normalize(prNew) * dist;
        vecRoomEdge1[i].SetPos1(vecRoomEdge1[i].GetPos1() + prNew);
        vecRoomEdge1[i].SetPos2(vecRoomEdge1[i].GetPos2() + prNew);
        vecRoomEdge2[i].SetPos1(vecRoomEdge2[i].GetPos1() - prNew);
        vecRoomEdge2[i].SetPos2(vecRoomEdge2[i].GetPos2() - prNew);
    }
    std::vector<v2f> vertices1 = room.GetVertices();
    std::vector<v2f> vertices2 = room.GetVertices();
    for (int i = 0; i < numOfEdges; i++)
    {
        int idx1 = i;
        int idx2 = (i + 1) % numOfEdges;
        v2f pi1;
        v2f p111 = vecRoomEdge1[idx1].GetPos1();
        v2f p112 = vecRoomEdge1[idx1].GetPos2();
        v2f p121 = vecRoomEdge1[idx2].GetPos1();
        v2f p122 = vecRoomEdge1[idx2].GetPos2();
        LineIntersection(p111, p112, p121, p122, pi1);
        vertices1[i] = pi1;
        v2f pi2;
        v2f p211 = vecRoomEdge2[idx1].GetPos1();
        v2f p212 = vecRoomEdge2[idx1].GetPos2();
        v2f p221 = vecRoomEdge2[idx2].GetPos1();
        v2f p222 = vecRoomEdge2[idx2].GetPos2();
        LineIntersection(p211, p212, p221, p222, pi2);
        vertices2[i] = pi2;
    }
    CClipperWrapper wrapper;
    CRoom room1;
    room1.SetVertices(vertices1);
    float area1 = wrapper.ComputeRoomArea(room1);
    CRoom room2;
    room2.SetVertices(vertices2);
    float area2 = wrapper.ComputeRoomArea(room2);
    std::vector<v2f> verticesNew = (area1 < area2) ? vertices1 : vertices2;
    room.SetVertices(verticesNew);
}

bool CLevelSynth::SaveGraphAsSVG(const char* fileName, CPlanarGraph* ptrGraph, int wd /* = 400 */, int ht /* = 400 */, float labelRad /* = 0.25f */)
{
    CPlanarGraph graphNew = *ptrGraph;
    graphNew.MoveGraphToSceneCenter();
    int strokeWd = 5;
    int circleRad = 7;
    v2f posMin, posMax;
    graphNew.GetGraphBoundingBox(posMin, posMax);
    float pMin = min(posMin[0], posMin[1]);
    float pMax = max(posMax[0], posMax[1]);
    float scaling = 1.05f;
    pMin *= scaling;
    pMax *= scaling;
    const char* str = "\t<?xml version=\"1.0\" standalone=\"no\" ?>\n"
                      "<!-- graph visualization -->\n"
                      "<svg>\n"
                      "</svg>\n";
    tinyxml2::XMLDocument doc;
    doc.Parse(str);
    tinyxml2::XMLElement* root = doc.RootElement();
    std::ostringstream ossViewBox;
    ossViewBox << 0 << " " << 0 << " " << wd << " " << ht;
    root->SetAttribute("viewBox", ossViewBox.str().c_str());
    root->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
    // Dump edges...
    for (int i = 0; i < graphNew.GetNumOfEdges(); i++)
    {
        tinyxml2::XMLElement* edgeElement = doc.NewElement("path");
        std::ostringstream ossPath;
        CGraphEdge& edge = graphNew.GetEdge(i);
        v2f p1 = graphNew.GetNodePos(edge.GetIdx0());
        v2f p2 = graphNew.GetNodePos(edge.GetIdx1());
        ossPath << "M ";
        ossPath << CRoomLayout::ConvertPosX(p1[0], pMin, pMax, wd) << " ";
        ossPath << CRoomLayout::ConvertPosY(p1[1], pMin, pMax, ht) << " ";
        ossPath << "L ";
        ossPath << CRoomLayout::ConvertPosX(p2[0], pMin, pMax, wd) << " ";
        ossPath << CRoomLayout::ConvertPosY(p2[1], pMin, pMax, ht) << " ";
        edgeElement->SetAttribute("d", ossPath.str().c_str());
        edgeElement->SetAttribute("fill", "none");
        edgeElement->SetAttribute("stroke", "black");
        edgeElement->SetAttribute("stroke-width", strokeWd);
        root->InsertEndChild(edgeElement);
    }
    // Dump nodes...
    for (int i = 0; i < graphNew.GetNumOfNodes(); i++)
    {
        tinyxml2::XMLElement* nodeElement = doc.NewElement("circle");
        v2f pi = graphNew.GetNodePos(i);
        nodeElement->SetAttribute("cx", CRoomLayout::ConvertPosX(pi[0], pMin, pMax, wd));
        nodeElement->SetAttribute("cy", CRoomLayout::ConvertPosY(pi[1], pMin, pMax, ht));
        nodeElement->SetAttribute("r", circleRad);
        nodeElement->SetAttribute("fill", "red");
        nodeElement->SetAttribute("stroke", "none");
        root->InsertEndChild(nodeElement);
    }
    // Dump labels...
    for (int i = 0; i < graphNew.GetNumOfNodes(); i++)
    {
        int shiftX = (i >= 10) ? 8 : 3;
        int shiftY = 5;
        v2f pi = ComputeLabelPosition(i, &graphNew, labelRad);
        tinyxml2::XMLElement* labelElement = doc.NewElement("text");
        labelElement->SetAttribute("x", CRoomLayout::ConvertPosX(pi[0], pMin, pMax, wd) - shiftX);
        labelElement->SetAttribute("y", CRoomLayout::ConvertPosY(pi[1], pMin, pMax, ht) + shiftY);
        labelElement->SetAttribute("font-family", "Verdana");
        labelElement->SetAttribute("font-size", 13);
        labelElement->SetAttribute("fill", "blue");
        std::ostringstream ossLabel;
        ossLabel << i;
        tinyxml2::XMLText* labelText = doc.NewText(ossLabel.str().c_str());
        labelElement->InsertEndChild(labelText);
        root->InsertEndChild(labelElement);
    }

    bool saveFlag = doc.SaveFile(fileName);
    return saveFlag;
}

bool CLevelSynth::CompareStateEnergySmallerFirst(const CurrentState& state1, const CurrentState& state2)
{
    return (state1.m_stateEnergy < state2.m_stateEnergy);
}

void CLevelSynth::SynthesizeSceneViaMainLoop()
{
    CurrentState state0;
    state0.m_stateGraph = *m_ptrGraph;
    state0.m_stateRoomPositions = m_roomPositions;
    state0.m_stateEnergy = 1e10;
    std::stack<CurrentState> stateStack;
    stateStack.push(state0);
    int targetNumOfSolutions = CLevelConfig::m_targetNumOfSolutions;
    float energyMin = 1e10;
    CRoomLayout layoutBest = m_layout;
    int numPartials = 0;
    m_backTrackCount = 0;
    m_backTrackLevel = 0;
    while (m_solutionCount < targetNumOfSolutions && stateStack.empty() == false)
    {
        CurrentState oldState = stateStack.top();
        stateStack.pop();
        SetCurrentState(oldState);
        m_flagVisitedNoNode = m_ptrGraph->VisitedNoNode();
        bool flagCyclic = false;
        std::vector<int> tmpIndices = m_ptrGraph->ExtractDeepestFaceOrChain(flagCyclic, CLevelConfig::m_flagSmallFaceFirst);
        std::vector<int> indices;
#if 0 // Before 09/03/2013
		if ( CLevelConfig::m_synMethod != 0 )
		{
			// Select all the graph nodes...
			indices.resize(m_ptrGraph->GetNumOfNodes());
			for ( int i=0; i<int(indices.size()); i++ )
			{
				indices[i] = i;
			}
		}
#else
        indices = oldState.myIndices;
        if (CLevelConfig::m_synMethod != 0)
        {
            if (m_ptrGraph->HasFixedNode() == false || m_ptrGraph->VisitedNoNode() == false)
            {
                indices = m_ptrGraph->GetUnfixedNodes();
            }
        }
        for (int i = 0; i < tmpIndices.size(); i++)
        {
            indices.push_back(tmpIndices[i]);
        }
#endif
        SetVisitedNeighbors(indices);
        for (int i = 0; i < m_ptrGraph->GetNumOfNodes(); i++)
        {
            m_ptrGraph->GetNode(i).SetFlagVisited(false);
        }

        for (int i = 0; i < int(indices.size()); i++)
        {
            int index = indices[i];
            m_ptrGraph->GetNode(indices[i]).SetFlagVisited(true);
        }
        oldState.m_stateGraph = *m_ptrGraph;
        std::vector<CurrentState> newStates;
        m_chainCount++;
        bool flag = Solve1Dchain(indices, &tmpIndices, oldState, newStates);
        //bool flag = Solve1Dchain(indices, NULL, oldState, newStates);
        if (newStates.empty())
        {
#ifndef PERFORMANCE_TEST
            std::cout << "Backtracked from level " << m_backTrackLevel << " to level " << m_backTrackLevel - 1 << "!" << std::endl;
            ofstream fout;
            fout.open(CLevelConfig::AddOutputPrefix("log.txt").c_str(), std::ios_base::app);
            fout << "Backtracked from level " << m_backTrackLevel << " to level " << m_backTrackLevel - 1 << "!" << std::endl;
#endif
            m_backTrackCount++;
            m_backTrackLevel--;
        }
        else
        {
            m_backTrackLevel++;
        }
        if (m_ptrGraph->VisitedAllNodes() == true)
        {
            for (int i = 0; i < int(newStates.size()); i++)
            {
                if (m_solutionCount >= targetNumOfSolutions)
                {
                    break;
                }
                SetCurrentState(newStates[i]);
                if (newStates[i].m_stateEnergy < energyMin)
                {
                    energyMin = newStates[i].m_stateEnergy;
                    layoutBest = m_layout;
                }
                float collide;
                float connectivity;
                float energy = GetLayoutEnergy(m_layout, m_ptrGraph, collide, connectivity);

                //float CLevelSynth::GetLayoutEnergy(CRoomLayout& layout, CPlanarGraph* ptrGraph, float& collideArea, float& connectivity)

                bool flagValid = (LayoutCollide(m_layout) <= g_numericalTolerance && CheckRoomConnectivity(m_layout, m_ptrGraph) <= g_numericalTolerance);
                if (flagValid == false)
                {
                    // Skip invalid solution...
                    continue;
                }
                DumpSolutionIntoXML();
                m_solutionCount++;
            }
        }
        else
        {
            for (int i = int(newStates.size()) - 1; i >= 0; i--)
            {
                newStates[i].myIndices = indices;
                //newStates[i].m_stateGraph = *m_ptrGraph;
                CPlanarGraph graphBest = newStates[i].m_stateGraph;
                for (int n = 0; n < graphBest.GetNumOfNodes(); n++)
                {
                    v2f pn = newStates[i].m_stateRoomPositions[n];
                    graphBest.GetNode(n).SetPos(pn);
                }

                CRoomLayout layoutBest = GetLayout(&graphBest, newStates[i].m_stateRoomPositions);
                //ofstream fout;
#ifdef DUMP_PARTIAL_SOLUTION
                graphBest.SaveGraphAsXML(CLevelConfig::AddOutputPrefix(sprint("partial_%03d.xml", numPartials)).c_str());
                OpenDoors(layoutBest, &graphBest, true);
                layoutBest.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("partial_%03d.svg", numPartials)).c_str(), 800, 800, true, &graphBest);
#endif
                //m_bestSolCount ++;
                numPartials++;

                stateStack.push(newStates[i]);
            }
        }
    }
    m_layout = layoutBest;
    m_layout.MoveToSceneCenter();

#ifndef PERFORMANCE_TEST
    {
        std::cout << "Total # of backtracks: " << m_backTrackCount << std::endl;
        ofstream fout;
        fout.open(CLevelConfig::AddOutputPrefix("log.txt").c_str(), std::ios_base::app);
        fout << "Total # of backtracks: " << m_backTrackCount << std::endl;
    }
#endif
}

bool CLevelSynth::Solve1Dchain(std::vector<int>& indices, std::vector<int>* weightedIndices, CurrentState& oldState, std::vector<CurrentState>& newStates)
{
    if (CLevelConfig::m_flagUseILS == true)
    {
        return Solve1DchainILS(indices, oldState, newStates);
    }
    CPlanarGraph* ptrGraph = &(oldState.m_stateGraph);
    SetSequenceAs1Dchain(indices, ptrGraph);
    newStates.clear();
    if (ptrGraph->GetNode(indices[0]).GetFlagFixed() == true)
    {
        oldState.InsertToNewStates(newStates, ptrGraph);
        return true;
    }
    bool flagLastChain = ptrGraph->VisitedAllNodes();

    // Number of cycles
    int n = CLevelConfig::m_saNumOfCycles;
    // Number of trials per cycle
    int m = CLevelConfig::m_saNumOfTrials;
    // Number of accepted solutions
    int na = 1;
    // Probability of accepting worse solution at the start
    float p1 = CLevelConfig::m_saProb1;
    // Probability of accepting worse solution at the end
    float p0 = CLevelConfig::m_saProb0;
    // Initial temperature
    float t1 = -1.f / log(p1);
    // Final temperature
    float t0 = -1.f / log(p0);
    // Fractional reduction every cycle
    float frac = pow(t0 / t1, 1.0f / float(n - 1.0f));
    // Current temperature
    float t = t1;
    // DeltaE Average
    float DeltaE_avg = 0.0;
    // Current best result so far
    CRoomLayout layoutBest = GetLayout(ptrGraph, oldState.m_stateRoomPositions);
    CPlanarGraph graphBest = *ptrGraph;

    if (!CLevelConfig::m_flagRandomWalk)
    {
#if 0
		for (int i = 0; i < weightedIndices->size(); i++)
		{
			CRoom& pickedRoom = layoutBest.GetRoom((*weightedIndices)[i]);
			SampleConfigSpaceForPickedRoom(layoutBest, ptrGraph, indices, (*weightedIndices)[i]);
		}
#else // New on 09/24/2013: connect the rooms together as initial guess
        int numOfVisitedNodes = int(indices.size() - weightedIndices->size());
        if (numOfVisitedNodes == 0) // The first chain...
        {
            numOfVisitedNodes = 1;
        }
        std::vector<int> indicesVisited(numOfVisitedNodes);
        std::vector<bool> flagsVisited(ptrGraph->GetNumOfNodes(), false);
        for (int i = 0; i < numOfVisitedNodes; i++)
        {
            indicesVisited[i] = indices[i];
            flagsVisited[indices[i]] = true;
        }
        while (indicesVisited.size() < indices.size())
        {
            int idxUnvisited = -1;
            int idxVisited = -1;
            for (int i = 0; i < int(weightedIndices->size()); i++)
            {
                int idx = (*weightedIndices)[i];
                if (flagsVisited[idx] == true)
                {
                    continue;
                }
                std::vector<int> connectedIndices = GetConnectedIndices(ptrGraph, idx, false);
                std::vector<int> visitedNeighbors;
                for (int j = 0; j < int(connectedIndices.size()); j++)
                {
                    int idxOther = connectedIndices[j];
                    if (flagsVisited[idxOther] == true)
                    {
                        visitedNeighbors.push_back(idxOther);
                    }
                }
                if (visitedNeighbors.empty() == false)
                {
                    idxUnvisited = idx;
                    CRoom& roomUnvisited = layoutBest.GetRoom(idxUnvisited);
                    random_shuffle(visitedNeighbors.begin(), visitedNeighbors.end());
                    CConfigSpace configSpace(layoutBest.GetRoom(visitedNeighbors[0]), roomUnvisited);
                    for (int j = 1; j < int(visitedNeighbors.size()); j++)
                    {
                        CConfigSpace configSpaceTmp(layoutBest.GetRoom(visitedNeighbors[j]), roomUnvisited);
                        CConfigSpace configSpaceNew = CConfigSpace::FindIntersection(configSpace, configSpaceTmp);
                        if (configSpaceNew.IsEmpty() == true)
                        {
                            break;
                        }
                        else
                        {
                            configSpace = configSpaceNew;
                        }
                    }
                    indicesVisited.push_back(idxUnvisited);
                    flagsVisited[idxUnvisited] = true;
    #if 1 // Smartly sample the configuration space based on the energy...
                    std::vector<v2f> vecPos = configSpace.SmartlySampleConfigSpace();
                    int idxBest = -1;
                    float energyMin = 1e10;
                    CPlanarGraph graphTmp = *ptrGraph;
                    for (int j = 0; j < graphTmp.GetNumOfNodes(); j++)
                    {
                        graphTmp.GetNode(j).SetFlagVisited(flagsVisited[j]);
                    }
                    for (int j = 0; j < int(vecPos.size()); j++)
                    {
                        CRoomLayout layoutTmp = layoutBest;
                        CRoom& pickedRoomTmp = layoutTmp.GetRoom(idxUnvisited);
                        v2f dp = vecPos[j] - pickedRoomTmp.GetRoomCenter();
                        pickedRoomTmp.TranslateRoom(dp);
                        float collideArea = 0.f;
                        float connectivity = 0.f;
                        float energyTmp = GetLayoutEnergy(layoutTmp, &graphTmp, collideArea, connectivity);
                        if (collideArea < energyMin)
                        {
                            energyMin = collideArea;
                            idxBest = j;
                        }
                    }
                    if (idxBest >= 0)
                    {
                        v2f dp = vecPos[idxBest] - roomUnvisited.GetRoomCenter();
                        roomUnvisited.TranslateRoom(dp);
                    }
    #else
                    v2f pos = configSpace.RandomlySampleConfigSpace();
                    v2f dp = pos - roomUnvisited.GetRoomCenter();
                    roomUnvisited.TranslateRoom(dp);
    #endif
                    break;
                }
            }
        }
#endif
    }

    m_layout = layoutBest;
#ifdef DUMP_INTERMEDIATE_OUTPUT
    layoutBest.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("chainInit_%03d.svg", m_chainCount)).c_str());
#endif
    float collideArea = 0.f;
    float connectivity = 0.f;
    float energyMin = GetLayoutEnergy(layoutBest, ptrGraph, collideArea, connectivity, -1, true, &indices);
    float energyCurrent = energyMin;
#ifdef DUMP_INTERMEDIATE_OUTPUT
    std::cout << "Initial energy: " << energyMin << std::endl;
    std::ofstream fout;
    fout.open(CLevelConfig::AddOutputPrefix("log.txt").c_str(), std::ios_base::app);
    fout << m_bestSolCount << "\t" << energyMin << std::endl;
    for (int n = 0; n < ptrGraph->GetNumOfNodes(); n++)
    {
        v2f pn = layoutBest.GetRoomPositions()[n];
        ptrGraph->GetNode(n).SetPos(pn);
    }
    ptrGraph->SaveGraphAsXML(CLevelConfig::AddOutputPrefix(sprint("tmpBest_%03d.xml", m_bestSolCount)).c_str());
    OpenDoors(layoutBest, ptrGraph, true);
    layoutBest.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("tmpBest_%03d.svg", m_bestSolCount)).c_str());
    m_bestSolCount++;
#endif
    int pickIndexCount = 0;
    int numFailures = 0;
    for (int i = 0; i < n; i++)
    {
#ifndef PERFORMANCE_TEST
        std::cout << "Cycle " << i + 1 << "/" << n << " (failures " << numFailures << ") ...\n";
#endif
        bool flagWasAccepted = false;
        if (numFailures > 10)
        {
            if (Random2(2) == 0)
            {
#ifndef PERFORMANCE_TEST
                std::cout << "RANDOM RESTART CALLED! 11+ failures" << std::endl;
#endif
                break;
            }
        }
        else if (numFailures > 8)
        {
            if (Random2(3) == 0)
            {
#ifndef PERFORMANCE_TEST
                std::cout << "RANDOM RESTART CALLED! 9+ failures" << std::endl;
#endif
                break;
            }
        }
        else if (numFailures > 5)
        {
            if (Random2(4) == 0)
            {
#ifndef PERFORMANCE_TEST
                std::cout << "RANDOM RESTART CALLED! 6+ failures" << std::endl;
#endif
                break;
            }
        }
        else if (numFailures > 3)
        {
            if (Random2(6) == 0)
            {
#ifndef PERFORMANCE_TEST
                std::cout << "RANDOM RESTART CALLED! 4+ failures" << std::endl;
#endif
                break;
            }
        }
        else if (numFailures > 0)
        {
            if (Random2(8) == 0)
            {
#ifndef PERFORMANCE_TEST
                std::cout << "RANDOM RESTART CALLED: 1 failure but we just felt like quittin'..." << std::endl;
#endif
                break;
            }
        }

        for (int j = 0; j < m; j++)
        {
            CPlanarGraph graphTmp = *ptrGraph;
            CRoomLayout layoutTmp = m_layout;
            int adjustedIndex = RandomlyAdjustOneRoom(layoutTmp, &graphTmp, indices, weightedIndices);
#if 0 // New on 08/16/2013
			if ( m_flagVisitedNoNode == true )
			{
				v2f pMin(1e10);
				v2f pMax(-1e10);
				for ( int d=0; d<int(indices.size()); d++ )
				{
					int idx = indices[d];
					v2f pj = layoutTmp.GetRoom(idx).GetRoomCenter();
					for ( int k=0; k<2; k++ )
					{
						pMin[k] = min(pMin[k], pj[k]);
						pMax[k] = max(pMax[k], pj[k]);
					}
				}
				v2f posCen = (pMin + pMax) * 0.5f;
				for ( int d=0; d<int(indices.size()); d++ )
				{
					int idx = indices[d];
					layoutTmp.GetRoom(idx).TranslateRoom(-posCen);
				}
			}
#endif
            float energyTmp = GetLayoutEnergy(layoutTmp, &graphTmp, collideArea, connectivity, adjustedIndex, true, &indices);

            // bool accept = GetLayoutEnergyEarlyOut(layoutTmp, &graphTmp, collideArea, connectivity, adjustedIndex, &energyTmp, energyCurrent );

            if ((CLevelConfig::m_flagRandomWalk && collideArea <= 1e-3 && connectivity <= 1e-3) ||
                (collideArea <= 1e-4 && connectivity <= 1e-4))
            {
                CurrentState newState = oldState;
                newState.m_stateGraph = graphTmp;
                newState.m_stateRoomPositions = layoutTmp.GetRoomPositions();
                newState.m_stateEnergy = energyTmp;
                newState.MoveRoomsToSceneCenter(&graphTmp);
                newState.InsertToNewStates(newStates, &graphTmp);
                if (newStates.size() >= CLevelConfig::m_numOfSolutionsToTrack)
                {
                    return true;
                }
                if (flagLastChain == true && (m_solutionCount + int(newStates.size()) >= CLevelConfig::m_targetNumOfSolutions))
                {
                    return true;
                }
                //newStates.push_back(newState);
            }
            bool flagAccept = false;

            if (energyTmp < energyCurrent)
            {
                // Energy is lower, automatically accept
                flagAccept = true;
                if (energyTmp < energyMin)
                {
                    energyMin = energyTmp;
#ifndef PERFORMANCE_TEST
                    std::cout << "A new minimum energy: " << energyMin << std::endl;
#endif
#ifdef DUMP_INTERMEDIATE_OUTPUT
                    layoutBest = layoutTmp;
                    graphBest = graphTmp;
                    for (int n = 0; n < graphBest.GetNumOfNodes(); n++)
                    {
                        v2f pn = layoutBest.GetRoomPositions()[n];
                        graphBest.GetNode(n).SetPos(pn);
                    }

                    std::ofstream fout;
                    fout.open(CLevelConfig::AddOutputPrefix("log.txt").c_str(), std::ios_base::app);
                    fout << m_bestSolCount << "\t" << energyMin << std::endl;
                    graphBest.SaveGraphAsXML(CLevelConfig::AddOutputPrefix(sprint("tmpBest_%03d.xml", m_bestSolCount)).c_str());
                    OpenDoors(layoutBest, &graphBest, true);
                    layoutBest.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("tmpBest_%03d.svg", m_bestSolCount)).c_str());
                    m_bestSolCount++;
#endif
                }
            }
            else
            {
                float DeltaE = std::abs(energyTmp - energyCurrent);

                // Energy is higher...
                if (i == 0 && j == 0)
                {
                    DeltaE_avg = DeltaE;
                    DeltaE_avg *= CLevelConfig::m_deltaEscaling;
                }
                // Generate probability of acceptance...
                float prob = exp(-(energyTmp - energyCurrent) / (DeltaE_avg * t));
                float r = rand() / float(RAND_MAX);
                if (r < prob)
                {
                    flagAccept = true;
                }
                else
                {
                    flagAccept = false;
                }
            }
            if (flagAccept == true)
            {
                float DeltaE = std::abs(energyTmp - energyCurrent);

                m_layout = layoutTmp;
                *ptrGraph = graphTmp;
                energyCurrent = energyTmp;
                if (DeltaE != 0.0)
                {
                    na++;
                    DeltaE_avg = (DeltaE_avg * (na - 1.0f) + DeltaE) / float(na);
                }
                flagWasAccepted = true;
            }

            pickIndexCount++;
            pickIndexCount = pickIndexCount % int(indices.size());
        }
        if (flagWasAccepted == false)
        {
            numFailures++;
        }
        // Lower the temperature for next cycle
        t *= frac;
    }
#ifndef PERFORMANCE_TEST
    std::cout << "Final energy: " << energyMin << std::endl;
#endif
    if (newStates.empty() == true)
    {
#ifdef DUMP_INTERMEDIATE_OUTPUT
        std::cout << "Empty solution set!\n";
        graphBest.SaveGraphAsXML(CLevelConfig::AddOutputPrefix(sprint("backTracking_level%02d_%03d.xml", m_backTrackLevel, m_backTrackCount)).c_str());
        for (int n = 0; n < graphBest.GetNumOfNodes(); n++)
        {
            graphBest.GetNode(n).SetFlagVisited(false);
        }
        for (int n = 0; n < int(indices.size()); n++)
        {
            graphBest.GetNode(indices[n]).SetFlagVisited(true);
        }
        for (int n = 0; n < int(weightedIndices->size()); n++)
        {
            graphBest.GetNode((*weightedIndices)[n]).SetFlagVisited(false);
        }
        layoutBest.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("backTracking_level%02d_%03d.svg", m_backTrackLevel, m_backTrackCount)).c_str());
        layoutBest.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("backTrackingPartial_level%02d_%03d.svg", m_backTrackLevel, m_backTrackCount)).c_str(), 400, 400, true, &graphBest);
#endif
        return false;
    }
#ifndef PERFORMANCE_TEST
    std::cout << "Number of valid states: " << newStates.size() << std::endl;
#endif
    sort(newStates.begin(), newStates.end(), CompareStateEnergySmallerFirst);
    int numOfSolutionsToTrack = min(int(newStates.size()), CLevelConfig::m_numOfSolutionsToTrack);
    std::vector<CurrentState> newerStates(numOfSolutionsToTrack);
    for (int i = 0; i < numOfSolutionsToTrack; i++)
    {
        newerStates[i] = newStates[i];
    }
    newStates = newerStates;

    return true;
}

bool CLevelSynth::Solve1DchainILS(std::vector<int>& indices, CurrentState& oldState, std::vector<CurrentState>& newStates)
{
    CPlanarGraph* ptrGraph = &(oldState.m_stateGraph);
    SetSequenceAs1Dchain(indices, ptrGraph);
    newStates.clear();
    if (ptrGraph->GetNode(indices[0]).GetFlagFixed() == true)
    {
        oldState.InsertToNewStates(newStates, ptrGraph);
        return true;
    }

    // Borrow the parameters from simulated annealing...
    const int n = CLevelConfig::m_saNumOfCycles;
    const int m = CLevelConfig::m_saNumOfTrials;
    // CRoomLayout layoutBest; // Current best result so far
    float collideArea = 0.f;
    float connectivity = 0.f;
    float energyMin = 1e10;
    float energyHistory = 1e10;
    int pickIndexCount = 0;
    for (int i = 0; i < n; i++)
    {
        CRoomLayout layoutHistory = m_layout;
        CPlanarGraph graphHistory = *ptrGraph;
        if (i != 0)
        {
            // Introduce perturbation...
            RandomlyAdjustOneRoom(m_layout, ptrGraph, indices, NULL);
        }
        float energyTmp = GetLayoutEnergy(m_layout, ptrGraph, collideArea, connectivity);
        float energyCurrent = energyTmp;
        if (i == 0)
        {
            energyMin = energyCurrent;
            std::cout << "Initial energy: " << energyCurrent << std::endl;
        }
        for (int j = 0; j < m; j++)
        {
            CPlanarGraph graphTmp = *ptrGraph;
            CRoomLayout layoutTmp = m_layout;
            RandomlyAdjustOneRoom(layoutTmp, &graphTmp, indices, NULL);
#if 1 // New on 08/16/2013
            if (m_flagVisitedNoNode == true)
            {
                v2f pMin(1e10);
                v2f pMax(-1e10);
                for (int d = 0; d < int(indices.size()); d++)
                {
                    int idx = indices[d];
                    v2f pj = layoutTmp.GetRoom(idx).GetRoomCenter();
                    for (int k = 0; k < 2; k++)
                    {
                        pMin[k] = min(pMin[k], pj[k]);
                        pMax[k] = max(pMax[k], pj[k]);
                    }
                }
                v2f posCen = (pMin + pMax) * 0.5f;
                for (int d = 0; d < int(indices.size()); d++)
                {
                    int idx = indices[d];
                    layoutTmp.GetRoom(idx).TranslateRoom(-posCen);
                }
            }
#endif
            float energyTmp = GetLayoutEnergy(layoutTmp, &graphTmp, collideArea, connectivity);
            if (collideArea <= g_numericalTolerance && connectivity <= g_numericalTolerance)
            {
                CurrentState newState = oldState;
                newState.m_stateGraph = *ptrGraph;
                newState.m_stateRoomPositions = layoutTmp.GetRoomPositions();
                newState.m_stateEnergy = energyTmp;
                newState.MoveRoomsToSceneCenter(ptrGraph);
                newState.InsertToNewStates(newStates, ptrGraph);
            }
            if (energyTmp < energyCurrent)
            {
                if (energyTmp < energyMin)
                {
                    // layoutBest = layoutTmp;
                    energyMin = energyTmp;
#ifndef PERFORMANCE_TEST
                    std::cout << "A new minimum energy: " << energyMin << std::endl;
#endif
                }
                m_layout = layoutTmp;
                *ptrGraph = graphTmp;
                energyCurrent = energyTmp;
            }
            pickIndexCount++;
            pickIndexCount = pickIndexCount % int(indices.size());
        }
        if (i == 0 || energyMin < energyHistory)
        {
            energyHistory = energyMin;
        }
        else
        {
            m_layout = layoutHistory;
            *ptrGraph = graphHistory;
        }
    }
    std::cout << "Final energy: " << energyMin << std::endl;
    if (newStates.empty() == true)
    {
        std::cout << "Empty solution set!\n";
        return false;
    }
    std::cout << "Number of valid states: " << newStates.size() << std::endl;
    sort(newStates.begin(), newStates.end(), CompareStateEnergySmallerFirst);
    int numOfSolutionsToTrack = min(int(newStates.size()), CLevelConfig::m_numOfSolutionsToTrack);
    std::vector<CurrentState> newerStates;
    for (int i = 0; i < numOfSolutionsToTrack; i++)
    {
        newerStates.push_back(newStates[i]);
    }
    newStates = newerStates;

    return true;
}

void CLevelSynth::SetCurrentState(CurrentState& s)
{
    *m_ptrGraph = s.m_stateGraph;
    m_roomPositions = s.m_stateRoomPositions;
    m_layout = GetLayout(m_ptrGraph, m_roomPositions);
}

void CLevelSynth::SetSequenceAs1Dchain(const std::vector<int>& indices, CPlanarGraph* ptrGraph)
{
    m_sequence.clear();
    for (int i = 0; i < int(indices.size()); i++)
    {
        int idx = ptrGraph->GetNode(indices[i]).GetType();
        idx = idx % m_ptrTemplates->GetNumOfTemplates();
        m_sequence.push_back(idx);
    }
}

void CLevelSynth::SetVisitedNeighbors(const std::vector<int>& indices)
{
    m_visitedNeighbors.clear();
    m_visitedNeighbors.resize(indices.size());
    for (int i = 0; i < int(m_visitedNeighbors.size()); i++)
    {
        int nodeIdx = indices[i];
        std::vector<int>& neighbors = m_ptrGraph->GetNode(nodeIdx).GetNeighbors();
        for (int j = 0; j < int(neighbors.size()); j++)
        {
            int neighborIdx = neighbors[j];
            if (m_ptrGraph->GetNode(neighborIdx).GetFlagVisited() == true)
            {
                m_visitedNeighbors[i].push_back(neighborIdx);
            }
        }
    }
}

void CLevelSynth::DumpSolutionIntoXML()
{
    CPlanarGraph graphSol = *m_ptrGraph;
    for (int i = 0; i < graphSol.GetNumOfNodes(); i++)
    {
        v2f pi = m_roomPositions[i];
        graphSol.GetNode(i).SetPos(pi[0], pi[1]);
    }
    graphSol.SaveGraphAsXML(CLevelConfig::AddOutputPrefix(sprint("dbg_%03d.xml", m_solutionCount)).c_str());
    CRoomLayout layoutSol = GetLayout(m_ptrGraph, m_roomPositions);
    OpenDoors(layoutSol, m_ptrGraph);
    layoutSol.SaveLayoutAsSVG(CLevelConfig::AddOutputPrefix(sprint("dbg_%03d.svg", m_solutionCount)).c_str());
}

int CLevelSynth::RandomlyPickOneRoom(CRoomLayout& layout)
{
    int numOfRooms = layout.GetNumOfRooms();
    int pickedRoomIndex = int(rand() / float(RAND_MAX) * numOfRooms);
    pickedRoomIndex = pickedRoomIndex % numOfRooms;
    return pickedRoomIndex;
}

int CLevelSynth::RandomlyPickOneRoom(CRoomLayout& layout, std::vector<int>& indices, std::vector<int>* weightedIndices)
{
    if (weightedIndices)
    {
        std::vector<int> tmpIndices = *weightedIndices;
        int chainLength = int(tmpIndices.size());
        for (int i = 0; i < indices.size(); i++)
        {
            float energyTmp = layout.GetRoom(indices[i]).GetEnergy();
            if (energyTmp > 1.1)
            {
                tmpIndices.push_back(indices[i]);
            }
        }

        int pickedRoomIndex = int(rand() / float(RAND_MAX) * chainLength);
        pickedRoomIndex = pickedRoomIndex % chainLength;
        pickedRoomIndex = (tmpIndices)[pickedRoomIndex];
        return pickedRoomIndex;
    }
    else
    {
        int chainLength = int(indices.size());
        int pickedRoomIndex = int(rand() / float(RAND_MAX) * chainLength);
        pickedRoomIndex = pickedRoomIndex % chainLength;
        pickedRoomIndex = indices[pickedRoomIndex];
        return pickedRoomIndex;
    }
}

int CLevelSynth::RandomlyPickAnotherRoom(CRoomLayout& layout, int pickedIndex)
{
    int numOfRooms = layout.GetNumOfRooms();
    int otherRoomIndex = pickedIndex;
    while (otherRoomIndex == pickedIndex)
    {
        otherRoomIndex = int(rand() / float(RAND_MAX) * numOfRooms);
        otherRoomIndex = otherRoomIndex % numOfRooms;
    }
    return otherRoomIndex;
}

std::vector<int> CLevelSynth::GetConnectedIndices(CPlanarGraph* ptrGraph, int pickedIndex, bool flagVisitedOnly /* = true */)
{
    std::vector<int> indices;
    for (int i = 0; i < ptrGraph->GetNumOfEdges(); i++)
    {
        CGraphEdge& edge = ptrGraph->GetEdge(i);
        int idx0 = edge.GetIdx0();
        int idx1 = edge.GetIdx1();
        if (idx0 != pickedIndex && idx1 != pickedIndex)
        {
            continue;
        }
        int idx = (idx0 == pickedIndex) ? idx1 : idx0;
        if (ptrGraph->GetNode(idx).GetFlagVisited() == false && flagVisitedOnly == true)
        {
            continue;
        }
        indices.push_back(idx);
    }
    return indices;
}

int CLevelSynth::RandomlyAdjustOneRoom(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices, std::vector<int>* weightedIndices)
{
    int numOfRooms = layout.GetNumOfRooms();
    if (numOfRooms <= 1)
    {
        return -1;
    }

    float r = rand() / float(RAND_MAX);

#if 0 // Before 07/16/2013
	if ( r < 0.25f )
	{
		RandomlyAdjustOneRoom01(layout, ptrGraph, indices);
	}
	else if ( r < 0.5f )
	{
		RandomlyAdjustOneRoom02(layout, ptrGraph, indices);
	}
	else if ( r < 0.75f || CLevelConfig::m_flagEnableTypeChange == false )
#else
    if (r < 0.75f || CLevelConfig::m_flagEnableTypeChange == false) // nv: was 0.9
#endif
    {
        if (CLevelConfig::m_flagRandomWalk == false)
        {
            return RandomlyAdjustOneRoom03(layout, ptrGraph, indices, weightedIndices);
        }
        else
        {
            return GradientDescentOneRoom(layout, ptrGraph, *weightedIndices);
        }
    }
    else
    {
        return RandomlyAdjustOneRoom04(layout, ptrGraph, indices, weightedIndices);
    }
}

void CLevelSynth::RandomlyAdjustOneRoom01(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices)
{
    int pickedRoomIndex = RandomlyPickOneRoom(layout, indices, NULL);
    CRoom& pickedRoom = layout.GetRoom(pickedRoomIndex);

    for (int i = 0; i < pickedRoom.GetNumOfEdges(); i++)
    {
        CRoomEdge edge = pickedRoom.GetEdge(i);
        v2f pr2 = edge.GetPos2() - edge.GetPos1();
        v3f pr = v3f(pr2[0], pr2[1], 0.f);
        v2f norm = v2f(pr[1], -pr[0]);
        norm = normalize(norm);
        float distMin = 1e10;
        CRoomEdge edgeNearest;
        for (int j = 0; j < layout.GetNumOfRooms(); j++)
        {
            if (j == pickedRoomIndex)
            {
                continue;
            }
            if (ptrGraph->GetNode(j).GetFlagVisited() == false) continue;
            CRoom& otherRoom = layout.GetRoom(j);
            for (int k = 0; k < otherRoom.GetNumOfEdges(); k++)
            {
                CRoomEdge otherEdge = otherRoom.GetEdge(k);
                v2f otherPr2 = otherEdge.GetPos2() - otherEdge.GetPos1();
                v3f otherPr = v3f(otherPr2[0], otherPr2[1], 0.f);
                v3f cp = cross(pr, otherPr);
                if (mag2(cp) > 0.0001f)
                {
                    continue;
                }
                v2f prTmp = otherEdge.GetPos1() - edge.GetPos1();
                float distTmp = std::abs(dot(norm, prTmp));
                if (distTmp < distMin)
                {
                    distMin = distTmp;
                    edgeNearest = otherEdge;
                }
            }
        }
        if (distMin < 0.3f)
        {
            v2f pr = edgeNearest.GetPos1() - edge.GetPos1();
            float d = dot(norm, pr);
            v2f dp = d * norm;
            pickedRoom.TranslateRoom(dp);
        }
    }
}

void CLevelSynth::RandomlyAdjustOneRoom02(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices)
{
    int pickedRoomIndex = RandomlyPickOneRoom(layout, indices, NULL);
    CRoom& pickedRoom = layout.GetRoom(pickedRoomIndex);

    int numOfEdges = pickedRoom.GetNumOfEdges();
    int pickedEdgeIndex = int(rand() / float(RAND_MAX) * numOfEdges);
    pickedEdgeIndex = pickedEdgeIndex % numOfEdges;
    CRoomEdge edge = pickedRoom.GetEdge(pickedEdgeIndex);
    v2f pr2 = edge.GetPos2() - edge.GetPos1();
    v3f pr = v3f(pr2[0], pr2[1], 0.f);
    v2f norm = v2f(pr[1], -pr[0]);
    norm = normalize(norm);

    int otherRoomIndex = RandomlyPickAnotherRoom(layout, pickedRoomIndex);
    CRoom& otherRoom = layout.GetRoom(otherRoomIndex);
    float distMin = 1e10;
    CRoomEdge edgeNearest;

    for (int k = 0; k < otherRoom.GetNumOfEdges(); k++)
    {
        CRoomEdge otherEdge = otherRoom.GetEdge(k);
        v2f otherPr2 = otherEdge.GetPos2() - otherEdge.GetPos1();
        v3f otherPr = v3f(otherPr2[0], otherPr2[1], 0.f);
        v3f cp = cross(pr, otherPr);
        if (mag2(cp) > 0.0001f)
        {
            continue;
        }
        v2f prTmp = otherEdge.GetPos1() - edge.GetPos1();
        float distTmp = std::abs(dot(norm, prTmp));
        if (distTmp < distMin)
        {
            distMin = distTmp;
            edgeNearest = otherEdge;
        }
    }
    if (distMin < 0.3f)
    {
        v2f pr = edgeNearest.GetPos1() - edge.GetPos1();
        float d = dot(norm, pr);
        v2f dp = d * norm;
        pickedRoom.TranslateRoom(dp);
    }
}

int CLevelSynth::GradientDescentOneRoom(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices)
{
    float collideArea = 0.f;
    float connectivity = 0.f;
    float myEnergy = GetLayoutEnergy(layout, ptrGraph, collideArea, connectivity);
    int pickedRoomIndex = RandomlyPickOneRoom(layout, indices, NULL);

    int besti = -1;
    float bestEnergy = 10e6;
    std::vector<int> candidateAngles;

    for (int i = 0; i < 360; i += 10)
    {
        float angle = i * atan(1.f) * 4.f / 180.0f;
        CRoomLayout l = layout;
        CRoom& pickedRoom = l.GetRoom(pickedRoomIndex);

        const float one_step_length = 0.1f;
        v2f dp = v2f(cosf(angle) * one_step_length, sinf(angle) * one_step_length);

        pickedRoom.TranslateRoom(dp);
        float collideArea = 0.f;
        float connectivity = 0.f;
        float energyMin = GetLayoutEnergy(l, ptrGraph, collideArea, connectivity);
        if (energyMin < bestEnergy || besti == -1)
        {
            bestEnergy = energyMin;
            besti = i;
        }
        if (energyMin < myEnergy)
        {
            candidateAngles.push_back(i);
        }
    }

    //	float angle = besti * M_PI / 180.0f;
    float angle;

    if (candidateAngles.empty())
    {
        // Well! I'm stuck on a local minimum.
        angle = Random2(36) * 10 * atan(1.f) * 4.f / 180.0f;
    }
    else
    {
        angle = candidateAngles[Random2(candidateAngles.size())] * atan(1.f) * 4.f / 180.0f;
    }

    float stepSize = 0.1f;
    float bestStep = -1;
    bestEnergy = 10e6;

    for (int iters = 0; iters < 4; iters++)
    {
        for (float stepLength = stepSize; stepLength <= 2; stepLength += stepSize)
        {
            CRoomLayout l = layout;
            CRoom& pickedRoom = l.GetRoom(pickedRoomIndex);

            v2f dp = v2f(cosf(angle) * stepLength, sinf(angle) * stepLength);

            pickedRoom.TranslateRoom(dp);
            float collideArea = 0.f;
            float connectivity = 0.f;
            float energyMin = GetLayoutEnergy(l, ptrGraph, collideArea, connectivity);
            if (energyMin < bestEnergy || bestStep == -1)
            {
                bestEnergy = energyMin;
                bestStep = stepLength;
            }
        }
        if (myEnergy - bestEnergy < 0.00001)
        {
            stepSize /= 2.0f;
        }
    }

    v2f dp = v2f(cosf(angle) * bestStep, sinf(angle) * bestStep);

    // do the actual translation on the room itself
    CRoom& pickedRoom = layout.GetRoom(pickedRoomIndex);
    pickedRoom.TranslateRoom(dp);
    return pickedRoomIndex;
}

int CLevelSynth::RandomlyAdjustOneRoom03(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices, std::vector<int>* weightedIndices)
{
    int pickedRoomIndex = RandomlyPickOneRoom(layout, indices, weightedIndices);
    CRoom& pickedRoom = layout.GetRoom(pickedRoomIndex);

    SampleConfigSpaceForPickedRoom(layout, ptrGraph, indices, pickedRoomIndex);
    return pickedRoomIndex;
}

void CLevelSynth::SampleConfigSpaceForPickedRoom(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices, int pickedRoomIndex)
{
    CRoom& pickedRoom = layout.GetRoom(pickedRoomIndex);
    CConfigSpace configSpace;
    std::vector<int> connectedIndices = GetConnectedIndices(ptrGraph, pickedRoomIndex);
    if (connectedIndices.size() >= 1)
    {
        random_shuffle(connectedIndices.begin(), connectedIndices.end());
        int idx0 = connectedIndices[0];
        CConfigSpace configSpace0(layout.GetRoom(idx0), pickedRoom);
        configSpace = configSpace0;
        for (int i = 1; i < int(connectedIndices.size()); i++)
        {
            CConfigSpace configSpaceTmp(layout.GetRoom(connectedIndices[i]), pickedRoom);
            CConfigSpace configSpaceNew = CConfigSpace::FindIntersection(configSpace, configSpaceTmp);
            if (configSpaceNew.IsEmpty() == true)
            {
                break;
            }
            else
            {
                configSpace = configSpaceNew;
            }
        }
    }
    int whileCnt = 0;
    while (configSpace.IsEmpty() == true)
    {
        int otherRoomIndex = RandomlyPickAnotherRoom(layout, pickedRoomIndex);
        CRoom& otherRoom = layout.GetRoom(otherRoomIndex);
        configSpace = CConfigSpace(otherRoom, pickedRoom);
        whileCnt++;
        if (whileCnt >= 1000)
        {
            std::cout << "Break from the while loop after reaching enough number of trials!\n";
            return;
        }
    }
    v2f pos = configSpace.RandomlySampleConfigSpace();
    v2f dp = pos - pickedRoom.GetRoomCenter();
    pickedRoom.TranslateRoom(dp);
}

int CLevelSynth::RandomlyAdjustOneRoom04(CRoomLayout& layout, CPlanarGraph* ptrGraph, std::vector<int>& indices, std::vector<int>* weightedIndices)
{
    int numOfTemplates = m_ptrTemplates->GetNumOfTemplates();
    if (numOfTemplates <= 1)
    {
        return -1;
    }

    int pickedRoomIndex = RandomlyPickOneRoom(layout, indices, weightedIndices);
    CRoom& pickedRoom = layout.GetRoom(pickedRoomIndex);

    int typeOld = ptrGraph->GetNode(pickedRoomIndex).GetType();
    int typeNew = typeOld;
    int boundaryOld = ptrGraph->GetNode(pickedRoomIndex).GetBoundaryType();
    int boundaryNew = -1;
    int whileCnt = 0;
    while (typeNew == typeOld || boundaryNew != boundaryOld || m_ptrTemplates->GetRoom(typeNew).GetBoundaryType() == 1)
    {
        typeNew = int(rand() / float(RAND_MAX) * numOfTemplates);
        typeNew = typeNew % numOfTemplates;
        boundaryNew = m_ptrTemplates->GetRoom(typeNew).GetBoundaryType();
        whileCnt++;
        if (whileCnt >= 1000)
        {
            std::cout << "Break from the while loop after reaching enough number of trials in RandomlyAdjustOneRoom04()!\n";
            return -1;
        }
    }
    ptrGraph->GetNode(pickedRoomIndex).SetType(typeNew);
    CRoom room = m_ptrTemplates->GetRoom(typeNew);
    v2f p1 = room.GetRoomCenter();
    v2f p2 = pickedRoom.GetRoomCenter();
    v2f dp = p2 - p1;
    room.TranslateRoom(dp);
    pickedRoom = room;
#if 1 // New on 09/15/2013
    //SampleConfigSpaceForPickedRoom(layout, ptrGraph, indices, pickedRoomIndex);
#endif
    return pickedRoomIndex;
}

bool CLevelSynth::GetLayoutEnergyEarlyOut(CRoomLayout& layout, CPlanarGraph* ptrGraph, float& collideArea, float& connectivity, int roomMoved, float* energyTmp, float energyCurrent)
{
    layout.ResetRoomEnergies();
    *energyTmp = 1.f;

    // do connectivity first, it's (probably?) cheaper

    if (CLevelConfig::m_sigmaConnectivity > 0.f)
    {
        connectivity = CheckRoomConnectivity(layout, ptrGraph, true, roomMoved);
        (*energyTmp) *= exp(connectivity * CLevelConfig::m_sigmaConnectivity);
    }
    if (*energyTmp > energyCurrent)
    {
        return false;
    }
    if (CLevelConfig::m_sigmaCollide > 0.f)
    {
        collideArea = LayoutCollide(layout, ptrGraph, true, roomMoved);
        (*energyTmp) *= exp(collideArea * CLevelConfig::m_sigmaCollide);
    }
    if (*energyTmp > energyCurrent)
    {
        return false;
    }

    if (CLevelConfig::m_sigmaContact > 0.f)
    {
        float contactArea = LayoutContact(layout, ptrGraph, true, CLevelConfig::m_flagNonOverlapContact);
        (*energyTmp) *= exp(-contactArea * CLevelConfig::m_sigmaContact);
    }
    return true;
}

float CLevelSynth::GetLayoutEnergy(CRoomLayout& layout, CPlanarGraph* ptrGraph, float& collideArea, float& connectivity, int roomMoved, bool doContact, std::vector<int>* indices)
{
    layout.ResetRoomEnergies();
    float layoutEnergy = 1.f;
    if (CLevelConfig::m_sigmaCollide > 0.f)
    {
        collideArea = LayoutCollide(layout, ptrGraph, true, roomMoved);
        layoutEnergy *= exp(collideArea * CLevelConfig::m_sigmaCollide);
    }

    if (CLevelConfig::m_sigmaConnectivity > 0.f)
    {
        connectivity = CheckRoomConnectivity(layout, ptrGraph, true, roomMoved);
        layoutEnergy *= exp(connectivity * CLevelConfig::m_sigmaConnectivity);
    }
    if (CLevelConfig::m_sigmaContact > 0.f && doContact)
    {
        float contactArea = -LayoutContact(layout, ptrGraph, true, CLevelConfig::m_flagNonOverlapContact, indices);

        if (contactArea >= 0.0f)
        {
            contactArea = 0.0;
        }
        if (contactArea < 0)
        {
            layoutEnergy *= exp(contactArea / CLevelConfig::m_sigmaContact);
        }
    }

    return layoutEnergy;
}

float CLevelSynth::CheckRoomConnectivity(CRoomLayout& layout, CPlanarGraph* ptrGraph, bool flagVisitedOnly /* = false */, int roomMoved)
{
    float connectivity = 0.f;
    if (ptrGraph == NULL)
    {
        return connectivity;
    }
    for (int i = 0; i < ptrGraph->GetNumOfEdges(); i++)
    {
        CGraphEdge& edge = ptrGraph->GetEdge(i);
        int idx0 = edge.GetIdx0();
        int idx1 = edge.GetIdx1();
        bool flagVisited0 = ptrGraph->GetNode(idx0).GetFlagVisited();
        bool flagVisited1 = ptrGraph->GetNode(idx1).GetFlagVisited();
        if (flagVisitedOnly == true && (flagVisited0 == false || flagVisited1 == false))
        {
            continue;
        }
        bool flagFixed0 = ptrGraph->GetNode(idx0).GetFlagFixed();
        bool flagFixed1 = ptrGraph->GetNode(idx1).GetFlagFixed();
        if (flagFixed0 == true && flagFixed1 == true)
        {
            continue;
        }
        if (roomMoved == -1 || roomMoved == idx0 || roomMoved == idx1 || layout.cachedConnectivities.find(std::make_pair(idx0, idx1)) == layout.cachedConnectivities.end())
        {
            float contactArea = RoomContact(layout.GetRoom(idx0), layout.GetRoom(idx1));
            if (contactArea <= CLevelConfig::m_roomContactThresh)
            {
                if (CLevelConfig::m_flagDiscreteConnectFunc == true)
                {
                    connectivity += 1.f;
                    layout.cachedConnectivities[std::make_pair(idx0, idx1)] = 1.f;
                }
                else
                {
                    float d = RoomDistance(layout.GetRoom(idx0), layout.GetRoom(idx1));
                    d += CLevelConfig::m_roomContactThresh;
                    layout.cachedConnectivities[std::make_pair(idx0, idx1)] = d;
                    connectivity += d;
                }
                float factor = 1.1f;
                layout.GetRoom(idx0).UpdateEnergy(factor);
                layout.GetRoom(idx1).UpdateEnergy(factor);
            }
            else
            {
                layout.cachedConnectivities[std::make_pair(idx0, idx1)] = 0.0f;
            }
        }
        else
        {
            connectivity += layout.cachedConnectivities[std::make_pair(idx0, idx1)];
        }
    }

    return connectivity;
}

float CLevelSynth::LayoutCollide(CRoomLayout& layout, CPlanarGraph* ptrGraph, bool flagVisitedOnly /* = false */, int roomThatMoved /* = -1 */)
{
    float collideAreaTotal = 0.f;
    int collideCount = 0;
    int numOfRooms = layout.GetNumOfRooms();
    for (int i = 0; i < numOfRooms; i++)
    {
        for (int j = i + 1; j < numOfRooms; j++)
        {
            bool flagVisited0 = ptrGraph->GetNode(i).GetFlagVisited();
            bool flagVisited1 = ptrGraph->GetNode(j).GetFlagVisited();
            if (flagVisitedOnly == true && (flagVisited0 == false || flagVisited1 == false))
            {
                continue;
            }
            bool flagFixed0 = ptrGraph->GetNode(i).GetFlagFixed();
            bool flagFixed1 = ptrGraph->GetNode(j).GetFlagFixed();
            if (flagFixed0 == true && flagFixed1 == true)
            {
                continue;
            }
            if (roomThatMoved == -1 || roomThatMoved == i || roomThatMoved == j || layout.cachedCollisionEnergies.find(std::make_pair(i, j)) == layout.cachedCollisionEnergies.end())
            {
                float collideArea = RoomCollides(layout.GetRoom(i), layout.GetRoom(j));
                if (collideArea > 0.f)
                {
                    collideAreaTotal += collideArea;
                    collideCount++;
                    float factor = exp(collideArea);
                    layout.GetRoom(i).UpdateEnergy(factor);
                    layout.GetRoom(j).UpdateEnergy(factor);
                    layout.cachedCollisionEnergies[std::make_pair(i, j)] = collideArea;
                }
                else
                {
                    layout.cachedCollisionEnergies[std::make_pair(i, j)] = collideArea;
                }
            }
            else
            {
                collideAreaTotal += layout.cachedCollisionEnergies[std::make_pair(i, j)];
            }
        }
    }

#ifdef PRINT_OUT_DEBUG_INFO
    std::cout << "Number of colliding room pairs: " << collideCount << std::endl;
    std::cout << "Total area of colliding area: " << collideAreaTotal << std::endl;
#endif
    return collideAreaTotal;
}

float CLevelSynth::LayoutCollide(CRoomLayout& layout)
{
    float collideAreaTotal = 0.f;
    int collideCount = 0;
    int numOfRooms = layout.GetNumOfRooms();
    for (int i = 0; i < numOfRooms; i++)
    {
        for (int j = i + 1; j < numOfRooms; j++)
        {
            if (layout.GetRoom(i).GetBoundaryType() == 1 && layout.GetRoom(j).GetBoundaryType() == 1)
            {
                continue;
            }
            float collideArea = RoomCollides(layout.GetRoom(i), layout.GetRoom(j));
            if (collideArea > 0.f)
            {
                collideAreaTotal += collideArea;
                collideCount++;
            }
        }
    }
#ifdef PRINT_OUT_DEBUG_INFO
    std::cout << "Number of colliding room pairs: " << collideCount << std::endl;
    std::cout << "Total area of colliding area: " << collideAreaTotal << std::endl;
#endif
    return collideAreaTotal;
}

float CLevelSynth::RoomCollides(CRoom& room1, CRoom& room2)
{
    float collideArea = -1.f;

    // Test the bounding box first...
    AABB2f bb1, bb2;
    room1.GetRoomBoundingBox(bb1);
    room2.GetRoomBoundingBox(bb2);
    if (TestBoundingBoxCollides(bb1, bb2) == false)
    {
        return 0.f;
    }
    // Use the Clipper library...
    CClipperWrapper wrapper;
    collideArea = wrapper.ComputeCollideArea(room1, room2);

    return collideArea;
}

float CLevelSynth::BoundingBoxCollidesArea(AABB2f& bb1, AABB2f& bb2)
{
    float collideArea = -1.f;
    for (int j = 0; j < 2; j++)
    {
        if (bb1.m_posMax[j] < bb2.m_posMin[j] || bb1.m_posMin[j] > bb2.m_posMax[j])
        {
            return collideArea;
        }
    }
    collideArea = 1.f;
    for (int j = 0; j < 2; j++)
    {
        float pMin = max(bb1.m_posMin[j], bb2.m_posMin[j]);
        float pMax = min(bb1.m_posMax[j], bb2.m_posMax[j]);
        if (pMin > pMax) return -1.f;
        float pd = pMax - pMin;
        collideArea *= pd;
    }
    return collideArea;
}

bool CLevelSynth::TestBoundingBoxCollides(AABB2f& bb1, AABB2f& bb2)
{
    for (int j = 0; j < 2; j++)
    {
        if (bb1.m_posMax[j] < bb2.m_posMin[j] || bb1.m_posMin[j] > bb2.m_posMax[j])
        {
            return false;
        }
    }
    return true;
}

float CLevelSynth::LayoutContact(CRoomLayout& layout, CPlanarGraph* ptrGraph, bool flagVisitedOnly /* = false */, bool flagNonOverlap /* = false */, std::vector<int>* indices, int roomThatMoved /* probably == null */)
{
    float contactAreaTotal = 0.f;
    int contactCount = 0;
    int numOfRooms = layout.GetNumOfRooms();

    for (int i = 0; i < numOfRooms; i++)
    {
        bool flagVisited0 = ptrGraph->GetNode(i).GetFlagVisited();
        if (flagVisited0 == false)
        {
            continue;
        }

        bool badNeighbour = false;
        std::vector<int>& neighbours = ptrGraph->GetNode(i).GetNeighbors();
        for (int j = 0; j < neighbours.size(); j++)
        {
            bool found = false;
            for (int k = 0; k < indices->size(); k++)
            {
                if ((*indices)[k] == neighbours[j])
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                badNeighbour = true;
                break;
            }
        }
        if (!badNeighbour)
        {
            continue;
        }
        float perimeter = RoomPerimeter(layout.GetRoom(i));

        for (int j = i + 1; j < numOfRooms; j++)
        {
            bool flagVisited1 = ptrGraph->GetNode(j).GetFlagVisited();
            if (flagVisitedOnly == true && (flagVisited0 == false || flagVisited1 == false))
            {
                continue;
            }
            if (i == roomThatMoved || j == roomThatMoved || roomThatMoved == -1 || layout.cachedContacts.find(std::make_pair(i, j)) == layout.cachedContacts.end())
            {
                if (RoomCollides(layout.GetRoom(i), layout.GetRoom(j)) > 0.f)
                {
                    layout.cachedContacts[std::make_pair(i, j)] = 0.0f;
                    continue;
                }
                float contactArea = RoomContact(layout.GetRoom(i), layout.GetRoom(j));
                if (contactArea > CLevelConfig::m_roomContactThresh) //0.f
                {
                    contactArea -= CLevelConfig::m_roomContactThresh;
                    layout.cachedContacts[std::make_pair(i, j)] = contactArea;
                    perimeter -= contactArea;
                    contactCount++;
                }
            }
            else
            {
                perimeter -= layout.cachedContacts[std::make_pair(i, j)];
            }
        }
        if (perimeter > 0)
        {
            contactAreaTotal += perimeter;
        }
    }
#ifdef PRINT_OUT_DEBUG_INFO
    std::cout << "Number of contacting room pairs: " << contactCount << std::endl;
    std::cout << "Total area of contacting area: " << contactAreaTotal << std::endl;
#endif
    return contactAreaTotal;
}

v2f CLevelSynth::ComputeLabelPosition(int idx, CPlanarGraph* ptrGraph, float labelRad)
{
    v2f pMin, pMax;
    ptrGraph->GetGraphBoundingBox(pMin, pMax);
    float vMax = max(max(std::abs(pMin[0]), std::abs(pMin[1])), max(std::abs(pMax[0]), std::abs(pMax[1])));
    const v2f pos = ptrGraph->GetNodePos(idx);
    const float rad = labelRad;
    int n = 32;
    float dMinMax = -1e10;
    v2f piMax;
    for (int i = 0; i < n; i++)
    {
        float angle = atan(1.f) * 8.f * float(i) / float(n);
        float cv = -cos(angle) * rad;
        float sv = sin(angle) * rad;
        v2f pi = pos + v2f(cv, sv);
        if (pi[0] < -vMax || pi[0] > vMax || pi[1] < -vMax || pi[1] > vMax)
        {
            continue;
        }
        float dMin = 1e10;
        for (int j = 0; j < ptrGraph->GetNumOfEdges(); j++)
        {
            CGraphEdge& edge = ptrGraph->GetEdge(j);
            int idx1 = edge.GetIdx0();
            int idx2 = edge.GetIdx1();
            if (idx1 != idx && idx2 != idx)
            {
                continue;
            }
            v2f pos1 = ptrGraph->GetNodePos(idx1);
            v2f pos2 = ptrGraph->GetNodePos(idx2);
            float dTmp = PointToSegmentSqDistance(pi, CLineBase(pos1, pos2));
            if (dTmp < dMin)
            {
                dMin = dTmp;
            }
        }
        if (dMin >= rad * rad)
        {
            return pi;
        }
        if (dMin > dMinMax)
        {
            dMinMax = dMin;
            piMax = pi;
        }
    }
    return piMax;
}
