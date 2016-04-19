//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-07-15
//  email:  chongyangm@gmail.com
//  info: class declaration of a configuration space
// --------------------------------------------------------------

#ifndef CONFIGSPACE_H
#define CONFIGSPACE_H

#include "Room.h"
#include "clipperWrapper.h"
#include "LevelMath.h"

using namespace level_math;

class CConfigLine : public CLineBase
{
public:
	CConfigLine(v2f pos1, v2f pos2) : CLineBase(pos1, pos2) {}

	CConfigLine(v2f pos) : CLineBase(pos) {}

	v2f RandomlySampleConfigLine();

	v2f RandomlySampleConfigLineDiscrete();

	float GetConfigLineLength() const;

	float GetConfigLineSqLength() const;

	void PrintConfigLine();

	void TranslateConfigLine(v2f trans);
};

class CConfigSpace
{
public:

	CConfigSpace() {}

	// The configuration space to put room2 around room1...
	CConfigSpace(const CRoom& room1, const CRoom& room2);

	CConfigSpace(std::vector<CConfigLine>& vecConfigLines);

	void AddConfigLine(CConfigLine& line) { m_vecConfigLine.push_back(line); }

	v2f RandomlySampleConfigSpace();

	v2f RandomlySampleConfigSpaceContinuous();

	v2f RandomlySampleConfigSpaceDiscrete();

	std::vector<v2f> SmartlySampleConfigSpace();

	int GetNumOfLines() { return int(m_vecConfigLine.size()); }

	bool IsEmpty() { return m_vecConfigLine.empty(); }

	void SetConfigLines(std::vector<CConfigLine>& vecConfigLine) { m_vecConfigLine = vecConfigLine; }

	std::vector<CConfigLine>& GetConfigLines() { return m_vecConfigLine; }

	CConfigLine& GetConfigLine(int idx) { return m_vecConfigLine[idx]; }

	static CConfigSpace FindIntersection(CConfigSpace& configSpace1, CConfigSpace& configSpace2);

	static CConfigSpace FindUnion(CConfigSpace& configSpace, CConfigLine& configLine);

	void SelfMerge();

	float GetConfigSpaceSize();

	void PrintConfigSpace();

	void TranslateConfigSpace(v2f trans);

	static bool CompareConfigLineLength(CConfigLine& line1, CConfigLine& line2);

	static void PrecomputeTable(const std::vector<CRoom>& vecRooms);

	static std::vector< std::vector<CConfigSpace> > m_precomputedTable;

	static bool m_flagPrecomputed;

private:

	std::vector<CConfigLine> m_vecConfigLine;
};

#endif // CONFIGSPACE_H
