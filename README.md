# ImageIO

Programs to read image files into arrays for use with datalackey. Programs
to write image-like data from JSON arrays into images.

## readimage

Reads image file (provided it recognizes the format) from given file and
outputs as JSON array to standard output. The optional minimum and maximum
result in shift and/or scaling of the values in output. If not given, the
values are output as they are.

```
---
readimage:
  input:
    filename:
      description: File name string.
      format: string
      required: true
    format:
      description: File format, determined from filename if not given.
      format: string
      required: false
    minimum:
      description: Map smallest possible value in image file to this value.
      format: float
      required: false
    maximum:
      description: Map largest possible value in image file to this value.
      format: float
      required: false
  output:
    image:
      description: Width * height * color planes array in [minimum, maximum].
      format: array
      required: true
...
```

## writeimage

Writes the given width * height * color planes image in given format to a file.
Number of color planes may be limited by the desired output format. The
optional minimum and maximum indicate what the real range of values is in the
input image. That range is scaled and shifted to cover the output format
precision. Useful to keep several images in same range with respect to each
other.

```
---
writeimage:
  input:
    "-typename-": WriteImageIO
    filename:
      description: File name string.
      format: string
      required: true
    format:
      description: |
        File format, determined from file name if not given.
        Supported formats are TIFF and PNG.
      format: string
      required: false
    image:
      description: Width * height * color planes array.
      format: [ array, array, array, float ]
      required: true
    depth:
      description: |
        Desired bit depth. Rounded up to nearest supported or maximum 16.
        Currently 8 and 16 are possible.
      format: integer
      required: false
    minimum:
      description: Minimum value for range of values in input image.
      format: float
      required: false
    maximum:
      description: Maximum value for range of values in input image.
      format: float
      required: false
...
```

# Requirements

For unit tests, you need https://github.com/onqtam/doctest to compile the
sources. Even if you do not need to run the tests, you still need it for
definitions on the test macros that are located around the source code.

Personally I installed doctest/doctest.h under /usr/local/include but wherever
the include directive in common/testdoc.h finds the file is ok.

# Building

You need cmake and a C++ compiler that supports 2017 standard. Assuming a build
directory parallel to the imageio directory, you can use:

    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=DEBUG ../imageio
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE ../imageio
    cmake -G Xcode

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
