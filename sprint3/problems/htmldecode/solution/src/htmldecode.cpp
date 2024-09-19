#include "htmldecode.h"

std::string HtmlDecode(std::string_view str) {
    return {str.begin(), str.end()};
}
