#include "util.h"

#include <sstream>

#include <climits>

namespace util {

namespace fs = std::filesystem;
using namespace std::literals;

// Возвращает true, если каталог path содержится внутри base.
bool IsSubPath(fs::path path, fs::path base){
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

char from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

// Декодирует URL строку
std::optional<std::string> DecodeURI(std::string_view encoded) {
    static const std::string prefix = "0x"s;
    std::string decoded;
    decoded.reserve(encoded.size());
    int i = 0;
    while (i < encoded.size()) {
        if (encoded[i] == '%') {
            if (i + 2 >= encoded.size()) {
                return std::nullopt;
            }
            size_t count, c;
            try {
                c = std::stoul(std::string{encoded.substr(++i, 2)}, &count, 16);
            } catch (...) {
                return std::nullopt;
            }
            if (count != 2 || c > CHAR_MAX) {
                return std::nullopt;
            }
            decoded.push_back(static_cast<char>(c));
            i += 2;
            continue;
        }
        if (encoded[i] == '+') {
            decoded.push_back(' ');
        } else {
            decoded.push_back(encoded[i]);
        }
        ++i;
    }
    return decoded;
}


std::queue<std::string_view> SplitIntoTokens(std::string_view str, char delimeter) {
    std::queue<std::string_view> words;
    size_t pos = str.find_first_not_of(delimeter);
    const size_t pos_end = str.npos;
    while (pos != pos_end) {
        size_t space = str.find(delimeter, pos);
        words.push(space == pos_end ? str.substr(pos) : str.substr(pos, space - pos));
        pos = str.find_first_not_of(delimeter, space);
    }
    return words;
}

}