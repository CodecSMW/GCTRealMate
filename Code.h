#ifndef GCTRM_COD_H
#define GCTRM_COD_H


#include <queue>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "PPCop.h"
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
public:
	aliasGroup replaceList, localReplaceList;
	list<labelSeek> labelFillList;
	string name;
	list<label> labelList;
	queue<PPCop> op;

	Code(string nameInit);
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
private:
	void ErrorContext(error errorType);
};

#endif