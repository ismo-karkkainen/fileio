//
// JSONWriters.cpp
//
// Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "JSONWriters.hpp"
#include "testdoc.h"
#if defined(TESTDOC_UNITTEST)
#include <sstream>
#include <cmath>
#endif


// Exceptions thrown by templated code.
WriterException NumberNotFinite("Number not finite.");


#if defined(TESTDOC_UNITTEST)

TEST_CASE("Write numbers") {
    std::vector<char> buf;
    SUBCASE("1.23456789f") {
        std::basic_stringstream<char> s;
        Write(s, 1.23456789f, buf);
        REQUIRE(s.str().size() == std::numeric_limits<float>::digits10 + 1);
        REQUIRE(s.str() == "1.23457"); // Rounded, hence 7 at end.
        REQUIRE(buf.size() > 0);
    }
    SUBCASE("1.234567890123456789") {
        std::basic_stringstream<char> s;
        Write(s, 1.234567890123456789, buf);
        REQUIRE(s.str().size() == std::numeric_limits<double>::digits10 + 1);
        REQUIRE(s.str() == "1.23456789012346"); // Rounded, hence 6 at end.
    }
    SUBCASE("12") {
        std::basic_stringstream<char> s;
        Write(s, 12, buf);
        REQUIRE(s.str() == "12");
    }
    SUBCASE("NaN") {
        std::basic_stringstream<char> s;
        REQUIRE_THROWS_AS(Write(s, nan(""), buf), WriterException);
    }
    SUBCASE("Inf") {
        std::basic_stringstream<char> s;
        REQUIRE_THROWS_AS(Write(s, 1.0f / 0.0f, buf), WriterException);
    }
    SUBCASE("-Inf") {
        std::basic_stringstream<char> s;
        REQUIRE_THROWS_AS(Write(s, -1.0f / 0.0f, buf), WriterException);
    }
}

TEST_CASE("Write strings") {
    std::vector<char> buf;
    SUBCASE("normal") {
        std::basic_stringstream<char> s;
        Write(s, "normal", buf);
        REQUIRE(s.str() == "\"normal\"");
    }
    SUBCASE("std::string normal") {
        std::basic_stringstream<char> s;
        Write(s, std::string("normal"), buf);
        REQUIRE(s.str() == "\"normal\"");
    }
    SUBCASE("new\\nline") {
        std::basic_stringstream<char> s;
        Write(s, "new\nline", buf);
        REQUIRE(s.str() == "\"new\\nline\"");
    }
    SUBCASE("quo\"te") {
        std::basic_stringstream<char> s;
        Write(s, "quo\"te", buf);
        REQUIRE(s.str() == "\"quo\\\"te\"");
    }
    SUBCASE("back\\slash") {
        std::basic_stringstream<char> s;
        Write(s, "back\\slash", buf);
        REQUIRE(s.str() == "\"back\\\\slash\"");
    }
    SUBCASE("tab\\t") {
        std::basic_stringstream<char> s;
        Write(s, "tab\t", buf);
        REQUIRE(s.str() == "\"tab\\t\"");
    }
    SUBCASE("cr\\r") {
        std::basic_stringstream<char> s;
        Write(s, "cr\r", buf);
        REQUIRE(s.str() == "\"cr\\r\"");
    }
    SUBCASE("feed\\f") {
        std::basic_stringstream<char> s;
        Write(s, "feed\f", buf);
        REQUIRE(s.str() == "\"feed\\f\"");
    }
    SUBCASE("backspace\\b") {
        std::basic_stringstream<char> s;
        Write(s, "backspace\b", buf);
        REQUIRE(s.str() == "\"backspace\\b\"");
    }
}

