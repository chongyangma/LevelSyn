#include "LevelConfig.h"

#ifndef WIN32
    #define MAX_PATH 260
#endif

bool CLevelConfig::m_flagRandomness = false;
bool CLevelConfig::m_flagEnableTypeChange = true;
bool CLevelConfig::m_flagEnrichTemplates = false;
bool CLevelConfig::m_flagEqualPickProb = true;
bool CLevelConfig::m_flagDiscreteConnectFunc = true;
bool CLevelConfig::m_flagRandomPick = true;
bool CLevelConfig::m_flagNonOverlapContact = false;
bool CLevelConfig::m_flagSmallFaceFirst = false;
bool CLevelConfig::m_flagUseILS = false;
bool CLevelConfig::m_flagRandomWalk = false;
int CLevelConfig::m_numOfSolutionsToTrack = 10;
int CLevelConfig::m_synMethod = 0;
int CLevelConfig::m_saNumOfCycles = 1000;
int CLevelConfig::m_saNumOfTrials = 1000;
int CLevelConfig::m_targetNumOfSolutions = 100;
float CLevelConfig::m_saProb0 = 0.001f;
float CLevelConfig::m_saProb1 = 0.7f;
float CLevelConfig::m_deltaEscaling = 1.0f;
float CLevelConfig::m_sigmaCollide = 50.f;
float CLevelConfig::m_sigmaContact = 1.f;
float CLevelConfig::m_sigmaConnectivity = 2.f;
float CLevelConfig::m_graphScaling = 1.f;
float CLevelConfig::m_roomScaling = 0.9f;
float CLevelConfig::m_stateDiffThresh = 0.f;
float CLevelConfig::m_roomContactThresh = 1e-6f;
std::string CLevelConfig::m_outputPrefix;

CLevelConfig::CLevelConfig()
{
}

bool CLevelConfig::LoadFromSynConfig(std::string fileName, bool resetFlag /* = true */)
{
    std::ifstream fin(fileName.c_str());
    if (fin.fail() == true)
    {
        std::cout << "Failed to load config parameters from config file " << fileName << "!\n";
        return false;
    }
    std::string param;
    while (fin >> param)
    {
        if (param == std::string("FLAG_RANDOMNESS"))
        {
            fin >> m_flagRandomness;
        }
        else if (param == std::string("FLAG_ENABLE_TYPE_CHANGE"))
        {
            fin >> m_flagEnableTypeChange;
        }
        else if (param == std::string("FLAG_ENRICH_TEMPLATES"))
        {
            fin >> m_flagEnrichTemplates;
        }
        else if (param == std::string("FLAG_EQUAL_PICK_PROBABILITY"))
        {
            fin >> m_flagEqualPickProb;
        }
        else if (param == std::string("FLAG_DISCRETE_CONNECTIVITY_FUNCTION"))
        {
            fin >> m_flagDiscreteConnectFunc;
        }
        else if (param == std::string("FLAG_RANDOM_PICK"))
        {
            fin >> m_flagRandomPick;
        }
        else if (param == std::string("FLAG_NON_OVERLAP_CONTACT"))
        {
            fin >> m_flagNonOverlapContact;
        }
        else if (param == std::string("FLAG_SMALL_FACE_FIRST"))
        {
            fin >> m_flagSmallFaceFirst;
        }
        else if (param == std::string("FLAG_USE_ILS"))
        {
            fin >> m_flagUseILS;
        }
        else if (param == std::string("FLAG_RANDOM_WALK"))
        {
            fin >> m_flagRandomWalk;
        }
        else if (param == std::string("NUMBER_OF_SOLUTIONS_TO_TRACK"))
        {
            fin >> m_numOfSolutionsToTrack;
        }
        else if (param == std::string("SYNTHESIS_METHOD"))
        {
            fin >> m_synMethod;
        }
        else if (param == std::string("SA_NUM_OF_CYCLES"))
        {
            fin >> m_saNumOfCycles;
        }
        else if (param == std::string("SA_NUM_OF_TRIALS"))
        {
            fin >> m_saNumOfTrials;
        }
        else if (param == std::string("SA_PROB_0"))
        {
            fin >> m_saProb0;
        }
        else if (param == std::string("SA_PROB_1"))
        {
            fin >> m_saProb1;
        }
        else if (param == std::string("DELTA_E_SCALING"))
        {
            fin >> m_deltaEscaling;
        }
        else if (param == std::string("SIGMA_COLLIDE"))
        {
            fin >> m_sigmaCollide;
        }
        else if (param == std::string("SIGMA_CONTACT"))
        {
            fin >> m_sigmaContact;
        }
        else if (param == std::string("SIGMA_CONNECTIVITY"))
        {
            fin >> m_sigmaConnectivity;
        }
        else if (param == std::string("GRAPH_SCALING"))
        {
            fin >> m_graphScaling;
        }
        else if (param == std::string("ROOM_SCALING"))
        {
            fin >> m_roomScaling;
        }
        else if (param == std::string("STATE_DIFFERENCE_THRESHOLD"))
        {
            fin >> m_stateDiffThresh;
        }
        else if (param == std::string("ROOM_CONTACT_THRESHOLD"))
        {
            fin >> m_roomContactThresh;
        }
        else if (param == std::string("OUTPUT_PREFIX"))
        {
            fin >> m_outputPrefix;
        }
    }
    if (resetFlag == false)
    {
        return true;
    }
    ResetConfig();

    return true;
}

