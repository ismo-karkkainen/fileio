//
// JSONParsers.hpp
//
// Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#if !defined(JSONPARSERS_HPP)
#define JSONPARSERS_HPP

#include <exception>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <ctype.h>
#include <cstring>


class ParserException : public std::exception {
private:
    const char* reason;

public:
    ParserException(const char* Reason) : reason(Reason) { }
    const char* what() const throw() { return reason; }
};


class ParserPool;


class SimpleValueParser {
protected:
    bool finished;
    const char* setFinished(const char* Endptr, ParserPool& Pool);
    const char* setFinished(const char* Endptr);

    const char* skipWhitespace(const char* Begin, const char* End);
    inline bool isWhitespace(const char C) {
        return C == ' ' || C == '\x9' || C == '\xA' || C == '\xD';
    }

public:
    SimpleValueParser() : finished(true) { }
    virtual ~SimpleValueParser();

    bool Finished() const { return finished; }
    // Returns nullptr if not finished (reached end) or pointer where ended.
    virtual const char* Scan(
        const char* Begin, const char* End, ParserPool& Pool)
        noexcept(false) = 0;
};


class ParseFloat : public SimpleValueParser {
public:
    typedef float Type;
    enum Pool { Index = 0 }; // Has to match ParserPool order.

    const char* Scan(const char* Begin, const char* End, ParserPool& Pool)
        noexcept(false);
};


class ParseString : public SimpleValueParser {
public:
    typedef std::string Type;

private:
    int count;
    char hex_digits[4];
    bool escaped, began;

    bool scan(const char Current, Type& out, std::vector<char>& buffer)
        noexcept(false);

public:
    enum Pool { Index = 1 }; // Has to match ParserPool order.

    ParseString() : count(-1), escaped(false), began(false) { }
    const char* Scan(const char* Begin, const char* End, ParserPool& Pool)
        noexcept(false);
};


// Holds lowest-level parsers and buffer they share.
class ParserPool {
public:
    ParserPool() { }
    ParserPool(const ParserPool&) = delete;
    ParserPool& operator=(const ParserPool&) = delete;
    virtual ~ParserPool();

    std::vector<char> buffer;

    enum Parsers { Float, String }; // Match with order below.
    // Match order with parsers' Pool enum Index values.
    std::tuple<ParseFloat, ParseString> Parser;
    std::tuple<ParseFloat::Type, ParseString::Type> Value;
};


extern ParserException NotFinished;

template<typename Parser>
class ParseArray : public SimpleValueParser {
public:
    typedef std::vector<typename Parser::Type> Type;

private:
    Type out;
    bool began, expect_number;

public:
    ParseArray() : began(false), expect_number(true) { }
    const char* Scan(const char* Begin, const char* End, ParserPool& Pool)
        noexcept(false);

    void Swap(Type& Alt) {
        if (!Finished())
            throw NotFinished;
        std::swap(Alt, out);
        out.resize(0);
    }
};


extern ParserException InvalidArrayStart;
extern ParserException InvalidArraySeparator;

template<typename Parser>
const char* ParseArray<Parser>::Scan(
    const char* Begin, const char* End, ParserPool& Pool) noexcept(false)
{
    Parser& p(std::get<Parser::Pool::Index>(Pool.Parser));
    typename Parser::Type& value(std::get<Parser::Pool::Index>(Pool.Value));
    if (!p.Finished()) {
        // In the middle of parsing value when buffer ended?
        Begin = p.Scan(Begin, End, Pool);
        if (Begin == nullptr)
            return setFinished(nullptr);
        out.push_back(value);
        expect_number = false;
    } else if (!began) {
        // Expect '[' on first call.
        if (*Begin != '[')
            throw InvalidArrayStart;
        began = expect_number = true;
        Begin = skipWhitespace(++Begin, End);
        if (Begin == nullptr)
            return setFinished(nullptr);
        if (*Begin == ']') {
            began = false; // In case caller re-uses. Out must be empty.
            return setFinished(++Begin);
        }
    } else if (out.empty()) {
        Begin = skipWhitespace(Begin, End);
        if (Begin == nullptr)
            return setFinished(nullptr);
        if (*Begin == ']') {
            began = false; // In case caller re-uses. Out must be empty.
            return setFinished(++Begin);
        }
    }
    while (Begin != End) {
        if (expect_number) {
            if (isWhitespace(*Begin)) {
                Begin = skipWhitespace(++Begin, End);
                if (Begin == nullptr)
                    return setFinished(nullptr);
            }
            // Now there should be the item to parse.
            Begin = p.Scan(Begin, End, Pool);
            if (Begin == nullptr)
                return setFinished(nullptr);
            out.push_back(value);
            expect_number = false;
        }
        // Comma, maybe surrounded by spaces.
        if (*Begin == ',') // Most likely unless prettified.
            ++Begin;
        else {
            Begin = skipWhitespace(Begin, End);
            if (Begin == nullptr)
                return setFinished(nullptr);
            if (*Begin == ']') {
                began = false;
                return setFinished(++Begin);
            }
            if (*Begin != ',')
                throw InvalidArraySeparator;
            Begin++;
        }
        expect_number = true;
    }
    return setFinished(nullptr);
}


