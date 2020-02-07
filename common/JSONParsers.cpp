//
// JSONParsers.cpp
//
// Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "JSONParsers.hpp"
#include "testdoc.h"


SimpleValueParser::~SimpleValueParser() { }

static ParserException InvalidFloat("Invalid float.");
static ParserException NoFloat("No float.");

bool ParseFloat::Scan(const char* Current) noexcept(false) {
    if ('0' <= *Current && *Current <= '9' || *Current == '.' ||
        *Current == 'e' || *Current == 'E' ||
        *Current == '-' || *Current == '+')
    {
        buffer.push_back(*Current);
        return false;
    }
    // First non-float character triggers conversion.
    // Separator scan will throw if the string in source is not a number.
    if (buffer.size() == 0)
        throw NoFloat;
    std::string s(buffer.begin(), buffer.end());
    char* end = nullptr;
    out = strtof(s.c_str(), &end);
    if (end != s.c_str() + s.size())
        throw InvalidFloat;
    return true;
}


static ParserException StringEscape("String with unknown escape.");
static ParserException StringHexDigits("String with invalid hex digits.");
static ParserException StringInvalidCharacter("String with invalid character.");

bool ParseString::Scan(const char* Current) noexcept(false) {
    if (!escaped && count == -1) {
        if (*Current != '\\') {
            if (*Current != '"') {
                if constexpr (static_cast<char>(0x80) < 0) {
                    // Signed char.
                    if (31 < *Current || *Current < 0) {
                        buffer.push_back(*Current);
                        if (buffer.size() > out.size()) {
                            out.append(buffer.begin(), buffer.end());
                            buffer.resize(0);
                        }
                    } else
                        throw StringInvalidCharacter;
                } else {
                    // Unsigned char.
                    if (31 < *Current) {
                        buffer.push_back(*Current);
                        if (buffer.size() > out.size()) {
                            out.append(buffer.begin(), buffer.end());
                            buffer.resize(0);
                        }
                    } else
                        throw StringInvalidCharacter;
                }
            } else {
                out.append(buffer.begin(), buffer.end());
                return true;
            }
        } else
            escaped = true;
    } else if (count != -1) {
        hex_digits[count++] = *Current;
        if (count < 4)
            return false;
        int value = 0;
        for (int k = 0; k < 4; ++k) {
            int m = 0;
            if ('0' <= hex_digits[k] && hex_digits[k] <= '9')
                m = hex_digits[k] - '0';
            else if ('a' <= hex_digits[k] && hex_digits[k] <= 'f')
                m = 10 + hex_digits[k] - 'a';
            else if ('A' <= hex_digits[k] && hex_digits[k] <= 'F')
                m = 10 + hex_digits[k] - 'A';
            else
                throw StringHexDigits;
            value = (value << 4) + m;
        }
        if (value < 0x80)
            buffer.push_back(static_cast<char>(value));
        else if (value < 0x800) {
            buffer.push_back(static_cast<char>(0xc0 | ((value >> 6) & 0x1f)));
            buffer.push_back(static_cast<char>(0x80 | (value & 0x3f)));
        } else {
            buffer.push_back(static_cast<char>(0xe0 | ((value >> 12) & 0xf)));
            buffer.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
            buffer.push_back(static_cast<char>(0x80 | (value & 0x3f)));
        }
        count = -1;
    } else {
        switch (*Current) {
        case '"':
        case '/':
        case '\\':
            buffer.push_back(*Current);
            break;
        case 'b': buffer.push_back('\b'); break;
        case 'f': buffer.push_back('\f'); break;
        case 'n': buffer.push_back('\n'); break;
        case 'r': buffer.push_back('\r'); break;
        case 't': buffer.push_back('\t'); break;
        case 'u': count = 0; break;
        default:
            throw StringEscape;
        }
        escaped = false;
    }
    return false;
}

bool SkipWhitespace::Scan(const char* Current) noexcept(false) {
    return !(*Current == ' ' ||
        *Current == '\x9' || *Current == '\xA' || *Current == '\xD');
}

