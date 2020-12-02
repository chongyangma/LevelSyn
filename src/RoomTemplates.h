//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-03-04
//  email:  chongyangm@gmail.com
//  info: class declaration of room templates
// --------------------------------------------------------------

#ifndef ROOMTEMPLATES_H
#define ROOMTEMPLATES_H

#include "Room.h"

class CRoomTemplates
{
public:
    void ClearTemplates() { m_rooms.clear(); }

    void PrintTemplates();

    void AddTemplate(CRoom& room) { m_rooms.push_back(room); }

    bool LoadTemplatesFromXML(const char* fileName);

    bool SaveTemplatesAsXML(const char* fileName);

    int GetNumOfTemplates() { return int(m_rooms.size()); }

    CRoom& GetRoom(int idx) { return m_rooms[idx]; }

    const std::vector<CRoom>& GetRooms() const { return m_rooms; }

    void EnrichByRotating180Degrees();

    void EnrichByIntroducingSizeVariation();

    void SetRoomTypes();

private:
    std::vector<CRoom> m_rooms;
};

#endif // ROOMTEMPLATES_H
