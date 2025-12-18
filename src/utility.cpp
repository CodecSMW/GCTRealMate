#include "utility.h"
#include <stack>
#include <iostream>
#include <filesystem>

bool noCaseEquals(char lhs, char rhs)
{
    return std::tolower(lhs) == std::tolower(rhs);
}
bool caseEquals(char lhs, char rhs)
{
    return lhs == rhs;
}
bool equals(const std::string& lhs, const std::string& rhs)
{
    return (lhs.size() != rhs.size()) ? false : std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), caseEquals);
}
bool iequals(const std::string& lhs, const std::string& rhs)
{
    return (lhs.size() != rhs.size()) ? false : std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), noCaseEquals);
}
void erase_all(std::string& str, char criteria)
{
    str.erase(std::remove(str.begin(), str.end(), criteria), str.end());
}
void replace_all(std::string& str, const std::string& criteria, const std::string& replacement)
{
	const int criteriaSize = criteria.size();
	const int replacementSize = criteria.size();
	std::size_t findLoc = 0;
	while (findLoc != std::string::npos)
	{
		findLoc = str.find(criteria, findLoc);
		if (findLoc != std::string::npos)
		{
			str.erase(findLoc, criteriaSize);
			str.insert(findLoc, replacement);
			findLoc++;
		}
	}
}

std::filesystem::path attemptPathCaseRepair(const std::filesystem::path& pathIn)
{
	std::filesystem::path result = pathIn;
	if (!std::filesystem::exists(pathIn))
	{
		std::stack<std::string> pathUnits{};
		const std::filesystem::path::iterator beginItr = pathIn.begin();
		std::filesystem::path tempPath = pathIn;
		std::filesystem::path::iterator itr = pathIn.end();
		while (itr != beginItr)
		{
			itr--;
			tempPath.clear();
			pathUnits.push(itr->string());
			for (std::filesystem::path::iterator tempItr = beginItr; tempItr != itr; tempItr++) { tempPath /= *tempItr;	}
			if (std::filesystem::exists(tempPath)) break;
		}
		bool matchFound = 1;
		while (!pathUnits.empty() && matchFound)
		{
			matchFound = 0;
			std::string currUnit = pathUnits.top(); pathUnits.pop();
			std::filesystem::directory_entry currEntry;
			for (auto currEntry : std::filesystem::directory_iterator(tempPath))
			{
				std::filesystem::path::iterator unitName = currEntry.path().end(); unitName--;
				if ((pathUnits.empty() || currEntry.is_directory()) && iequals(currUnit, unitName->string()))
				{
					matchFound = 1;
					tempPath /= *unitName; 
					break;
				}
			}
		}
		if (matchFound)
		{
			result = tempPath;
		}
	}
	return result;
}

