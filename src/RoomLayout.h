//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-03-07
//  email:  chongyangm@gmail.com
//  info: class declaration of a room layout, i.e. a set of rooms
// --------------------------------------------------------------

#ifndef ROOMLAYOUT_H
#define ROOMLAYOUT_H

//#define DUMP_INTERMEDIATE_OUTPUT
//#define DUMP_PARTIAL_SOLUTION
#define PERFORMANCE_TEST 1

#include "Room.h"

#include <map>

typedef CLineBase CorridorWall;

class CRoomLayout
{
public:
    void ClearLayout() { m_rooms.clear(); }

    void AddRoom(CRoom& room) { m_rooms.push_back(room); }

    int GetNumOfRooms() { return int(m_rooms.size()); }

    int GetNumOfVertices();

    int GetNumOfEdges();

    CRoom& GetRoom(int idx) { return m_rooms[idx]; }

    v2i GetNearestEdgePair(int roomIdx0, int roomIdx1);

    void GetLayoutBoundingBox(v2f& posMin, v2f& posMax);

    void MoveToSceneCenter();

    std::vector<v2f> GetRoomPositions();

    void ResetRoomEnergies();

    void PrintLayout();

    bool SaveLayoutAsSVG(const char* fileName, int wd = 400, int ht = 400, bool writeOnlyVisited = false, class CPlanarGraph* graphBest = NULL, bool labelFlag = true);

    static int ConvertPos(float p, float pMin, float pMax, int sz);

    static int ConvertPosX(float p, float pMin, float pMax, int sz);

    static int ConvertPosY(float p, float pMin, float pMax, int sz);

    void InsertCorridorWall(CorridorWall& wall) { m_corridorWalls.push_back(wall); }

    int GetNumOfCorridorWalls() const { return int(m_corridorWalls.size()); }

    RoomWall& GetCorridorWall(int idx) { return m_corridorWalls[idx]; }

    std::map<std::pair<int, int>, float> cachedCollisionEnergies;
    std::map<std::pair<int, int>, float> cachedConnectivities;
    std::map<std::pair<int, int>, float> cachedContacts;

private:
    std::vector<CRoom> m_rooms;
    std::vector<CorridorWall> m_corridorWalls;
};

#endif // ROOMLAYOUT_H
