/*
GCTRealMate


v0.9.005 Added interrupt operations and new scalar and PSA (i.e. RA_basic) types
v0.9.006 Added new launch argument by request where a prompt to close does not appear.
v0.9.007 Added support for eqv and fixed issues with ba and bla
v0.9.008 Added support for ps_div and fixed some other paired-single settings

*/

#include <filesystem>
#include <vector>
#include "compileGCT.h"

using namespace std;
bool provideTXT, provideLOG, preserveOld, fileCompare, GCTconvert, astUsage, pressKeyClose, repairPathCase;
ofstream logFile, codeset;

 int main(int argc, char* argv[])
{
	compileGCT compile;
	::provideTXT = false;
	::provideLOG = false;
	::preserveOld = false;
	::fileCompare = false;
	::GCTconvert = false;
	::astUsage = false;
	::pressKeyClose = true;
	::repairPathCase = false;
	if (argc <= 1)
	{
		cout << "GCTRealMate Alpha 0.2" << endl;
		cout << "How to use GCTRealMate." << endl;
		cout << "Drag the asm or txt file onto the program." << endl;
		cout << "It will put out a GCT with the filename on the first line of the file or default to RSBE01.GCT, otherwise." << endl;
		cout << "Comments can be written with # or \\ \\, hooks can be written with HOOK and codes written directly can use CODE." << endl;
		cout << "Types uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t and float are supported." << endl;
	}
	else
	{
		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-')
			{
				switch (argv[i][1])
				{
				case 'g': case 'G': ::GCTconvert = true;
					//provides a codeset that GCTconvert can use.
				case '*': ::astUsage = true;
					//if a codeset is created, it will set the code to have * for easy insertion to other programs
				case 't': case 'T': 
					::provideTXT = true; 
					if(!codeset.is_open())
						::codeset.open("codeset.txt", ofstream::trunc);  
					break;
					//creates a text codeset
				case 'l': case 'L': 
					::provideLOG = true; 
					if (!logFile.is_open())
						::logFile.open("log.txt", ofstream::trunc);  
					break;
					//creates a log
				case 'p': case 'P': ::preserveOld = true; break;
				case 'c': case 'C': ::fileCompare = true; break;
				case 'q': case 'Q': ::pressKeyClose = false; break;
				case 'r': case 'R': ::repairPathCase = true; break;
				default: break;
				};
			}
			else
			{
				compile.compile(std::filesystem::absolute(argv[i]));
			}
		}
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