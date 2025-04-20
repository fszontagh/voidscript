        1. Collapse the “namespace‑map” indirection
           • Right now every call to peek(), advance(), isAtEnd(), etc. does
             lookup(currentScopeName) → positions_[ns] (and inputs_[ns], line_numbers_[ns], …).
           • Instead, when you enter a namespace/input, snapshot pointers/references to
             the string, the current pos/line/col ints and use those directly.
           • Eliminates a handful of std::unordered_map hashes on every character.
        2. Switch from index‑based to pointer‑based scanning
           • Keep a `const char* cur` and `const char* end` instead of `size_t pos` + `.at()` bounds checks.
           • You can advance with `*cur++` and test `cur < end` in pure pointer arithmetic.
        3. Turn your operator/keyword lookups into O(1) tables
           • Right now you have
           – `operators_.find(c) != npos` (linear search through a string)
           – and for two‑char ops you build a `std::string(two_chars_str)` + `find()` on a small vector.
           • Instead:
           – Build a 256‑entry `std::bitset<256>` (or `bool isOpChar[256]`) for fast single‑char lookup.
           – For multi‑char operators, read the next two chars into a uint16 or small stack buffer and do
             a single switch or an `unordered_map<uint16_t,TokenType>` lookup. Fall back to the 1‑char table.
        4. Avoid needless heap allocations on every token
           • `matchIdentifierOrKeyword` does `input().substr(...)` to test for keyword → allocates a new string.
           • You can instead use `std::string_view` slices (and make your keyword map `unordered_map<string_view,Type>`).
           • Similarly, only materialize `token.value` as a `std::string` when you really need it (e.g. permanently storing).
        5. Inline and flatten the hot loops
           • Make your small helpers (peek, advance, isAtEnd) `inline` or even macros in a private detail header.
           • Fuse `skipWhitespace` and comment skipping into a single loop; remove extra function calls.
        6. (Optional) Pre‑generate a DFA/trie for identifiers vs. numbers vs. operators
           • If you’re really pushing for speed you can switch to a table‑driven state machine (Flex style) and
             generate the transition table.  That’s extra complexity, but you’ll get maximum raw throughput.
