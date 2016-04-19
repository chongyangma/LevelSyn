//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-06-13
//  email:  chongyangm@gmail.com
//  info: class declaration of config file for level synthesis
// --------------------------------------------------------------

#ifndef LEVELCONFIG_H
#define LEVELCONFIG_H

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#endif
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>

class CLevelConfig
{
public:
	CLevelConfig();

	bool LoadFromSynConfig(std::string fileName, bool resetFlag = true);

	static std::string AddOutputPrefix(std::string str);

	static bool m_flagRandomness;
	static bool m_flagEnableTypeChange;
	static bool m_flagEnrichTemplates;
	static bool m_flagEqualPickProb;
	static bool m_flagDiscreteConnectFunc;
	static bool m_flagRandomPick;
	static bool m_flagNonOverlapContact;
	static bool m_flagSmallFaceFirst;
	static bool m_flagUseILS;
	static bool m_flagRandomWalk;
	static int m_numOfSolutionsToTrack;
	static int m_synMethod;
	static int m_targetNumOfSolutions;
	// For simulated annealing...
	static int m_saNumOfCycles;
	static int m_saNumOfTrials;
	static float m_saProb0;
	static float m_saProb1;
	static float m_deltaEscaling;
	// For layout energy calculation...
	static float m_sigmaCollide;
	static float m_sigmaContact;
	static float m_sigmaConnectivity;
	static float m_graphScaling; // For initialization
	static float m_roomScaling; // For rendering only
	static float m_stateDiffThresh;
	static float m_roomContactThresh;
	static std::string m_outputPrefix;

	static void DumpTimeAndDate(FILE* file);

	static void DumpTimeAndDate(std::ofstream& fout);

protected:
	virtual void ResetConfig();

	bool DumpToSynConfig();

	virtual void DumpParameters(FILE* file);

	virtual void UpdateOutputPrefix();

	void DumpStringParam(FILE* file, const char* param, const std::string str);

};

#endif //LEVELCONFIG_H
