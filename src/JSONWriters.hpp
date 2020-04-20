//
// JSONWriters.hpp
//
// Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#if !defined(JSONWRITERS_HPP)
#define JSONWRITERS_HPP

#include <exception>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <ctype.h>
#include <cstring>
#include <cstdio>
#if !defined(__GNUG__)
#include <cmath>
#else
#include <math.h>
#endif


class WriterException : public std::exception {
private:
    const char* reason;

public:
    WriterException(const char* Reason) : reason(Reason) { }
    const char* what() const throw() { return reason; }
};

extern WriterException NumberNotFinite;

template<typename Sink>
void Write(Sink& S, double Value, std::vector<char>& Buffer) {
    if (isfinite(Value)) {
        int count = snprintf(&Buffer.front(), Buffer.size(),
            "%.*g", std::numeric_limits<double>::digits10, Value);
        if (count < static_cast<int>(Buffer.size()))
            S.write(&Buffer.front(), count);
        else {
            Buffer.resize(count + 1);
            Write(S, Value, Buffer);
        }
    } else
        throw NumberNotFinite;
}

template<typename Sink>
void Write(Sink& S, float Value, std::vector<char>& Buffer) {
    if (isfinite(Value)) {
        int count = snprintf(&Buffer.front(), Buffer.size(),
            "%.*g", std::numeric_limits<float>::digits10, double(Value));
        if (count < static_cast<int>(Buffer.size()))
            S.write(&Buffer.front(), count);
        else {
            Buffer.resize(count + 1);
            Write(S, Value, Buffer);
        }
    } else
        throw NumberNotFinite;
}

template<typename Sink>
void Write(Sink& S, int Value, std::vector<char>& Buffer) {
    int count = snprintf(&Buffer.front(), Buffer.size(), "%i", Value);
    if (count < static_cast<int>(Buffer.size()))
        S.write(&Buffer.front(), count);
    else {
        Buffer.resize(count + 1);
        Write(S, Value, Buffer);
    }
}

template<typename Sink>
void Write(
    Sink& S, const char* Begin, const char* End, std::vector<char>& Buffer)
{
    char quote = '"';
    char escaped[3] = "\\ ";
    S.write(&quote, 1);
    for (const char* curr = Begin; curr < End; ++curr) {
        escaped[1] = 0;
        switch (*curr) {
        case '"': escaped[1] = '"'; break;
        case '\\': escaped[1] = '\\'; break;
        case '\n': escaped[1] = 'n'; break;
        case '\t': escaped[1] = 't'; break;
        case '\r': escaped[1] = 'r'; break;
        case '\f': escaped[1] = 'f'; break;
        case '\b': escaped[1] = 'b'; break;
        }
        if (escaped[1]) {
            S.write(Begin, curr - Begin);
            S.write(escaped, 2);
            Begin = curr + 1;
        } else if (*curr < 0x20) {
            S.write(Begin, curr - Begin);
            char u[7] = "\\u00xx";
            char c = *curr & 0xf;
            u[4] = (c == *curr) ? '0' : '1';
            u[5] = (c < 10) ? '0' + c : 'a' + c - 10;
            S.write(u, 6);
            Begin = curr + 1;
        }
    }
    if (Begin < End)
        S.write(Begin, End - Begin);
    S.write(&quote, 1);
}

template<typename Sink>
void Write(Sink& S, const std::string& Value, std::vector<char>& Buffer) {
    Write(S, Value.c_str(), Value.c_str() + Value.size(), Buffer);
}

template<typename Sink, typename T>
void Write(Sink& S, const std::vector<T>& Value, std::vector<char>& Buffer);

template<typename Sink, typename T>
void Write(Sink& S, const T* Value, std::vector<char>& Buffer) {
    if (Value != nullptr)
        Write(S, *Value, Buffer);
    else {
        char null[] = "null";
        S.write(&null, 4);
    }
}

template<typename Sink>
void Write(Sink& S, const char* Value, std::vector<char>& Buffer) {
    if (Value != nullptr)
        Write(S, Value, Value + strlen(Value), Buffer);
    else {
        char null[] = "null";
        S.write(null, 4);
    }
}

template<typename Sink, typename ForwardIterator>
void Write(Sink& S,
    ForwardIterator& Begin, ForwardIterator& End, std::vector<char>& Buffer)
{
    char c = '[';
    S.write(&c, 1);
    if (Begin != End) {
        Write(S, *Begin, Buffer);
        c = ',';
        while (++Begin != End) {
            S.write(&c, 1);
            Write(S, *Begin, Buffer);
        }
    }
    c = ']';
    S.write(&c, 1);
}

template<typename Sink, typename T>
void Write(Sink& S, const std::vector<T>& Value, std::vector<char>& Buffer) {
    auto b = Value.cbegin();
    auto e = Value.cend();
    Write<Sink,typename std::vector<T>::const_iterator>(S, b, e, Buffer);
}

// Object needs checks for all values (assume xGiven?)
// and will also need write order in case it matters.
// If no xGiven then all members need to be known. Pass accessor name.
// The value is passed to another stringify generated for that type.
// Tester name could also be given.

// Top-level object is needed for field names. Generate and put in the fields.
// Something similar to Value, with getter name configurable.

#endif
