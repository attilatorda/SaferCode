# Example: URL Parser

Build:

```sh
cc -I../../include -o example main.c
./example
```

Purpose: parse a URL into `scheme`, `host`, and `path` components using
arena-backed temporary allocations.

Expected output includes parsed components for:

- `https://example.com/docs/index.html`
