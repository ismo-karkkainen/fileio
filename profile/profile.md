# Profile program specs

Programs for profiling the source code.

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

