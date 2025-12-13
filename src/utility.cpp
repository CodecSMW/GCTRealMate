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

// Try exact path first, then search the containing directory case-insensitively.
std::optional<std::filesystem::path> findFileIgnoringCase(const std::filesystem::path& filePath) {
    if (filePath.empty()) {
        return std::nullopt;
    }

    if (std::filesystem::exists(filePath)) {
        return filePath;
    }

    // Resolve directory and target filename to compare.
    const std::filesystem::path dir = filePath.has_parent_path() ? filePath.parent_path() : std::filesystem::current_path();
    const std::string targetName = filePath.filename().string();

    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return std::nullopt;
    }

    // Scan entries and return the first case-insensitive match.
    for (const auto& entry : std::filesystem::directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied)) {
        if (iequals(entry.path().filename().string(), targetName)) {
            return entry.path();
        }
    }

    return std::nullopt;
}
