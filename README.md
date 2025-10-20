# FileIO

Programs to read image files into arrays for use with datalackey. Programs
to write image-like data from JSON arrays into images. Programs to write a 3D
model in GLB, glTF and COLLADA format. Other related tools.

The YAML in code blocks are I/O specifications for specificjson, extracted
using edicta. See repositories parallel to this one.

## readimage

Reads image file from given file and outputs as JSON array to standard output.
The optional minimum and maximum result in shift and/or scaling of the values
in output. If not given, the values are output as they are.

Supported formats are PPM (P6-PPM), P3-PPM (text), TIFF (via libtiff), PNG
(via libpng).

```YAML
---
readimage_io:
  namespace: io
  requires:
  - Int32
  types:
    ReadImageIn:
      filename:
        description: File name string.
        format: String
      format:
        description: File format, determined from filename if not given.
        format: String
        required: false
      minimum:
        description: Map smallest possible value in image file to this value.
        format: Float
        required: false
      maximum:
        description: Map largest possible value in image file to this value.
        format: Float
        required: false
      shift:
        description: |
          Value to add to integer values to ensure truncation or rounding does
          not cause values to shift if processing introduces unintended changes.
          For rounding, 0 is ok, for truncation 0.5, and 0.25 works for both.
          A side effect is that pixel value 0 will not be 0 in unscaled output.
          Used only when minimum or maximum are given.
        format: Float
        required: false
    ReadImageOut:
      image:
        description: Height * width * components array in [minimum, maximum].
        format: [ ContainerStdVector, ContainerStdVector, StdVector, Float ]
        accessor: image
  generate:
    ReadImageIn:
      parser: true
    ReadImageOut:
      writer: true
...
```

## writeimage

Writes the given width * height * components image in given format to a file.
Number of components may be limited by the output format. The optional minimum
and maximum indicate what the real range of values is in the input image. That
range is scaled and shifted to cover the output format precision. Useful to
keep several images in same range with respect to each other.

Supported formats are (P6-)PPM, P3-PPM, TIFF (via libtiff) and PNG (via libpng).
Compression is not used.

```YAML
---
writeimage_io:
  namespace: io
  types:
    WriteImageIn:
      filename:
        description: File name string.
        format: String
      format:
        description: File format, determined from file name if not given.
        format: String
        required: false
      image:
        description: Height * width * components array.
        format: [ ContainerStdVectorEqSize, ContainerStdVectorEqSize, StdVector, Float ]
      depth:
        description: |
          Desired bit depth. Rounded up to nearest supported or maximum 16.
          Currently 8 and 16 are possible, except P3 supports 1 to 16.
        format: Int32
        required: false
      minimum:
        description: Minimum value for range of values in input image.
        format: Float
        required: false
      maximum:
        description: Maximum value for range of values in input image.
        format: Float
        required: false
  generate:
    WriteImageIn:
      parser: true
...
```

## split2planes

Splits the third dimension and outputs multiple separate arrays of arrays of
floats, named plane0, plane1, ... until all components of the third dimension
are used. Each array representing the third dimension must have the same
length as others.

```YAML
---
split2planes_io:
  namespace: io
  types:
    Split2PlanesIn:
      planes:
        description: Array of arrays of arrays of floats.
        format: [ ContainerStdVector, ContainerStdVectorEqSize, StdVector, Float ]
  generate:
    Split2PlanesIn:
      parser: true
...
```

## writegltf

Writes given 3D model information as glTF file.

```YAML
---
writegltf_io:
  namespace: io
  types:
    WriteglTFIn:
      filename:
        description: Output file name. ".gltf" is appended unless ends with it.
        format: String
      vertices:
        description: Array of arrays of 3 float x, y, and z coordinates.
        format: [ ContainerStdVectorEqSize, StdVector, Float ]
      colors:
        description: |
          Array of arrays of 3 float red, green, and blue values. Has to match
          vertices in order and size.
        format: [ ContainerStdVectorEqSize, StdVector, Float ]
        required: false
      tristrips:
        description: Array of arrays of indexes to top-level vertices array.
        format: [ ContainerStdVector, StdVector, UInt32 ]
  generate:
    WriteglTFIn:
      parser: true
...
```

## writeglb

Writes given 3D model information as a binary glTF file.

```YAML
---
writeglb_io:
  namespace: io
  types:
    WriteGLBIn:
      filename:
        description: Output file name. ".glb" is appended unless ends with it.
        format: String
      vertices:
        description: Array of arrays of 3 float x, y, and z coordinates.
        format: [ ContainerStdVectorEqSize, StdVector, Float ]
      coordinates:
        description: Array of arrays of 2 float texture coordinate values.
        format: [ ContainerStdVectorEqSize, StdVector, Float ]
        required: false
      texture:
        description: Image that represents texture.
        format: [ ContainerStdVectorEqSize, ContainerStdVectorEqSize, StdVector, Float ]
        required: false
      tristrips:
        description: Array of arrays of indexes to top-level vertices array.
        format: [ ContainerStdVector, StdVector, UInt32 ]
  generate:
    WriteGLBIn:
      parser: true
...
```

## writecollada

Writes given 3D model information as COLLADA file.

```YAML
---
writecollada_io:
  namespace: io
  types:
    WriteColladaIn:
      filename:
        description: Output file name. ".dae" is appended unless ends with it.
        format: String
      vertices:
        description: Array of arrays of 3 float x, y, and z coordinates.
        format: [ ContainerStdVectorEqSize, StdVector, Float ]
      tristrips:
        description: Array of arrays of indexes to top-level vertices array.
        format: [ ContainerStdVector, StdVector, UInt32 ]
      asset:
        description: asset element contents (child elements). Output as is.
        format: String
        required: false
      effects:
        description: |
          library_effects element contents. Output as is. Use id "effect".
        format: String
        required: false
      materials:
        description: |
          library_materials element contents. Output as is. Use id "material".
        format: String
        required: false
  generate:
    WriteColladaIn:
      parser: true
...
```

# Building

For TIFF support you need the libraries and development files. Same for PNG.

You need edicta to extract the input and output specifications from this file.
You need specificjson to generate source code from the specifications.

    gem install edicta
    gem install specificjson

For unit tests, https://github.com/onqtam/doctest is included as git subtree
with prefix doctest.

You need cmake and a C++ compiler that supports 2017 standard. Assuming a build
directory parallel to the fileio directory, you can use:

    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ../fileio
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../fileio
    cmake -G Xcode

You can disable TIFF support by setting `NO_TIFF` to any value, for example:
`NO_TIFF=1 cmake ...` To disable PNG support, set `NO_PNG=1` when running cmake.

To specify the compiler, set for example:

    CXX=clang++
    CXX=g++

To build, assuming Unix Makefiles:

    make
    make test
    sudo make install

To run unit tests and to see the output you can `make unittest` and then run
the resulting executable.

# License

Copyright © 2020-2025 Ismo Kärkkäinen

Licensed under Universal Permissive License. See License.txt.
