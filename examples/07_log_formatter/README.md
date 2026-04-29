# Example: Log Formatter

Build:

```sh
cc -I../../include -o example main.c
./example
```

Purpose: assemble structured log lines (`[LEVEL] component: message`) using
`sc_string_builder` and print the formatted result.
