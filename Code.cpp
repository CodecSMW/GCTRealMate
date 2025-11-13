#include "Code.h"

bool Code::Errors()
{
	if (errorList.empty())
		return false;
	cout << "Error in " << name << "!!" << endl;
	while (!errorList.empty())
	{
		ErrorContext(errorList.front());
		errorList.pop();
	}
	return true;
}

void Code::clearLocalMacros()
{
	localReplaceList.aliasList.clear();
	localReplaceList.macroList.clear();
}
void Code::ErrorContext(error errorType)
{
	/*
	switch (errorType.errorID)
	{
	default:
	};
	*/
}

void Code::RequestLabel(string labelName, int currentOpOffset, int insertSize)
{
	labelFillList.emplace_back();
	labelFillList.back().labelType = labelName;
	labelFillList.back().opOffset = currentOpOffset;
	labelFillList.back().insertMode = insertSize;
}
int Code::NextLabelRequest()
{
	if (!labelFillList.empty())
		return labelFillList.front().opOffset;
	else
		return -1;
}
void Code::ImplementLabel()
{
	for (list<label>::iterator i = labelList.begin(); i != labelList.end(); i++)
	{
		if (boost::iequals(labelFillList.front().labelType, i->labelType))
		{
			switch (labelFillList.front().insertMode)
			{
			case 1:
				op.front().enforceOffset((i->opOffset - labelFillList.front().opOffset) * 0.5 - 1); break;
			default:
				op.front().enforceOffset((i->opOffset - labelFillList.front().opOffset) * 4 - 8); break;
			}
			
		}
	}
}
void Code::findAliases(string& comparedString, bool& foundVal)
{
	for (int i = 0; i < localReplaceList.aliasList.size(); i++)
	{
		if (boost::iequals(comparedString, localReplaceList.aliasList.at(i).aliasName))
		{
			foundVal = true;
			comparedString = localReplaceList.aliasList.at(i).aliasContent;
			return;
		}
	}
	for (int i = 0; i < replaceList.aliasList.size(); i++)
	{
		if (boost::iequals(comparedString, replaceList.aliasList.at(i).aliasName))
		{
			foundVal = true;
			comparedString = replaceList.aliasList.at(i).aliasContent;
			return;
		}
	}
}

void Code::ShowName()
{
	std::cout << name << endl;
}

void Code::FoundError(errorNo errorID, string context)
{
	errorList.emplace(); 
	errorList.front().errorID = errorID; 
	errorList.front().context = context;
}

string Code::getMacro(string macroName, vector<string>& ArgOrig)
{
	for (int i = 0; i < localReplaceList.macroList.size(); i++)
	{
		if (boost::equals(macroName, localReplaceList.macroList.at(i).name))
		{
			for (int j = 0; j < localReplaceList.macroList.at(i).arguments.size(); j++)
			{
				ArgOrig.emplace_back(localReplaceList.macroList.at(i).arguments.at(j));
			}
			return localReplaceList.macroList.at(i).content;
		}
	}

	for (int i = 0; i < replaceList.macroList.size(); i++)
	{
		if (boost::equals(macroName, replaceList.macroList.at(i).name))
		{
			for (int j = 0; j < replaceList.macroList.at(i).arguments.size(); j++)
			{
				ArgOrig.emplace_back(replaceList.macroList.at(i).arguments.at(j));
			}
			return replaceList.macroList.at(i).content;
		}
	}

	return "";
}

Code::Code(string nameInit)
{
	name = nameInit;
}

Code::~Code()
{
	while (!op.empty())
		op.pop();
	labelFillList.clear();
	labelList.clear();
	while (!errorList.empty())
		errorList.pop();
	replaceList.aliasList.clear();
	replaceList.macroList.clear();
	clearLocalMacros();
}