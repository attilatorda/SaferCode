# Example: CSV to JSON

Build:

```sh
cc -I../../include -o example main.c
./example
```

Purpose: convert simple CSV rows (`name,age,city`) into JSON lines using
`sc_string_builder` for safe incremental output construction.
