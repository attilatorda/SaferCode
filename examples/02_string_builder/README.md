# Example: String Builder

## Build

```sh
cc -I../../include -o example main.c
./example
```

## What this shows

Using the string builder for efficient string construction. This example demonstrates:

- Creating a string builder
- Appending strings with `sc_sb_append_cstr()`
- Appending individual characters
- Formatting with `sc_sb_append_printf()`
- Finishing the builder to get an `SC_String`
- Proper cleanup

String builders use amortized O(1) appends by growing a buffer
exponentially, avoiding the overhead of many individual allocations.
