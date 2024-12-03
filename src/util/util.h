#pragma once
#include <filesystem>
#include <optional>
#include <queue>

namespace util {

namespace fs = std::filesystem;

// Возвращает true, если каталог p содержится внутри base_path.
bool IsSubPath(fs::path path, fs::path base);

std::optional<std::string> DecodeURI(std::string_view encoded);

std::queue<std::string_view> SplitIntoTokens(std::string_view str, char delimeter);

}