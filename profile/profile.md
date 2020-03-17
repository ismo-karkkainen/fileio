# Profile program specs

Programs for profiling the source code.

Keep input typename the same as the included file is varied but types are expected to remain the same.

## readfloatarray

Reads an array of floats from standard input and exits.

```
---
readfloatarray:
  input:
    "-typename-": ReadSomething
    array:
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
    "-typename-": ReadSomething
    array:
      format: [ array, array, float ]
      required: true
...
```

## readstringarray

Reads an array of strings from standard input and exits.

```
---
readstringarray:
  input:
    "-typename-": ReadSomething
    array:
      format: [ array, string ]
      required: true
...
```

## readintarray

Reads an array of ints from standard input and exits.

```
---
readintarray:
  input:
    "-typename-": ReadSomething
    array:
      format: [ array, integer ]
      required: true
...
```

