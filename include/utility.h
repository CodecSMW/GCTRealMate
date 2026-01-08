#ifndef GCTRM_UTIL_H
#define GCTRM_UTIL_H

#include <string>
#include <cctype>
#include <format>
#include <algorithm>
#include <filesystem>
#include <vector>

using namespace std::literals::string_view_literals;

// Performs case-sensitive comparison between lhs and rhs.
bool equals(std::string_view lhs, std::string_view rhs);
// Performs case-insensitive comparison between lhs and rhs.
bool iequals(std::string_view lhs, std::string_view rhs);
// Removes all instances of criteria from str, and shrinks it to reflect its new length.
void erase_all(std::string& str, char criteria);
// Checks (with case-sensitivity) whether lhs begins with rhs.
bool begins_with(std::string_view lhs, std::string_view rhs);
// Checks (without case-sensitivity) whether lhs begins with rhs.
bool ibegins_with(std::string_view lhs, std::string_view rhs);
// Replaces all instances of criteria from str with the provided replacement.
void replace_all(std::string& str, const std::string& criteria, const std::string& replacement);
// Attempts to repair incorrect capitalization in the provided path.
// Returns the repaired path if successful, or a copy of pathIn if not. 
std::filesystem::path attemptPathCaseRepair(const std::filesystem::path& pathIn);
// Tokenizes the provided string.
std::vector<std::string_view> tokenizeString(std::string_view str, std::string_view whitespaceDelimiters, std::string_view symbolDelimiters, bool respectQuoteBoundaries);

#endif