std::string CLevelConfig::AddOutputPrefix(std::string str)
{
    if (m_outputPrefix.empty() == true)
    {
        return str;
    }
    std::ostringstream oss;
    oss << m_outputPrefix << str;
    std::string strNew = oss.str();
    return strNew;
}

void CLevelConfig::ResetConfig()
{
    if (m_flagRandomness == true)
    {
        srand((unsigned int)time(0));
    }
    if (m_outputPrefix.empty() == false)
    {
        UpdateOutputPrefix();
    }
    DumpToSynConfig();
}

bool CLevelConfig::DumpToSynConfig()
{
    std::ostringstream oss;
    oss << m_outputPrefix << "SynConfig.txt";
    std::string fileName = oss.str();
    FILE* file;
    file = fopen(fileName.c_str(), "w");
    if (!file)
    {
        std::cout << "Failed to dump parameters into config file " << fileName << "!\n";
        return false;
    }
    DumpTimeAndDate(file);
    DumpParameters(file);
    fclose(file);

    return true;
}

void CLevelConfig::DumpTimeAndDate(FILE* file)
{
    time_t myTime = time(NULL);
    tm* ptrTime = localtime(&myTime);
    fprintf(file, "%02d:%02d:%02d ", ptrTime->tm_hour, ptrTime->tm_min, ptrTime->tm_sec);
    fprintf(file, "%02d/%02d/%04d\n\n", ptrTime->tm_mon + 1, ptrTime->tm_mday, ptrTime->tm_year + 1900);
}

void CLevelConfig::DumpTimeAndDate(std::ofstream& fout)
{
    time_t myTime = time(NULL);
    tm* ptrTime = localtime(&myTime);
    char str[1000];
    sprintf(str, "%02d:%02d:%02d ", ptrTime->tm_hour, ptrTime->tm_min, ptrTime->tm_sec);
    fout << str;
    sprintf(str, "%02d/%02d/%04d", ptrTime->tm_mon + 1, ptrTime->tm_mday, ptrTime->tm_year + 1900);
    fout << str << std::endl;
}

