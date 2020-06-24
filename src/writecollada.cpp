//
//  writecollada.cpp
//
//  Created by Ismo Kärkkäinen on 11.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "writecollada_io.hpp"
#if defined(UNITTEST)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#else
#include "convenience.hpp"
#endif
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <deque>


#if !defined(UNITTEST)
static int writecollada(io::WriteColladaIn& Val) {
    // Convert all tri-strips (and later fans) to triangles.
    std::vector<std::vector<std::uint32_t>> triangles;
    for (auto& strip : Val.tristrips())
        for (size_t k = 0; k < strip.size() - 2; ++k)
            if (k & 1)
                triangles.push_back(std::vector<std::uint32_t> {
                    strip[k], strip[k + 2], strip[k + 1] });
            else
                triangles.push_back(std::vector<std::uint32_t> {
                    strip[k], strip[k + 1], strip[k + 2] });
    std::ofstream out(Val.filename().c_str());
    if (out.fail()) {
        std::cerr << "Failed to open: " << Val.filename() << std::endl;
        return 1;
    }
    std::vector<char> buffer;
    out << R"WRDAE(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2008/03/COLLADASchema" version="1.5.0">)WRDAE";
    if (Val.assetGiven())
        out << "\n<asset>" << Val.asset() << "</asset>";
    if (Val.effectsGiven())
        out << "\n<library_effects>" << Val.effects() << "</library_effects>";
    else
        out << R"WRDAE(
<library_effects><effect id="effect"><profile_COMMON>
  <technique sid="COMMON"><blinn>
    <diffuse><color>0.8 0.8 0.8 1</color></diffuse>
    <specular><color>0.2 0.2 0.2 1</color></specular>
    <shininess><float>0.25</float></shininess>
  </blinn></technique>
</profile_COMMON></effect></library_effects>)WRDAE";
    if (Val.materialsGiven())
        out << "\n<library_materials>" << Val.materials() << "</library_materials>";
    else
        out << R"WRDAE(
<library_materials><material id="material">
  <instance_effect url="#effect"/>
</material></library_materials>)WRDAE";
    out << R"WRDAE(<library_geometries><geometry id="content-lib"><mesh>)WRDAE";
    // Vertices.
    out << R"WRDAE(<source id="content-positions"><float_array id="content-positions-array" count=")WRDAE"
        << Val.vertices().size() * 3 << "\">\n";
    for (auto& vertex : Val.vertices())
        out << vertex[0] << ' ' << vertex[1] << ' ' << vertex[2] << "\n";
    out << "</float_array><technique_common><accessor count=\""
        << Val.vertices().size()
        << R"WRDAE(" source="#content-positions-array" stride="3">
<param name="X" type="float"/><param name="Y" type="float"/><param name="Z" type="float"/>
</accessor></technique_common></source>)WRDAE";
    out << R"WRDAE(
<vertices id="content-vertices"><input semantic="POSITION" source="#content-positions"/></vertices>
<triangles material="material" count=")WRDAE"
        << triangles.size()
        << R"WRDAE(">
<input offset="0" semantic="VERTEX" source="#content-vertices" set="0"/>)WRDAE";
    for (auto& tri : triangles)
        out << "<p>" << tri[0] << ' ' << tri[1] << ' ' << tri[2] << "</p>\n";
    out << R"WRDAE(</triangles></mesh></geometry></library_geometries>
<library_visual_scenes><visual_scene id="scene">
<node id="content">
  <instance_geometry url="#content-lib"><bind_material><technique_common>
    <instance_material symbol="material" target="#material"/>
  </technique_common></bind_material></instance_geometry>
</node>
</visual_scene></library_visual_scenes>
<scene><instance_visual_scene url="#scene"/></scene>
</COLLADA>)WRDAE";
    bool ok = out.good();
    out.close();
    return ok ? 0 : 2;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    InputParser<io::ParserPool, io::WriteColladaIn_Parser, io::WriteColladaIn>
        ip(f);
    int status = ip.ReadAndParse(writecollada);
    if (f)
        close(f);
    return status;
}

#else

#endif
