# Compress module

gzip compression via [zlib](https://zlib.net): `gzencode` / `gzdecode`. On by default;
skips itself if zlib is missing. Needs `zlib1g-dev` (Debian/Ubuntu).

```voidscript
string $gz = gzencode("large repetitive text...");   // optional level 0-9 (default 6)
string $original = gzdecode($gz);                     // round-trips
```

`gzencode(data [, level])` produces gzip-framed bytes (write to a `.gz` file, or send with
`Content-Encoding: gzip`). `gzdecode(data)` auto-detects gzip or zlib framing. Both are
binary-safe; store the compressed bytes as-is or `hex_encode()`/`base64_encode()` them for
text transport.
