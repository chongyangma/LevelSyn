//  Copyright (c) www.chongyangma.com
//
//  author: Chongyang Ma - 2013-02-28
//  email:  chongyangm@gmail.com
//  info: main function of command line tool for level synthesis
// --------------------------------------------------------------

#include <algorithm>
#include <iostream>
#include <ctime>
#include <cmath>
#include <cstring>

#include "PlanarGraph.h"
#include "RoomTemplates.h"
#include "LevelSynth.h"
#include "LevelTimer.h"

void usage(char* argv[])
{
	std::cout << "Usage: " << argv[0] << " graph.xml templates.xml config.txt [target_solution_number]\n";
}

int main(int argc, char **argv)
{
	if ( argc < 4 )
	{
		usage(argv);
		return -1;
	}

	CPlanarGraph planar_graph;
	CRoomTemplates room_templates;
	CLevelConfig level_config;
	CLevelSynth level_synthesizer;
	std::string input_graph_name = argv[1];
	std::string input_template_name = argv[2];
	level_config.LoadFromSynConfig(argv[3]);
	if ( argc > 4 )
	{
		CLevelConfig::m_targetNumOfSolutions = atoi(argv[4]);
	}

	planar_graph.LoadGraphFromXML(input_graph_name.c_str());
	planar_graph.SaveGraphAsXML(CLevelConfig::AddOutputPrefix(input_graph_name).c_str());
	room_templates.LoadTemplatesFromXML(input_template_name.c_str());
	room_templates.SaveTemplatesAsXML(CLevelConfig::AddOutputPrefix(input_template_name).c_str());

	if ( CLevelConfig::m_flagRandomness )
	{
		srand((unsigned int)time(0));
	}
	if ( CLevelConfig::m_flagEnrichTemplates == true )
	{
		room_templates.EnrichByRotating180Degrees();
	}
	CConfigSpace::PrecomputeTable(room_templates.GetRooms());

	CLevelTimer timer;
	double oldTime = timer.GetTime();
	level_synthesizer.SetGraphAndTemplates(&planar_graph, &room_templates);
	double elapseTime = timer.GetElapsedTime(oldTime);

	std::ofstream fout;
	fout.open("log.txt", std::ios_base::app);
	CLevelConfig::DumpTimeAndDate(fout);
	fout << "Have found " << level_synthesizer.GetSolutionCount() << " solution(s) within " << elapseTime << " seconds.\n";
	std::cout << "Have found " << level_synthesizer.GetSolutionCount() << " solution(s) within " << elapseTime << " seconds.\n";

	return (0);
}
