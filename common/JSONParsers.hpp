//
// JSONParsers.hpp
//
// Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

// funktioita jotka ottavat jokun streamin ja lukevat siitä halutun asian
// merkkijonoille ja numeroille omat funktiot, ovat lopullisia
// taulukoille pitää olla tieto alityypistä, joten kaavioita
// objekteille olisi joukko alityyppejä, saako kaavioilla tehtyä?
// objektin osalta on mielivaltainen järjestys, joten kuvaus nimeltä arvolle
//  annetaan kaavion perusteella alustettavalle objektille parametrina?

// Syöte tulee forward-iteraattorin kautta jotta datan varastointi ei vaikuta
// Virheet tai vain odottamattomat tyypit saavat aiheuttaa poikkeuksen

#include <exception>
#include <string>
#include <vector>
#include <ctype.h>

// datalackeyssa skannattiin ensin blokki joka oli jotain ja sitten
// tunnistettiin. Pitäisikö tehdä sama vai kartutetaanko arvosta kopio ja
// kun havaitaan erotin, konvertoidaan.
// Tämän voi tehdä merkkijonoille kartuttamalla paitsi jos on Unicodea niin
// sitten pitää ne kerätä kerralla ja tulkita.
// Syötteenä on nykyinen kohta ja viimeinen toistaiseksi saatavilla oleva.

// Toinen optio on ylemmältä tasolta syöttää alemmalle merkki kerrallaan ja
// kun alempi tunnistaa jotain niin se palauttaa arvon. Tämä kertoo samalla
// mistä kohtaa aletaan syöttää seuraavalle tai skannataan välimerkit.

// Alempi taso voi palauttaa bool ja varsinainen arvo tulee kutsujan antaman
// viitteen kautta. Eli kutsuja luo objektin ja stringin kohdalla siihen voi
// jopa lisätä arvoja sitä mukaa kun tarvitsee. Unicode-koodeille voi olla
// pieni puskuri kutsuttavassa.

// Entäpä jos akkumuloitava puskuri olisi kutsujalla? Käyttö valinnaista
// mutta koko täytyy olla tallessa. Stringille ei tarvitse välttämättä
// käyttää puskuria, paitsi jos halutaan välttää yksi merkki kerrallaan
// lisääminen jossa tapauksessa voi kikkailla. Numeroille ei kasva suureksi
// ja parsinta voidaan tehdä suoraan kun työnnetään nollatavu loppuun.
// Stringeille voi olla pieni puskuri Unicode-koodeja varten.

// Add an exception class that these throw when improper input is found.


class ParserException : public std::exception {
private:
    const char* reason;

public:
    ParserException(const char* Reason) : reason(Reason) { }
    const char* what() const throw() { return reason; }
};


class SimpleValueParser {
public:
    virtual ~SimpleValueParser();
    virtual bool Scan(const char* Current) noexcept(false) = 0;
};


class ParseFloat : public SimpleValueParser {
private:
    float& out;
    std::vector<char>& buffer;

public:
    ParseFloat(float& Out, std::vector<char>& AccumulationBuffer)
        : out(Out), buffer(AccumulationBuffer) { }
    bool Scan(const char* Current) noexcept(false);
};


class ParseString : public SimpleValueParser {
private:
    std::string& out;
    std::vector<char>& buffer;
    int count;
    char hex_digits[4];
    bool escaped;

public:
    ParseString(std::string& Out, std::vector<char>& AccumulationBuffer)
        : out(Out), buffer(AccumulationBuffer), count(-1), escaped(false) { }
    bool Scan(const char* Current) noexcept(false);
};


class SkipWhitespace : public SimpleValueParser {
public:
    bool Scan(const char* Current) noexcept(false);
};

