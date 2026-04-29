# Example: Logging

## Build

```sh
cc -I../../include -o example main.c
./example
```

## What this shows

Using the SaferCode logging system for cross-platform logging. This example demonstrates:

- Initializing the logger to console
- Setting log levels (DEBUG, INFO, WARN, ERROR)
- Logging messages at different levels
- Changing log configuration at runtime
- Using printf-style formatting in log messages
- Proper cleanup

The logging system works on Windows, Linux, macOS, and BSD,
and can log to console, files, or network sockets.
