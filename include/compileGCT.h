#ifndef GCTRM_CMP_H
#define GCTRM_CMP_H

#include <iostream>
#include <fstream>
#include <queue>
#include <stack>
#include <vector>
#include <string>
#include "Code.h"
#include "utility.h"
#include "aliasGroup.h"

using namespace std;
extern bool provideTXT, provideLOG, preserveOld, fileCompare, GCTconvert, astUsage, repairPathCase;
extern ofstream logFile, codeset;

#define ISIT(stringCheck) isString(line,stringCheck,tempOff)
#define ISIT2(stringCheck) isString(line,stringCheck)
#define MAXSTRM numeric_limits<streamsize>::max()
#define IGNORE(character) ignore(MAXSTRM,character)
#define STOI_ (uint32_t)stoul(line.substr(tempOff),nullptr,0)
#define _STOI(line1) stoul(line1,nullptr,0)

class compileGCT
{
	struct label { string labelType; int opOffset; };	
	union streamAmbig { ifstream* fileStream; stringstream* macroStream; };
	struct streamPack { streamAmbig* streamUnk; bool isMacro; std::filesystem::path filepath; };
	enum PSAtype {  IC_Basic = 0x00000000, IC_Bit = 0x01000000, IC_Float = 0x02000000, 
					LA_Basic = 0x10000000, LA_Bit = 0x11000000, LA_Float = 0x12000000,
					RA_Basic = 0x20000000, RA_Bit = 0x21000000, RA_Float = 0x22000000};
	enum textMode { seekEnabledCode, geckoCodeMode, opCodeMode, codeMode, finishOpCodeMode };
	enum hookMode { notHooked, hookCode, hookHook, hookPulse, hookPSA };
	ifstream GCTtext;
	ofstream GCTgct;
	string gctName, gctTemp;
	PPCop tempOp;
	queue<Code> geckoOps;
	uint8_t tempVal[4];
	uint8_t header[8] = { 0x00, 0xD0, 0xC0, 0xDE, 0x00, 0xD0, 0xC0, 0xDE };
	uint8_t footer[8] = { 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int textSize, functionCount, GCTbytes;
	bool breakLine, error;
	uint8_t charPair2Hex(string& line);
	uint32_t addressConvert(string line);
	bool handleAddressSet(string& line, Code& alias);
	void handleRaw(string& line, queue<uint8_t>& content);
	void dumpRawBytes(queue<uint8_t>& content);
	int getArraySize(string& line, int& tempOff), seekLabelDistance(string labelName, int offsetOp, int maxSize, vector<label>& labels);
	bool isJustHex(string& line), isCodeName(string& line), hasAddress(string& line), hasType(string& line);
	bool isString(string& line, string comp, int& offset), isString(string& line, string comp);
	bool isOp(string& line);
	int convCharToHex(char hexchar);
	void processLines(const char* name, queue<Code>& geckoOps, bool& error);
	string replaceExtension(const char* name, string extension, bool& error);
	void parseLine(string& temp, string& tempLine, textMode& mode, ifstream* currentStream,
		vector<label>& labels, queue<uint8_t>& rawBytes, queue<PPCop>& operations),
		 parseLine(string& temp, string& tempLine, textMode& mode, stringstream* currentStream,
			vector<label>& labels, queue<uint8_t>& rawBytes, queue<PPCop>& operations);
	void compileAlias(string line, Code& alias, bool isLocal);
	void compileMacro(string line, aliasGroup& replaceList, ifstream* currentStream);
	void openMacro(string& line, Code& macroContain, stringstream*& currentStream);
public:
	compileGCT();
	void compile(const std::filesystem::path& name);
};

#endif