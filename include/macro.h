#pragma once

#include <string>
#include <vector>

class macro
{
public:
	std::string name;
	std::vector<std::string> arguments;
	std::string content;

	macro()
	{
		name = "";
		content = "";
	}
	~macro()
	{
		arguments.clear();
	}
};