template<typename Parser>
class ParseContainerArray : public SimpleValueParser {
public:
    typedef std::vector<typename Parser::Type> Type;

private:
    Parser p;
    Type out;
    bool began, expect_item;

public:
    ParseContainerArray() : began(false), expect_item(true) { }
    const char* Scan(const char* Begin, const char* End, ParserPool& Pool)
        noexcept(false);

    void Swap(Type& Alt) {
        if (!Finished())
            throw NotFinished;
        std::swap(Alt, out);
        out.resize(0);
    }
};


template<typename Parser>
const char* ParseContainerArray<Parser>::Scan(
    const char* Begin, const char* End, ParserPool& Pool) noexcept(false)
{
    if (!p.Finished()) {
        // In the middle of parsing value when buffer ended?
        Begin = p.Scan(Begin, End, Pool);
        if (Begin == nullptr)
            return setFinished(nullptr);
        out.push_back(typename Parser::Type());
        p.Swap(out.back());
        expect_item = false;
    } else if (!began) {
        // Expect '[' on first call.
        if (*Begin != '[')
            throw InvalidArrayStart;
        began = expect_item = true;
        Begin = skipWhitespace(++Begin, End);
        if (Begin == nullptr)
            return setFinished(nullptr);
        if (*Begin == ']') {
            began = false; // In case caller re-uses. Out must be empty.
            return setFinished(++Begin);
        }
    } else if (out.empty()) {
        Begin = skipWhitespace(Begin, End);
        if (Begin == nullptr)
            return setFinished(nullptr);
        if (*Begin == ']') {
            began = false; // In case caller re-uses. Out must be empty.
            return setFinished(++Begin);
        }
    }
    while (Begin != End) {
        if (expect_item) {
            if (isWhitespace(*Begin)) {
                Begin = skipWhitespace(++Begin, End);
                if (Begin == nullptr)
                    return setFinished(nullptr);
            }
            // Now there should be the item to parse.
            Begin = p.Scan(Begin, End, Pool);
            if (Begin == nullptr)
                return setFinished(nullptr);
            out.push_back(typename Parser::Type());
            p.Swap(out.back());
            expect_item = false;
        }
        // Comma, maybe surrounded by spaces.
        if (*Begin == ',') // Most likely unless prettified.
            ++Begin;
        else {
            Begin = skipWhitespace(Begin, End);
            if (Begin == nullptr)
                return setFinished(nullptr);
            if (*Begin == ']') {
                began = false;
                return setFinished(++Begin);
            }
            if (*Begin != ',')
                throw InvalidArraySeparator;
            Begin++;
        }
        expect_item = true;
    }
    return setFinished(nullptr);
}


class ValueStore;

// Helper class for implementation of KeyValues template.
class ScanningKeyValue {
protected:
    bool given;
    void Give(ValueStore* VS);

public:
    ScanningKeyValue() : given(false) { }
    virtual ~ScanningKeyValue();
    virtual SimpleValueParser& Scanner(ParserPool& Pool) = 0;
    virtual const char* Key() const = 0;
    virtual void Swap(ValueStore* VS, ParserPool& Pool) = 0;
    virtual bool Required() const = 0;
    bool Given() const { return given; }
};

// Helper class for implementation of KeyValues template.
template<const char* KeyString, typename Parser>
class KeyValue : public ScanningKeyValue {
public:
    typedef typename Parser::Type Type;
    const char* Key() const { return KeyString; }
    void Swap(ValueStore* VS, ParserPool& Pool);

    void Swap(Type& Alt, ParserPool& Pool) {
        std::swap(Alt, std::get<Parser::Pool::Index>(Pool.Value));
        given = false;
    }

    SimpleValueParser& Scanner(ParserPool& Pool) {
        given = true;
        return std::get<Parser::Pool::Index>(Pool.Parser);
    }

    bool Required() const { return false; }
};

