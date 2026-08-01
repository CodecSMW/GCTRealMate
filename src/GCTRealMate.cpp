/*
GCTRealMate


v0.9.005 Added interrupt operations and new scalar and PSA (i.e. RA_basic) types
v0.9.006 Added new launch argument by request where a prompt to close does not appear.
v0.9.007 Added support for eqv and fixed issues with ba and bla
v0.9.008 Added support for ps_div and fixed some other paired-single settings

*/

#include <climits>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "_cmdArgs.h"
#include "compileGCT.h"

using namespace std;
ofstream logFile, codeset;

void setFlagState(std::string_view& argsView, bool& flagIn, bool defaultStateIn)
{
	if (argsView.size() > 2 && argsView[1] == ':')
	{
		switch (argsView[2])
		{
			case '0': { flagIn = false; break; }
			case '1': { flagIn = true; break; }
			default: { flagIn = defaultStateIn; break; }
		}
	}
}
void parseCmdLineArgs(std::string_view argsView)
{
	while (!argsView.empty())
	{
		std::size_t nextArgPos = argsView.find('-');
		if (nextArgPos == std::string::npos || (nextArgPos + 1) >= argsView.size()) break;
		argsView.remove_prefix(nextArgPos + 1);
		switch (tolower(argsView.front()))
		{
			//provides a codeset that GCTconvert can use. Note: Falls through to below.
			case 'g': { setFlagState(argsView, ::GCTconvert, true); }
			//if a codeset is created, it will set the code to have * for easy insertion to other programs. Note: Falls through to below.
			case '*': { setFlagState(argsView, ::astUsage, true); }
			//creates a text codeset
			case 't':
			{
				setFlagState(argsView, ::provideTXT, true);
				if(!codeset.is_open()) ::codeset.open("codeset.txt", ofstream::trunc);  
				break;
			}
			//creates a log
			case 'l':
			{

				setFlagState(argsView, ::provideLOG, true);
				if (!logFile.is_open()) ::logFile.open("log.txt", ofstream::trunc);
				break;
			}
			case 'p': { setFlagState(argsView, ::preserveOld, true); break; }
			case 'c': { setFlagState(argsView, ::fileCompare, true); break; }
			case 'q': { setFlagState(argsView, ::pressKeyClose, false); break; }
			case 'r': { setFlagState(argsView, ::repairPathCase, true); break; }
			case 'i': { setFlagState(argsView, ::ignoreSettingsFile, true); break; }
			case 'a': { setFlagState(argsView, ::doInlineBAConv, true); break; }
			case 'b':
			{
				argsView.remove_prefix(argsView.find_first_not_of(" \t", 1));
				size_t addrBeginIdx = 0;
				if (argsView.starts_with('$'))
				{
					addrBeginIdx = 1;
				}
				else if (argsView.starts_with("0x"))
				{
					addrBeginIdx = 2;
				}
				std::size_t lenOut = SIZE_MAX;
				::codesetBaseAddress = stoul(argsView.substr(addrBeginIdx).data(), &lenOut, 16);
				break;
			}
		}
	}
}

bool parseSettingsFile(const std::filesystem::path& settingsPath, const std::filesystem::path& codesetPath)
{
	bool result = 0;

	if (std::filesystem::is_regular_file(settingsPath))
	{
		std::string currLine;
		std::ifstream streamIn(settingsPath);
		while (!result && std::getline(streamIn, currLine))
		{
			std::string codesetFilename = codesetPath.filename().string();
			if (ibegins_with(currLine, codesetFilename))
			{
				std::string_view argsView(currLine.data() + codesetFilename.size());
				std::size_t colonPos = argsView.find(':');
				if (colonPos != std::string::npos)
				{
					parseCmdLineArgs(argsView.substr(colonPos + 1));
				}
				result = 1;
			}
		}
	}

	return result;
}

 int main(int argc, char* argv[])
{
	::provideTXT = false;
	::provideLOG = false;
	::preserveOld = false;
	::fileCompare = false;
	::GCTconvert = false;
	::astUsage = false;
	::pressKeyClose = true;
	::repairPathCase = false;
	::doInlineBAConv = false;
	::codesetBaseAddress = UINT_MAX;
	::ignoreSettingsFile = false;
	
	cout << "GCTRealMate v0.2.5" << endl;
	if (argc <= 1)
	{
		cout << "How to use GCTRealMate." << endl;
		cout << "Drag the asm or txt file onto the program." << endl;
		cout << "It will put out a GCT with the filename on the first line of the file or default to RSBE01.GCT, otherwise." << endl;
		cout << "Comments can be written with # or \\\\, hooks can be written with HOOK and codes written directly can use CODE." << endl;
		cout << "Types uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t and float are supported." << endl;
	}
	else
	{
		std::string combinedArgStr;
		combinedArgStr.reserve(0x40);
		uint32_t checksToSkip = 0;
		bool compiledGCT = 0;
		for (std::size_t itr = 1; itr < argc; itr++)
		{
			if ((--checksToSkip == 0) || argv[itr][0] == '-')
			{
				combinedArgStr += argv[itr]; combinedArgStr += " ";
				if (argv[itr][1] == 'b')
				{
					checksToSkip = 1;
				}
			}
			else if (!compiledGCT)
			{
				parseCmdLineArgs(combinedArgStr);
				combinedArgStr.clear();
				std::filesystem::path selfPath(std::filesystem::absolute(argv[0]));
				std::filesystem::path settingsPath(selfPath.replace_extension(".ini"));
				std::filesystem::path absolutePath(std::filesystem::absolute(argv[itr]));
				if (!ignoreSettingsFile && std::filesystem::is_regular_file(settingsPath))
				{
					parseSettingsFile(settingsPath, absolutePath);
				}
				compileGCT().compile(absolutePath);
				compiledGCT = 1;
			}
		}
		parseCmdLineArgs(combinedArgStr);
	}

	if (::logFile.is_open())
		::logFile.close();
	if (::codeset.is_open())
		::codeset.close();
	if (::pressKeyClose)
	{
		cout << "Press enter to close.";
		cin.ignore();
	}
}