#include "utility.h"
#include <cctype>
#include <cstddef>
#include <stack>
#include <iostream>
#include <filesystem>
#include <string_view>

bool noCaseEquals(char lhs, char rhs)
{
    return std::tolower(lhs) == std::tolower(rhs);
}
bool caseEquals(char lhs, char rhs)
{
    return lhs == rhs;
}
bool equals(std::string_view lhs, std::string_view rhs)
{
    return (lhs.size() != rhs.size()) ? false : std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), caseEquals);
}
bool iequals(std::string_view lhs, std::string_view rhs)
{
    return (lhs.size() != rhs.size()) ? false : std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), noCaseEquals);
}
bool begins_with(std::string_view lhs, std::string_view rhs)
{
	return equals(lhs.substr(0, rhs.size()), rhs);
}
bool ibegins_with(std::string_view lhs, std::string_view rhs)
{
	return iequals(lhs.substr(0, rhs.size()), rhs);
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
std::vector<std::string_view> tokenizeString(std::string_view str, std::string_view whitespaceDelimiters, std::string_view symbolDelimiters, bool respectQuoteBoundaries)
{
	std::vector<std::string_view> result{};
	
	std::size_t currTokenBegin = str.find_first_not_of(whitespaceDelimiters);
	while (currTokenBegin < str.size())
	{
		std::size_t tokenLength = 0;
		std::size_t delimLength = 0;
		
		std::size_t nextSymbolDelim = str.find_first_of(symbolDelimiters, currTokenBegin);
		std::size_t nextWhitespaceDelim = str.find_first_of(whitespaceDelimiters, currTokenBegin);
		
		std::size_t nextDelim = std::min(nextSymbolDelim, nextWhitespaceDelim);
		if (nextDelim > currTokenBegin)
		{
			if (respectQuoteBoundaries && str[currTokenBegin] == '\"')
			{
				std::size_t closingQuoteLoc = currTokenBegin;
				do 
				{
					closingQuoteLoc = str.find_first_of('\"', closingQuoteLoc + 1);
				} while (closingQuoteLoc != std::string::npos && str[closingQuoteLoc - 1] == '\\');

				if (closingQuoteLoc != std::string::npos)
				{
					tokenLength = 2 + result.emplace_back(str.substr(currTokenBegin + 1, closingQuoteLoc - (currTokenBegin + 1))).size();
				}
			}
			if (tokenLength == 0)
			{
				tokenLength = result.emplace_back(str.substr(currTokenBegin, nextDelim - currTokenBegin)).size();
			}
		}
		
		if (nextSymbolDelim < nextWhitespaceDelim)
		{
			delimLength = result.emplace_back(str.substr(nextDelim, str.find_first_not_of(symbolDelimiters, nextDelim) - nextDelim)).size();
		}
		else if (nextWhitespaceDelim < nextSymbolDelim)
		{
			result.emplace_back(str.substr(nextWhitespaceDelim, 0));
			delimLength = str.find_first_not_of(whitespaceDelimiters, nextWhitespaceDelim) - nextWhitespaceDelim;
		}

		currTokenBegin += tokenLength + delimLength;
	}
	return result;
}
