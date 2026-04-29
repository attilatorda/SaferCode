# Example: Word Wrapper

Build:

```sh
cc -I../../include -o example main.c
./example
```

Purpose: wrap long text into fixed-width lines and emit the wrapped output
using incremental `sc_string_builder` appends.