TEST_CASE("Unicodes") {
    std::vector<char> buf;
    SUBCASE("a[0x1] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\x1 b", buf);
        REQUIRE(s.str() == "\"a\\u0001 b\"");
    }
    SUBCASE("a[0x9] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\x9 b", buf);
        REQUIRE(s.str() == "\"a\\t b\"");
    }
    SUBCASE("a[0xa] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\xa b", buf);
        REQUIRE(s.str() == "\"a\\n b\"");
    }
    SUBCASE("a[0xf] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\xf b", buf);
        REQUIRE(s.str() == "\"a\\u000f b\"");
    }
    SUBCASE("a[0x10] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\x10 b", buf);
        REQUIRE(s.str() == "\"a\\u0010 b\"");
    }
    SUBCASE("a[0x1f] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\x1f b", buf);
        REQUIRE(s.str() == "\"a\\u001f b\"");
    }
    SUBCASE("a[0x20] b") {
        std::basic_stringstream<char> s;
        Write(s, "a\x20 b", buf);
        REQUIRE(s.str() == "\"a  b\"");
    }
    SUBCASE("a[0x0] b") {
        std::basic_stringstream<char> s;
        char src[] = "a\x0 b";
        Write(s, src, src + 4, buf);
        REQUIRE(s.str() == "\"a\\u0000 b\"");
    }
}

TEST_CASE("Number vector") {
    std::vector<char> buf;
    SUBCASE("[]") {
        std::basic_stringstream<char> s;
        std::vector<int> src;
        Write(s, src, buf);
        REQUIRE(s.str() == "[]");
    }
    SUBCASE("[1]") {
        std::basic_stringstream<char> s;
        std::vector<int> src = { 1 };
        Write(s, src, buf);
        REQUIRE(s.str() == "[1]");
    }
    SUBCASE("[1,2]") {
        std::basic_stringstream<char> s;
        std::vector<int> src = { 1, 2 };
        Write(s, src, buf);
        REQUIRE(s.str() == "[1,2]");
    }
    SUBCASE("[1,2,3]") {
        std::basic_stringstream<char> s;
        std::vector<int> src = { 1, 2, 3 };
        Write(s, src, buf);
        REQUIRE(s.str() == "[1,2,3]");
    }
}

TEST_CASE("String vector") {
    std::vector<char> buf;
    SUBCASE("[]") {
        std::basic_stringstream<char> s;
        std::vector<std::string> src;
        Write(s, src, buf);
        REQUIRE(s.str() == "[]");
    }
    SUBCASE("[\"a\"]") {
        std::basic_stringstream<char> s;
        std::vector<std::string> src = { "a" };
        Write(s, src, buf);
        REQUIRE(s.str() == "[\"a\"]");
    }
    SUBCASE("[\"a\",\"b\"]") {
        std::basic_stringstream<char> s;
        std::vector<std::string> src = { "a", "b" };
        Write(s, src, buf);
        REQUIRE(s.str() == "[\"a\",\"b\"]");
    }
    SUBCASE("[\"a\",\"b\",\"c\"]") {
        std::basic_stringstream<char> s;
        std::vector<std::string> src = { "a", "b", "c" };
        Write(s, src, buf);
        REQUIRE(s.str() == "[\"a\",\"b\",\"c\"]");
    }
    SUBCASE("[\"a\"]") {
        std::basic_stringstream<char> s;
        std::vector<const char*> src = { "a" };
        Write(s, src, buf);
        REQUIRE(s.str() == "[\"a\"]");
    }
}

TEST_CASE("Number vector vector") {
    std::vector<char> buf;
    SUBCASE("[]") {
        std::basic_stringstream<char> s;
        std::vector<std::vector<int>> src;
        Write(s, src, buf);
        REQUIRE(s.str() == "[]");
    }
    SUBCASE("[[]]") {
        std::basic_stringstream<char> s;
        std::vector<std::vector<int>> src;
        src.push_back(std::vector<int>());
        Write(s, src, buf);
        REQUIRE(s.str() == "[[]]");
    }
    SUBCASE("[[1]]") {
        std::basic_stringstream<char> s;
        std::vector<std::vector<int>> src;
        src.push_back({ 1 });
        Write(s, src, buf);
        REQUIRE(s.str() == "[[1]]");
    }
    SUBCASE("[[1],[2]]") {
        std::basic_stringstream<char> s;
        std::vector<std::vector<int>> src;
        src.push_back({ 1 });
        src.push_back({ 2 });
        Write(s, src, buf);
        REQUIRE(s.str() == "[[1],[2]]");
    }
}

#endif
