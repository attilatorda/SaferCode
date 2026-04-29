# Example: Reference Tracker

## Build

```sh
cc -I../../include -o example main.c
./example
```

## What this shows

Using the reference tracker to prevent use-after-free bugs. This example demonstrates:

- Creating a reference tracker within an object
- Registering multiple references to an object
- Automatically nullifying all references when the object is destroyed
- How this prevents dangling pointer bugs

The reference tracker automatically nullifies all registered pointers
when `sc_reftracker_clear()` is called, preventing any subsequent access
to the freed object.
