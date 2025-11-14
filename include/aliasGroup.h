#ifndef GCTRM_ASG_H
#define GCTRM_ASG_H

#include <vector>
#include "macro.h"

class aliasGroup
{
public:
	struct alias { std::string aliasName; std::string aliasContent; };
	std::vector<alias> aliasList;
	std::vector<macro> macroList;
	~aliasGroup()
	{
		aliasList.clear();
		macroList.clear();
	}
};

#endif