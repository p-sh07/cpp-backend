#include "urlencode.h"
#include <iomanip>
#include <sstream>

using namespace std::literals;

namespace {
const std::string reserved = "!#$&'()*+,/:;=?@[]"s;

bool MustEncode(const char c) {
    if(reserved.find(c) != std::string::npos) {
        return true;
    }
    auto numeric = static_cast<unsigned short>(c);
    if(numeric < 32 || numeric > 127) {
        return true;
    }
    return false;
}

}

std::string UrlEncode(std::string_view str) {
    if(str.empty()) {
        return {};
    }

    std::stringstream encoded;

    for(const auto c : str) {
        if(c == ' ') {
            encoded << '+';
        } else if(MustEncode(c)) {
            encoded << '%' << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned short>(c);
        }
        else {
            encoded << c;
        }
    }
    return encoded.str();
}