// Helper class for implementation of KeyValues template.
template<const char* KeyString, typename Parser>
class RequiredKeyValue : public KeyValue<KeyString,Parser> {
public:
    bool Required() const { return true; }
};

// Helper class for implementation of KeyValues template.
template<const char* KeyString, typename Parser>
class KeyContainerValue : public ScanningKeyValue {
private:
    Parser p;

public:
    typedef typename Parser::Type Type;
    const char* Key() const { return KeyString; }
    void Swap(ValueStore* VS, ParserPool& Pool);

    void Swap(Type& Alt, ParserPool& Pool) {
        p.Swap(Alt);
        given = false;
    }

    SimpleValueParser& Scanner(ParserPool& Pool) {
        given = true;
        return p;
    }

    bool Required() const { return false; }
};

// Helper class for implementation of KeyValues template.
template<const char* KeyString, typename Parser>
class RequiredKeyContainerValue : public KeyContainerValue<KeyString,Parser> {
    bool Required() const { return true; }
};


// Helper class for implementation of ParseObject class.
template<class ... Fields>
class KeyValues {
private:
    std::vector<ScanningKeyValue*> ptrs;

    template<typename FuncObj, typename Tuple, size_t... IdxSeq>
    void apply_to_each(FuncObj&& fo, Tuple&& t, std::index_sequence<IdxSeq...>)
    {
        int a[] = {
            (fo(std::get<IdxSeq>(std::forward<Tuple>(t))), void(), 0) ...
        };
        (void)a; // Suppresses unused variable warning.
    }

    class Pusher {
    private:
        std::vector<ScanningKeyValue*>& tgt;
    public:
        Pusher(std::vector<ScanningKeyValue*>& Target) : tgt(Target) { }
        void operator()(ScanningKeyValue& SKV) { tgt.push_back(&SKV); }
    };

public:
    std::tuple<Fields...> fields;

    KeyValues() {
        Pusher psh(ptrs);
        apply_to_each(psh, fields,
            std::make_index_sequence<std::tuple_size<decltype(fields)>::value>
                {});
    }

    size_t size() const { return std::tuple_size<decltype(fields)>::value; }
    SimpleValueParser& Scanner(size_t Index, ParserPool& Pool) {
        return ptrs[Index]->Scanner(Pool);
    }
    ScanningKeyValue* KeyValue(size_t Index) { return ptrs[Index]; }
};

// Another derived class adds all the convenience methods that map to index.

class ValueStore {
protected:
    bool given;

private:
    void Give();
    friend class ScanningKeyValue;

public:
    ValueStore() : given(false) { }
    bool Given() const { return given; }
};

template<typename Parser>
class Value : public ValueStore {
public:
    typedef typename Parser::Type Type;
    Type value;
};

template<class ... Fields>
class NamelessValues {
private:
    std::vector<ValueStore*> ptrs;

    template<typename FuncObj, typename Tuple, size_t... IdxSeq>
    void apply_to_each(FuncObj&& fo, Tuple&& t, std::index_sequence<IdxSeq...>)
    {
        int a[] = {
            (fo(std::get<IdxSeq>(std::forward<Tuple>(t))), void(), 0) ...
        };
        (void)a; // Suppresses unused variable warning.
    }

    class Pusher {
    private:
        std::vector<ValueStore*>& tgt;
    public:
        Pusher(std::vector<ValueStore*>& Target) : tgt(Target) { }
        void operator()(ValueStore& VS) { tgt.push_back(&VS); }
    };

public:
    std::tuple<Fields...> fields;

    NamelessValues() {
        Pusher psh(ptrs);
        apply_to_each(psh, fields, std::make_index_sequence<std::tuple_size<decltype(fields)>::value> {});
    }

    size_t size() const { return std::tuple_size<decltype(fields)>::value; }
    ValueStore* operator[](size_t Index) { return ptrs[Index]; }
};

template<const char* KeyString, typename Parser>
void KeyValue<KeyString,Parser>::Swap(ValueStore* VS, ParserPool& Pool) {
    Value<Parser>* dst(static_cast<Value<Parser>*>(VS));
    std::swap(dst->value, std::get<Parser::Pool::Index>(Pool.Value));
    Give(VS);
}

template<const char* KeyString, typename Parser>
void KeyContainerValue<KeyString,Parser>::Swap(ValueStore* VS, ParserPool& Pool)
{
    Value<Parser>* dst(static_cast<Value<Parser>*>(VS));
    p.Swap(dst->value);
    Give(VS);
}

// The Values class used by the template should be derived from NamelessValues
// and it adds way to access the fields using sensibly named methods.