void CLevelConfig::DumpParameters(FILE* file)
{
    fprintf(file, "%s\t%d\n", "FLAG_RANDOMNESS", m_flagRandomness);
    fprintf(file, "%s\t%d\n", "FLAG_ENABLE_TYPE_CHANGE", m_flagEnableTypeChange);
    fprintf(file, "%s\t%d\n", "FLAG_ENRICH_TEMPLATES", m_flagEnrichTemplates);
    fprintf(file, "%s\t%d\n", "FLAG_EQUAL_PICK_PROBABILITY", m_flagEqualPickProb);
    fprintf(file, "%s\t%d\n", "FLAG_DISCRETE_CONNECTIVITY_FUNCTION", m_flagDiscreteConnectFunc);
    fprintf(file, "%s\t%d\n", "FLAG_RANDOM_PICK", m_flagRandomPick);
    fprintf(file, "%s\t%d\n", "FLAG_NON_OVERLAP_CONTACT", m_flagNonOverlapContact);
    fprintf(file, "%s\t%d\n", "FLAG_SMALL_FACE_FIRST", m_flagSmallFaceFirst);
    fprintf(file, "%s\t%d\n", "FLAG_USE_ILS", m_flagUseILS);
    fprintf(file, "%s\t%d\n", "FLAG_RANDOM_WALK", m_flagRandomWalk);
    fprintf(file, "%s\t%d\n", "NUMBER_OF_SOLUTIONS_TO_TRACK", m_numOfSolutionsToTrack);
    fprintf(file, "%s\t%d\n", "SYNTHESIS_METHOD", m_synMethod);
    fprintf(file, "%s\t%d\n", "SA_NUM_OF_CYCLES", m_saNumOfCycles);
    fprintf(file, "%s\t%d\n", "SA_NUM_OF_TRIALS", m_saNumOfTrials);
    fprintf(file, "%s\t%f\n", "SA_PROB_0", m_saProb0);
    fprintf(file, "%s\t%f\n", "SA_PROB_1", m_saProb1);
    fprintf(file, "%s\t%f\n", "DELTA_E_SCALING", m_deltaEscaling);
    fprintf(file, "%s\t%f\n", "SIGMA_COLLIDE", m_sigmaCollide);
    fprintf(file, "%s\t%f\n", "SIGMA_CONTACT", m_sigmaContact);
    fprintf(file, "%s\t%f\n", "SIGMA_CONNECTIVITY", m_sigmaConnectivity);
    fprintf(file, "%s\t%f\n", "GRAPH_SCALING", m_graphScaling);
    fprintf(file, "%s\t%f\n", "ROOM_SCALING", m_roomScaling);
    fprintf(file, "%s\t%f\n", "STATE_DIFFERENCE_THRESHOLD", m_stateDiffThresh);
    fprintf(file, "%s\t%f\n", "ROOM_CONTACT_THRESHOLD", m_roomContactThresh);
    DumpStringParam(file, "OUTPUT_PREFIX", m_outputPrefix);
}

void CLevelConfig::UpdateOutputPrefix()
{
#ifdef WIN32
    if (m_outputPrefix[m_outputPrefix.size() - 1] != '\\')
    {
        m_outputPrefix = m_outputPrefix + std::string("\\");
    }
#else
    while (m_outputPrefix.empty() == false && isdigit(m_outputPrefix[m_outputPrefix.size() - 1]) == 0)
    {
        m_outputPrefix = m_outputPrefix.substr(0, m_outputPrefix.size() - 1);
    }
    if (m_outputPrefix[m_outputPrefix.size() - 1] != '/')
    {
        m_outputPrefix = m_outputPrefix + std::string("/");
    }
#endif
    std::ostringstream oss;
#ifdef WIN32
    bool flag = CreateDirectoryA(m_outputPrefix.c_str(), NULL);
    const int numLength = 2;
    while (flag == false && m_outputPrefix.size() >= 2)
    {
        std::string subStr = m_outputPrefix.substr(m_outputPrefix.length() - numLength - 1, 2);
        int num = atoi(subStr.c_str()) + 1;
        char numChar[MAX_PATH];
        sprintf_s(numChar, "%02d", num);
        std::ostringstream oss;
        oss << m_outputPrefix.substr(0, m_outputPrefix.length() - numLength - 1) << numChar << "\\";
        m_outputPrefix = oss.str();
        flag = CreateDirectoryA(m_outputPrefix.c_str(), NULL);
    }
#else
    std::string outputFolder = m_outputPrefix.substr(0, m_outputPrefix.size() - 1);
    int flag = mkdir(outputFolder.c_str(), 0777);
    const int numLength = 2;
    while (flag == -1 && m_outputPrefix.size() >= 2)
    {
        std::string subStr = m_outputPrefix.substr(m_outputPrefix.length() - numLength - 1, 2);
        int num = atoi(subStr.c_str()) + 1;
        char numChar[MAX_PATH];
        sprintf(numChar, "%02d", num);
        std::ostringstream oss;
        oss << m_outputPrefix.substr(0, m_outputPrefix.length() - numLength - 1) << numChar << "/";
        m_outputPrefix = oss.str();
        outputFolder = m_outputPrefix.substr(0, m_outputPrefix.size() - 1);
        flag = mkdir(outputFolder.c_str(), 0777);
    }
#endif
    std::cout << "Generating results into the directory: " << m_outputPrefix.c_str() << "...\n";
}

void CLevelConfig::DumpStringParam(FILE* file, const char* param, const std::string& str)
{
    if (str.empty() != true)
    {
        fprintf(file, "%s\t%s\n", param, str.c_str());
    }
    else
    {
        //fprintf(file, "%s\t%s\n", param, "NULL");
    }
}
