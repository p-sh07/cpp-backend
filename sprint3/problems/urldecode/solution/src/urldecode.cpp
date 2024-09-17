#include "urldecode.h"

#include <charconv>
#include <stdexcept>

std::string UrlDecode(std::string_view str) {
      if(str.empty()) {
        return {};
    }

    std::string decoded_str;

    for(size_t pos = 0; pos < str.size();) {
        if(str[pos] == '%') {
            //check two symbols after % are a hex num
            if(str.size() - pos <= 2) {
                throw std::invalid_argument("encoded symbol len < 2");
            }

            std::string_view sym_code = str.substr(pos+1, 2);
            for(const char& c : sym_code) {
                if(!std::isxdigit(c)) {
                    throw std::invalid_argument("encoded symbol is not a hex digit");
                }
            }
            try {
                //decode
                char decoded = std::stoul(std::string(sym_code), nullptr, 16);
                if(isblank(decoded)) {
                    throw std::invalid_argument("");
                }
                decoded_str.push_back(decoded);
                pos += 3;
            } catch (...) {
                throw std::invalid_argument("failed to convert hex to char");
            }
        } else if(str[pos] == '+') {
            decoded_str.push_back(' ');
        } else if(str[pos] == ' ') {
            throw std::invalid_argument("space \' \' not allowed in url");
        }
        else {
            decoded_str.push_back(str[pos++]);
        }
    }
    return decoded_str;
}