TEST_CASE("Floats") {
    std::vector<char> buffer;
    float out;
    ParseFloat parser(out, buffer);
    char space(' ');
    SUBCASE("123") {
        buffer.resize(0);
        std::string s("123");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&space) == true);
        REQUIRE(out == 123.0f);
    }
    SUBCASE("456.789") {
        buffer.resize(0);
        std::string s("456.789");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&space) == true);
        REQUIRE(out == 456.789f);
    }
    SUBCASE("1e6") {
        buffer.resize(0);
        std::string s("1e6");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&space) == true);
        REQUIRE(out == 1e6f);
    }
    SUBCASE("2E6") {
        buffer.resize(0);
        std::string s("2E6");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&space) == true);
        REQUIRE(out == 2e6f);
    }
    SUBCASE("-1.2") {
        buffer.resize(0);
        std::string s("-1.2");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&space) == true);
        REQUIRE(out == -1.2f);
    }
    SUBCASE("+0.9") {
        buffer.resize(0);
        std::string s("+0.9");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&space) == true);
        REQUIRE(out == 0.9f);
    }
    SUBCASE("empty") {
        buffer.resize(0);
        REQUIRE_THROWS_AS(parser.Scan(&space), ParserException);
    }
    SUBCASE("1e3e") {
        buffer.resize(0);
        std::string s("1e3e");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE_THROWS_AS(parser.Scan(&space), ParserException);
    }
}

TEST_CASE("String and escapes") {
    std::vector<char> buffer;
    std::string out;
    ParseString parser(out, buffer);
    char quote('"');
    SUBCASE("empty") {
        buffer.resize(0);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "");
    }
    SUBCASE("string") {
        buffer.resize(0);
        std::string s("string");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "string");
    }
    SUBCASE("a\\\"b") {
        buffer.resize(0);
        std::string s("a\\\"b");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "a\"b");
    }
    SUBCASE("a\\\"") {
        buffer.resize(0);
        std::string s("a\\\"");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "a\"");
    }
    SUBCASE("\\\"b") {
        buffer.resize(0);
        std::string s("\\\"b");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "\"b");
    }
    SUBCASE("\\/\\\\\\b\\f\\n\\r\\t") {
        buffer.resize(0);
        std::string s("\\/\\\\\\b\\f\\n\\r\\t");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "/\\\b\f\n\r\t");
    }
    SUBCASE("Invalid escape") {
        std::string valid("\"/\\bfnrtu");
        char escape('\\');
        for (unsigned char u = 255; 31 < u; --u) {
            ParseString p(out, buffer);
            p.Scan(&escape);
            char c(static_cast<char>(u));
            if (valid.find(c) == std::string::npos)
                REQUIRE_THROWS_AS(p.Scan(&c), ParserException);
        }
    }
    SUBCASE("Too small") {
        char c(0x1F);
        REQUIRE_THROWS_AS(parser.Scan(&c), ParserException);
    }
    SUBCASE("Too small") {
        char c(0x01);
        REQUIRE_THROWS_AS(parser.Scan(&c), ParserException);
    }
}

TEST_CASE("String Unicode") {
    std::vector<char> buffer;
    std::string out;
    ParseString parser(out, buffer);
    char quote('"');
    SUBCASE("\\u0079") {
        buffer.resize(0);
        std::string s("\\u0079");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "\x79");
    }
    SUBCASE("\\u0080") {
        buffer.resize(0);
        std::string s("\\u0080");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "\xC2\x80");
    }
    SUBCASE("\\u07FF") {
        buffer.resize(0);
        std::string s("\\u07FF");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "\xDF\xBF");
    }
    SUBCASE("\\u0800") {
        buffer.resize(0);
        std::string s("\\u0800");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "\xE0\xA0\x80");
    }
    SUBCASE("\\uFFFF") {
        buffer.resize(0);
        std::string s("\\uFFFF");
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(parser.Scan(iter) == false);
        REQUIRE(parser.Scan(&quote) == true);
        REQUIRE(out == "\xEF\xBF\xBF");
    }
}

TEST_CASE("Spaces") {
    SkipWhitespace skipper;
    std::string s(" \x9\xA\xD");
    SUBCASE("Valid spaces") {
        for (const char* iter = s.c_str(); iter != s.c_str() + s.size(); ++iter)
            REQUIRE(skipper.Scan(iter) == false);
    }
    SUBCASE("Non-spaces") {
        for (unsigned char c = 255; c; --c)
            if (s.find(c) == std::string::npos) {
                char sc = static_cast<char>(c);
               REQUIRE(skipper.Scan(&sc) == true);
            }
    }
}
