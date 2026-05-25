#ifndef GCTRM_COD_H
#define GCTRM_COD_H


#include <queue>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include "PPCop.h"
#include "utility.h"
#include "aliasGroup.h"

using namespace std;

class Code
{
private:
	enum errorNo { ERN_missingLabel, ERN_streamOverload, ERN_fileNotFound };
	struct error { errorNo errorID; string context; };
	struct label { string labelType; int opOffset; };
	struct labelSeek { string labelType; int opOffset; int insertMode; };
	queue<error> errorList;
	uint32_t gctPos;
public:
	aliasGroup replaceList, localReplaceList;
	list<labelSeek> labelFillList;
	string name;
	list<label> labelList;
	queue<PPCop> op;

	Code(string nameInit, uint32_t gctPosIn);
	~Code();

	bool Errors();
	void ShowName();
	void clearLocalMacros();
	void RequestLabel(string labelName, int currentOpOffset, int insertSize = 16);
	int NextLabelRequest();
	void ImplementLabel();
	void FoundError(errorNo errorID, string context);
	void findAliases(string& comparedString, bool& foundVal);
	string getMacro(string macroName, vector<string>& ArgOrig);
	uint32_t getLen();
	uint32_t getGctPos();
private:
	void ErrorContext(error errorType);
};

#endif