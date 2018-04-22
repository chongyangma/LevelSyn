
#include "RoomLayout.h"

int CRoomLayout::GetNumOfVertices()
{
	int numOfVertices = 0;
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		numOfVertices += m_rooms[i].GetNumOfVertices();
	}
	return numOfVertices;
}

int CRoomLayout::GetNumOfEdges()
{
	int numOfEdges = 0;
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		numOfEdges += m_rooms[i].GetNumOfEdges();
	}
	return numOfEdges;
}

v2i CRoomLayout::GetNearestEdgePair(int roomIdx0, int roomIdx1)
{
	v2i pairMin(-1);
	float distMin = 1e10;
	CRoom& room0 = GetRoom(roomIdx0);
	CRoom& room1 = GetRoom(roomIdx1);
	int numOfEdges0 = room0.GetNumOfEdges(); 
	int numOfEdges1 = room1.GetNumOfEdges();
	for ( int i=0; i<numOfEdges0; i++ )
	{
		int idx00 = i;
		int idx01 = (i + 1) % numOfEdges0;
		v2f e0 = room0.GetVertex(idx01) - room0.GetVertex(idx00);
		v2f p0 = (room0.GetVertex(idx01) + room0.GetVertex(idx00)) * 0.5f;
		for ( int j=0; j<numOfEdges1; j++ )
		{
			int idx10 = j;
			int idx11 = (j + 1) % numOfEdges1;
			v2f e1 = room1.GetVertex(idx11) - room1.GetVertex(idx10);
			v2f p1 = (room1.GetVertex(idx11) + room1.GetVertex(idx10)) * 0.5f;
			v2f pr = p1 - p0;
			if ( std::abs(dot(e0, e1)) < 1e-10 )
			{
				continue;
			}
			float distTmp = mag2(pr);
			if ( distTmp < distMin )
			{
				distMin = distTmp;
				pairMin[0] = i;
				pairMin[1] = j;
			}
		}
	}
	return pairMin;
}

void CRoomLayout::GetLayoutBoundingBox(v2f& posMin, v2f& posMax)
{
	v2f pMin(1e10);
	v2f pMax(-1e10);
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		v2f pMinTmp;
		v2f pMaxTmp;
		GetRoom(i).GetRoomBoundingBox(pMinTmp, pMaxTmp);
		for ( int j=0; j<2; j++ )
		{
			pMin[j] = min(pMin[j], pMinTmp[j]);
			pMax[j] = max(pMax[j], pMaxTmp[j]);
		}
	}
	posMin = pMin;
	posMax = pMax;
}

void CRoomLayout::MoveToSceneCenter()
{
	v2f posMin, posMax;
	GetLayoutBoundingBox(posMin, posMax);
	v2f posCen = (posMin + posMax) * 0.5f;
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		GetRoom(i).TranslateRoom(-posCen);
	}
}

std::vector<v2f> CRoomLayout::GetRoomPositions()
{
	std::vector<v2f> roomPositions(GetNumOfRooms());
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		roomPositions[i] = GetRoom(i).GetRoomCenter();
	}
	return roomPositions;
}

void CRoomLayout::ResetRoomEnergies()
{
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		GetRoom(i).ResetEnergy();
	}
}

void CRoomLayout::PrintLayout()
{
	std::cout << "A layout with " << GetNumOfRooms() << " rooms...\n";
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		std::cout << i << "th room:\n";
		GetRoom(i).PrintRoom();
	}
}

#include "PlanarGraph.h"

