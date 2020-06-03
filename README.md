# ImageIO

Programs to read image files into arrays for use with datalackey. Programs
to write image-like data from JSON arrays into images.

## readimage

Reads image file from given file and outputs as JSON array to standard output.
The optional minimum and maximum result in shift and/or scaling of the values
in output. If not given, the values are output as they are.

Supported formats are PPM (P6-PPM), P3-PPM (text), TIFF (via libtiff), PNG
(via libpng).

```
---
readimage_io:
  namespace: readio
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
        description: |
          Height * width * components array in [minimum, maximum].
          Present only if error is not, and vice versa.
        format: [ ContainerStdVector, ContainerStdVector, StdVector, Float ]
        required: false
        checker: "error.size() == 0"
        accessor: image
      error:
        description: Error string if reading the image fails.
        format: String
        required: false
        checker: "error.size() != 0"
        accessor: error
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

```
---
writeimage_io:
  namespace: writeio
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
        format: [ ContainerStdVector, ContainerStdVector, StdVector, Float ]
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
length as others. If not, output object has error field with message.

```
---
split2planes_io:
  namespace: io
  types:
    Split2PlanesIn:
      planes:
        description: Array of arrays of arrays of floats.
        format: [ ContainerStdVector, ContainerStdVector, StdVector, Float ]
  generate:
    Split2PlanesIn:
      parser: true
...
```

# Building

For TIFF support you need the libraries and development files. Same for PNG.

You need edicta to extract the input and output specifications from this file.
You need specificjson to generate source code from the specifications.

    https://github.com/ismo-karkkainen/edicta
    https://github.com/ismo-karkkainen/specificjson

For unit tests, you need https://github.com/onqtam/doctest to compile them.
Install into location for which `#include <doctest/doctest.h>` works.

You need cmake and a C++ compiler that supports 2017 standard. Assuming a build
directory parallel to the imageio directory, you can use:

    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=DEBUG ../imageio
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE ../imageio
    cmake -G Xcode

You can disable TIFF support by setting NO_TIFF to any value, for example:
NO_TIFF=1 cmake ... To disable PNG support, set NO_PNG=1 when running cmake.

To specify the compiler, set for example:

    CXX=clang++
    CXX=g++

To build, assuming Unix Makefiles:

    make
    make test
    sudo make install

To run unit tests and to see the output you can "make unittest" and then run
the resulting executable.

# License

Copyright (C) 2020 Ismo Kärkkäinen

Licensed under Universal Permissive License. See License.txt.
