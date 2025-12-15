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

std::optional<std::filesystem::path> findFileIgnoringCase(const std::filesystem::path& filePath) {
    // Fast-path: reject empty input and accept already-existing paths.
    if (filePath.empty()) {
        return std::nullopt;
    }

    if (std::filesystem::exists(filePath)) {
        return filePath;
    }

    // Search within each directory level for a case-insensitive match.
    auto findMatchingChild = [](const std::filesystem::path& dir, const std::filesystem::path& target) -> std::optional<std::filesystem::path> {
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
            return std::nullopt;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied)) {
            if (iequals(entry.path().filename().string(), target.string())) {
                return entry.path();
            }
        }

        return std::nullopt;
    };

    // Start from root (absolute paths) or current working directory (relative paths),
    // then walk each component case-insensitively.
    std::filesystem::path current = filePath.is_absolute() ? filePath.root_path() : std::filesystem::current_path();

    for (const auto& part : filePath.relative_path()) {
        if (part == ".") {
            continue;
        }
        if (part == "..") {
            current = current.parent_path();
            continue;
        }

        auto resolvedPart = findMatchingChild(current, part);
        if (!resolvedPart.has_value()) {
            return std::nullopt;
        }
        current = resolvedPart.value();
    }

    return current.empty() ? std::nullopt : std::optional<std::filesystem::path>(current);
}
