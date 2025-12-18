#ifndef GCTRM_UTIL_H
#define GCTRM_UTIL_H

#include <string>
#include <cctype>
#include <format>
#include <algorithm>
#include <filesystem>

// Performs case-sensitive comparison between lhs and rhs.
bool equals(const std::string& lhs, const std::string& rhs);
// Performs case-insensitive comparison between lhs and rhs.
bool iequals(const std::string& lhs, const std::string& rhs);
// Removes all instances of criteria from str, and shrinks it to reflect its new length.
void erase_all(std::string& str, char criteria);
// Replaces all instances of criteria from str with the provided replacement.
void replace_all(std::string& str, const std::string& criteria, const std::string& replacement);
// Attempts to repair incorrect capitalization in the provided path.
// Returns the repaired path if successful, or a copy of pathIn if not. 
std::filesystem::path attemptPathCaseRepair(const std::filesystem::path& pathIn);

#endif
