# Profile program specs

Programs for profiling the source code.

Keep input typename the same as the included file is varied but types are expected to remain the same.

## readfloatarray

Reads an array of floats from standard input and exits.

```
---
readfloatarray:
  input:
    "-typename-": ReadFloatArrayIO
    array:
      description: Arbitrary length array of floats.
      format: [ array, float ]
      required: true
...
```

## readfloatplane

Reads an array of an array of floats from standard input and exits.

```
---
readfloatarray2:
  input:
    "-typename-": ReadFloatArrayIO
    array:
      description: Arbitrary length array of floats.
      format: [ array, array, float ]
      required: true
...
```