extern ParserException InvalidKey;
extern ParserException RequiredKeyNotGiven;

template<typename KeyValues, typename Values>
class ParseObject : public SimpleValueParser {
private:
    KeyValues parsers;
    Values out;
    int activating, active;
    enum State {
        NotStarted,
        PreKey,
        ExpectKey,
        PreColon,
        ExpectColon,
        PreValue,
        ExpectValue,
        PreComma,
        ExpectComma
    };
    State state;

    void setActivating(const std::string& Incoming) {
        for (int k = 0; k < parsers.size(); ++k)
            if (strcmp(Incoming.c_str(), parsers.KeyValue(k)->Key()) == 0) {
                activating = k;
                return;
            }
        throw InvalidKey;
    }

    const char* checkPassed(const char* Ptr) noexcept(false) {
        for (size_t k = 0; k < out.size(); ++k) {
            ScanningKeyValue* skv = parsers.KeyValue(k);
            ValueStore* vs = out[k];
            if (skv->Required() && !vs->Given())
                throw RequiredKeyNotGiven;
        }
        state = NotStarted;
        activating = -1;
        active = -1;
        return setFinished(Ptr);
    }

public:
    typedef decltype(out.fields) Type;

    ParseObject() : activating(-1), active(-1), state(NotStarted) { }

    const char* Scan(const char* Begin, const char* End, ParserPool& Pool)
        noexcept(false);

    void Swap(Type& Alt) {
        std::swap(Alt, out.fields);
        out.fields = Type();
    }
};

extern ParserException InvalidObjectStart;
extern ParserException InvalidKeySeparator;
extern ParserException InvalidValueSeparator;

template<typename KeyValues, typename Values>
const char* ParseObject<KeyValues,Values>::Scan(
    const char* Begin, const char* End, ParserPool& Pool) noexcept(false)
{
    while (Begin != End) {
        // Re-order states to most expected first once works.
        switch (state) {
        case NotStarted:
            // Expect '{' on the first call.
            if (*Begin != '{')
                throw InvalidObjectStart;
            state = PreKey;
            ++Begin;
        case PreKey:
            if (isWhitespace(*Begin)) {
                Begin = skipWhitespace(++Begin, End);
                if (Begin == nullptr)
                    return setFinished(nullptr);
            }
            if (*Begin == '}')
                return checkPassed(++Begin);
            state = ExpectKey;
        case ExpectKey:
            Begin = std::get<ParserPool::String>(Pool.Parser).Scan(
                Begin, End, Pool);
            if (Begin == nullptr)
                return setFinished(nullptr);
            setActivating(std::get<ParserPool::String>(Pool.Value));
            state = PreColon;
        case PreColon:
            if (isWhitespace(*Begin)) {
                Begin = skipWhitespace(++Begin, End);
                if (Begin == nullptr)
                    return setFinished(nullptr);
            }
            state = ExpectColon;
        case ExpectColon:
            if (*Begin != ':')
                throw InvalidKeySeparator;
            state = PreValue;
            if (++Begin == End)
                return setFinished(nullptr);
        case PreValue:
            if (isWhitespace(*Begin)) {
                Begin = skipWhitespace(++Begin, End);
                if (Begin == nullptr)
                    return setFinished(nullptr);
            }
            active = activating;
            activating = -1;
            state = ExpectValue;
        case ExpectValue:
            Begin = parsers.Scanner(active, Pool).Scan(Begin, End, Pool);
            if (Begin == nullptr)
                return setFinished(nullptr);
            parsers.KeyValue(active)->Swap(out[active], Pool);
            active = -1;
            state = PreComma;
        case PreComma:
            if (isWhitespace(*Begin)) {
                Begin = skipWhitespace(++Begin, End);
                if (Begin == nullptr)
                    return setFinished(nullptr);
            }
            state = ExpectComma;
        case ExpectComma:
            if (*Begin == '}')
                return checkPassed(++Begin);
            if (*Begin != ',')
                throw InvalidValueSeparator;
            state = PreKey;
            ++Begin;
        }
    }
    return setFinished(nullptr);
}

// multi-dimensional array of one type would probably be more
// efficient. One just needs to have operator() set properly and scalar type.

// the array, array, array could be cube and array, array a plane for short
// it would also tell if all sub-arrays are expected to be of same length in
// the same direction.

// That would simplify specifying what you have but needs explaining
// Then again array, array, array is for case where they are of different length
// The limitation needs to be specified anyway somehow.
// Maybe take limitation and format and combine them?

#endif