bool CRoomLayout::SaveLayoutAsSVG(const char* fileName, int wd /* = 400 */, int ht /* = 400 */, bool visitedOnly /* = FALSE */, class CPlanarGraph *graphBest /* = NULL */, bool labelFlag /* = true */ )
{
	int strokeWd = 4;
	v2f posMin, posMax;
	GetLayoutBoundingBox(posMin, posMax);
	float pMin = min(posMin[0], posMin[1]);
	float pMax = max(posMax[0], posMax[1]);
	/*
	#ifdef DUMP_INTERMEDIATE_OUTPUT
	pMin = -1.f;
	pMax = 1.f;
	#endif
	*/

	float scaling = 1.05f;
	pMin *= scaling;
	pMax *= scaling;
	const char* str = "\t<?xml version=\"1.0\" standalone=\"no\" ?>\n"
		"<!-- layout visualization -->\n"
		"<svg>\n"
		"</svg>\n";
	TiXmlDocument doc;
	doc.Parse(str);
	TiXmlElement* root = doc.RootElement();
	std::ostringstream ossViewBox;
	ossViewBox << 0 << " " << 0 << " " << wd << " " << ht;
	root->SetAttribute("viewBox", ossViewBox.str().c_str());
	root->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
	// Draw a background...
	TiXmlElement bgElement("rect");
	bgElement.SetAttribute("x", 0);
	bgElement.SetAttribute("y", 0);
	bgElement.SetAttribute("width", wd);
	bgElement.SetAttribute("height", ht);
	bgElement.SetAttribute("fill", "#00A000");
	bgElement.SetAttribute("stroke", "none");
	//root->InsertEndChild(bgElement);
	// Dump rooms as closed polygons...
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		if (graphBest != NULL && graphBest->GetNode(i).GetFlagVisited() == false && visitedOnly)
		{
			continue;
		}
		TiXmlElement roomElement("path");
		std::ostringstream ossPath;
		CRoom& room = GetRoom(i);
		for ( int j=0; j<room.GetNumOfVertices(); j++ )
		{
			v2f pj = room.GetVertex(j);
			if ( j == 0 )
			{
				ossPath << "M ";
			}
			else
			{
				ossPath << "L ";
			}
			ossPath << ConvertPosX(pj[0], pMin, pMax, wd) << " ";
			ossPath << ConvertPosY(pj[1], pMin, pMax, ht) << " ";
		}
		ossPath << "z";
		roomElement.SetAttribute("d", ossPath.str().c_str());
		if ( room.GetFlagFixed() == true )
		{
			roomElement.SetAttribute("fill", "#808080");
		}
		else if ( room.GetBoundaryType() == 2 )
		{
			// Corridors...
			roomElement.SetAttribute("fill", "#FAFAFA");
		}
		else
		{
			roomElement.SetAttribute("fill", "#E0E0E0");
		}
		roomElement.SetAttribute("stroke", "none");
		root->InsertEndChild(roomElement);
	}
	// Dump rooms as sets of line segments...
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		if (graphBest != NULL && graphBest->GetNode(i).GetFlagVisited() == false && visitedOnly)
		{
			continue;
		}
		TiXmlElement roomElement("path");
		std::ostringstream ossPath;
		CRoom& room = GetRoom(i);

		if ( room.HasWalls() == false )
		{
			for ( int j=0; j<room.GetNumOfVertices(); j++ )
			{
				v2f pj = room.GetVertex(j);
				if ( j == 0 )
				{
					ossPath << "M ";
				}
				else
				{
					ossPath << "L ";
				}
				ossPath << ConvertPosX(pj[0], pMin, pMax, wd) << " ";
				ossPath << ConvertPosY(pj[1], pMin, pMax, ht) << " ";
			}
			ossPath << "z";
		}
		else
		{
			for ( int j=0; j<room.GetNumOfWalls(); j++ )
			{
				RoomWall& wall = room.GetWall(j);
				v2f p1 = wall.GetPos1();
				v2f p2 = wall.GetPos2();
				ossPath << "M ";
				ossPath << ConvertPosX(p1[0], pMin, pMax, wd) << " ";
				ossPath << ConvertPosY(p1[1], pMin, pMax, ht) << " ";
				ossPath << "L ";
				ossPath << ConvertPosX(p2[0], pMin, pMax, wd) << " ";
				ossPath << ConvertPosY(p2[1], pMin, pMax, ht) << " ";
			}
		}
		roomElement.SetAttribute("d", ossPath.str().c_str());
		roomElement.SetAttribute("fill", "none");
		roomElement.SetAttribute("stroke", "black");
		roomElement.SetAttribute("stroke-width", strokeWd);
		root->InsertEndChild(roomElement);
	}
	// Dump vertices...
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		if (graphBest != NULL && graphBest->GetNode(i).GetFlagVisited() == false && visitedOnly)
		{
			continue;
		}
		CRoom& room = GetRoom(i);
		for ( int j=0; j<room.GetNumOfVertices(); j++ )
		{
			TiXmlElement vertexElement("circle");
			v2f pj = room.GetVertex(j);
			vertexElement.SetAttribute("cx", ConvertPosX(pj[0], pMin, pMax, wd));
			vertexElement.SetAttribute("cy", ConvertPosY(pj[1], pMin, pMax, ht));
			vertexElement.SetAttribute("r", strokeWd / 2);
			vertexElement.SetAttribute("fill", "black");
			vertexElement.SetAttribute("stroke", "none");
			root->InsertEndChild(vertexElement);
		}
	}
	// Dump corridor walls...
	for ( int i=0; i<GetNumOfCorridorWalls(); i++ )
	{
		CorridorWall& wall = GetCorridorWall(i);
		v2f p1 = wall.GetPos1();
		v2f p2 = wall.GetPos2();
		TiXmlElement wallElement("path");
		std::ostringstream ossWall;
		ossWall << "M ";
		ossWall << CRoomLayout::ConvertPosX(p1[0], pMin, pMax, wd) << " ";
		ossWall << CRoomLayout::ConvertPosY(p1[1], pMin, pMax, ht) << " ";
		ossWall << "L ";
		ossWall << CRoomLayout::ConvertPosX(p2[0], pMin, pMax, wd) << " ";
		ossWall << CRoomLayout::ConvertPosY(p2[1], pMin, pMax, ht) << " ";
		wallElement.SetAttribute("d", ossWall.str().c_str());
		wallElement.SetAttribute("fill", "none");
		wallElement.SetAttribute("stroke", "black");
		wallElement.SetAttribute("stroke-width", strokeWd);
		root->InsertEndChild(wallElement);
		TiXmlElement vertexElement1("circle");
		vertexElement1.SetAttribute("cx", ConvertPosX(p1[0], pMin, pMax, wd));
		vertexElement1.SetAttribute("cy", ConvertPosY(p1[1], pMin, pMax, ht));
		vertexElement1.SetAttribute("r", strokeWd / 2);
		vertexElement1.SetAttribute("fill", "black");
		vertexElement1.SetAttribute("stroke", "none");
		root->InsertEndChild(vertexElement1);
		TiXmlElement vertexElement2("circle");
		vertexElement2.SetAttribute("cx", ConvertPosX(p2[0], pMin, pMax, wd));
		vertexElement2.SetAttribute("cy", ConvertPosY(p2[1], pMin, pMax, ht));
		vertexElement2.SetAttribute("r", strokeWd / 2);
		vertexElement2.SetAttribute("fill", "black");
		vertexElement2.SetAttribute("stroke", "none");
		root->InsertEndChild(vertexElement2);
	}
	// Dump labels...
	for ( int i=0; i<GetNumOfRooms(); i++ )
	{
		if (graphBest != NULL && graphBest->GetNode(i).GetFlagVisited() == false && visitedOnly)
		{
			continue;
		}

		int shiftX = (i >= 10) ? 8 : 3;
		int shiftY = 5;
		v2f pi = GetRoom(i).GetRoomCenter();
		pi = pi + GetRoom(i).GetCenterShift();
		TiXmlElement labelElement("text");
		labelElement.SetAttribute("x", ConvertPosX(pi[0], pMin, pMax, wd) - shiftX);
		labelElement.SetAttribute("y", ConvertPosY(pi[1], pMin, pMax, ht) + shiftY);
		labelElement.SetAttribute("font-family", "Verdana");
		labelElement.SetAttribute("font-size", 15);
		labelElement.SetAttribute("fill", "blue");
		std::ostringstream ossLabel;
		ossLabel << i;
		TiXmlText labelText(ossLabel.str().c_str());
		labelElement.InsertEndChild(labelText);
		if ( labelFlag == true )
		{
			root->InsertEndChild(labelElement);
		}
	}

	bool saveFlag = doc.SaveFile(fileName);
	return saveFlag;
}

int CRoomLayout::ConvertPos(float p, float pMin, float pMax, int sz)
{
	float pd = (p - pMin) / (pMax - pMin) * sz;
	return int(pd);
}

int CRoomLayout::ConvertPosX(float p, float pMin, float pMax, int sz)
{
	return ConvertPos(p, pMin, pMax, sz);
}

int CRoomLayout::ConvertPosY(float p, float pMin, float pMax, int sz)
{
	return sz - 1 - ConvertPos(p, pMin, pMax, sz);
}
