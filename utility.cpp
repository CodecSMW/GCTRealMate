#include "utility.h"

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
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), caseEquals);
}
bool iequals(const std::string& lhs, const std::string& rhs)
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), noCaseEquals);
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
		}
	}
}
