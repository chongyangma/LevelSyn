
#include "RoomTemplates.h"
#include <sstream>

void CRoomTemplates::PrintTemplates()
{
	std::cout << "There are " << m_rooms.size() << " room templates!\n";
	for ( int i=0; i<int(m_rooms.size()); i++ )
	{
		std::vector<v2f> vertices = m_rooms[i].GetVertices();
		std::cout << "Room " << i << " with " << vertices.size() << " vertices:\n";
		for ( int j=0; j<int(vertices.size()); j++ )
		{
			std::cout << vertices[j] << " ";
		}
		std::cout << std::endl;
	}
}

bool CRoomTemplates::LoadTemplatesFromXML(const char* fileName)
{
	// Clear the current templates...
	ClearTemplates();

	TiXmlDocument doc;
	bool loadFlag = doc.LoadFile(fileName);
	if ( loadFlag == false )
	{
		std::cout << "Failed to load templates from " << fileName << "!\n";
		return loadFlag;
	}
	//doc.Print();

	TiXmlNode* xmlRoot = 0;
	xmlRoot = doc.RootElement();
	assert( xmlRoot );

	TiXmlNode* xmlNode = xmlRoot->FirstChild();
	while ( xmlNode != 0 )
	{
		if ( strcmp(xmlNode->Value(), "Room") == 0 )
		{
			std::vector<int> doorPositions;
			// Parse a room...
			CRoom room;
			std::vector<v2f> vertices;
			TiXmlNode* xmlChildNode = xmlNode->FirstChild();
			while ( xmlChildNode != 0 )
			{
				if ( strcmp(xmlChildNode->Value(), "Vertex") == 0 )
				{
					// Parse a vertex...
					float px, py;
					int rx = xmlChildNode->ToElement()->QueryFloatAttribute("px", &px);
					int ry = xmlChildNode->ToElement()->QueryFloatAttribute("py", &py);
					if ( rx == TIXML_SUCCESS && ry == TIXML_SUCCESS )
					{
						v2f pos;
						pos[0] = px;
						pos[1] = py;
						vertices.push_back(pos);
					}
				}
				else if ( strcmp(xmlChildNode->Value(), "Shift") == 0 )
				{
					float px, py;
					int rx = xmlChildNode->ToElement()->QueryFloatAttribute("px", &px);
					int ry = xmlChildNode->ToElement()->QueryFloatAttribute("py", &py);
					if ( rx == TIXML_SUCCESS && ry == TIXML_SUCCESS )
					{
						v2f pos;
						pos[0] = px;
						pos[1] = py;
						room.SetCenterShift(pos);
					}
				}
				else if ( strcmp(xmlChildNode->Value(), "Boundary") == 0 )
				{
					int type;
					int r = xmlChildNode->ToElement()->QueryIntAttribute("type", &type);
					if ( r == TIXML_SUCCESS )
					{
						room.SetBoundaryType(type);
					}
				}
				else if ( strcmp(xmlChildNode->Value(), "Door") == 0 )
				{
					int idx;
					int r = xmlChildNode->ToElement()->QueryIntAttribute("edgeIndex", &idx);
					if ( r == TIXML_SUCCESS )
					{
						doorPositions.push_back(idx);
					}
				}
				xmlChildNode = xmlChildNode->NextSibling();
			}
			room.SetVertices(vertices);
			if ( doorPositions.empty() == false )
			{
				room.ResetDoorFlags();
				for ( int i=0; i<int(doorPositions.size()); i++ )
				{
					room.SetDoorFlag(doorPositions[i], true);
				}
			}
			AddTemplate(room);
		}

		// Move to the next sibling...
		xmlNode = xmlNode->NextSibling();
	}

	SetRoomTypes();
	//PrintTemplates();
	return loadFlag;
}

bool CRoomTemplates::SaveTemplatesAsXML(const char* fileName)
{
	const char* str =	"\t<?xml version=\"1.0\" standalone=\"yes\" ?>\n"
		"<!-- room template data -->\n"
		"<Templates>\n"
		"</Templates>\n";
	TiXmlDocument doc;
	doc.Parse(str);
	TiXmlNode* root = doc.RootElement();
	// Dump nodes...
	for ( int i=0; i<GetNumOfTemplates(); i++ )
	{
		TiXmlElement roomElement("Room");
		for ( int j=0; j<GetRoom(i).GetNumOfVertices(); j++ )
		{
			TiXmlElement vertexElement("Vertex");
			v2f pj = GetRoom(i).GetVertex(j);
			std::ostringstream oss0;
			std::ostringstream oss1;
			oss0 << pj[0];
			oss1 << pj[1];
			vertexElement.SetAttribute("px", oss0.str().c_str());
			vertexElement.SetAttribute("py", oss1.str().c_str());
			roomElement.InsertEndChild(vertexElement);
		}
		// Dump center shift...
		TiXmlElement shiftElement("Shift");
		v2f shift = GetRoom(i).GetCenterShift();
		std::ostringstream oss0;
		std::ostringstream oss1;
		oss0 << shift[0];
		oss1 << shift[1];
		shiftElement.SetAttribute("px", oss0.str().c_str());
		shiftElement.SetAttribute("py", oss1.str().c_str());
		roomElement.InsertEndChild(shiftElement);
		// Dump boundary type...
		if ( GetRoom(i).GetBoundaryType() != 0 )
		{
			TiXmlElement boundaryElement("Boundary");
			boundaryElement.SetAttribute("type", GetRoom(i).GetBoundaryType());
			roomElement.InsertEndChild(boundaryElement);
		}
		// Dump door positions...
		if ( GetRoom(i).HasRestrictedDoorPosition() == true )
		{
			std::vector<bool> doorFlags = GetRoom(i).GetDoorFlags();
			for ( int j=0; j<int(doorFlags.size()); j++ )
			{
				if ( doorFlags[j] == false )
				{
					continue;
				}
				TiXmlElement doorElement("Door");
				doorElement.SetAttribute("edgeIndex", j);
				roomElement.InsertEndChild(doorElement);
			}
		}
		// Add room...
		root->InsertEndChild(roomElement);
	}

	bool saveFlag = doc.SaveFile(fileName);
	return saveFlag;
}

void CRoomTemplates::EnrichByRotating180Degrees()
{
	int numOfTemplatesOld = GetNumOfTemplates();
	for ( int i=0; i<numOfTemplatesOld; i++ )
	{
		CRoom roomNew = GetRoom(i);
		roomNew.RotateRoom(atan(1.f) * 4.f);
		AddTemplate(roomNew);
	}
	SetRoomTypes();
}

void CRoomTemplates::EnrichByIntroducingSizeVariation()
{
	int numOfTemplatesOld = GetNumOfTemplates();
	for ( int n=0; n<3; n++ )
	{
		for ( int i=0; i<numOfTemplatesOld; i++ )
		{
			CRoom roomNew = GetRoom(i);
			float rx = 1.f + rand() / float(RAND_MAX);
			float ry = 1.f + rand() / float(RAND_MAX);
			roomNew.ScaleRoom(v2f(rx, ry));
			AddTemplate(roomNew);
		}
	}
	SetRoomTypes();
}

void CRoomTemplates::SetRoomTypes()
{
	for ( int i=0; i<GetNumOfTemplates(); i++ )
	{
		m_rooms[i].SetTemplateType(i);
	}
}
