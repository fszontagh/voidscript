# Module audit - findings

A deliberate sweep of all 25 modules (16 built-in, 9 dynamic) for the issue classes
found incidentally during earlier work: object-identity collisions, dead code, and
crash-on-bad-input.

## Root cause (framework)

**Object identity by serialised contents.** Modules keyed per-instance state on
`args[0].toString()`, which returns the object's serialised *contents*. A freshly
constructed instance is empty but for its `$class_name`, so every instance produces the
same key and they all collide onto one entry. This has now been found in five modules.
The fix is a framework-level per-instance id (see below), after which modules key on
identity, not contents.

## Findings

| Module | Finding | Status |
|---|---|---|
| (interpreter) | `5 / 0`, `5 % 0` on ints crashed with SIGFPE | FIXED (prior) |
| (interpreter) | method-call depth counter leaked, capped programs at ~100 calls | FIXED (prior) |
| DateTime | timestamp keyed by `toString()` -> all instances shared one | FIXED (#24) |
| Imagick | composite unregistered; crop/mode/blur/rotate/flip dead; instance collision; mode() unregistered | FIXED (prior) |
| Curl | CurlResponse/CurlClient collision; `curlPost/Put/Delete` unregistered; `__class__` marker | FIXED (prior) |
| MariaDB | connection keyed by `toString()` -> all connections shared | FIXED, compile-only (needs server) |
| Memcached | connection keyed by `toString()` -> all connections shared | FIXED, compile-only (needs server) |
| **Xml2** | `__class__` marker instead of `$class_name` -> ...fails "Object missing $class_name" | FIXED |
| **Xml2** | instance handle stored as a CLASS-level property (`setObjectProperty("XmlDocument", ...)`) -> all instances of a class share one handle | FIXED (per-object handle reads) |
| **Xml2** | `createFromString` baseUrl documented optional but not flagged `optional` | minor, open |
| **Xml2** | SIGSEGV at process teardown (`~XmlDocument`->`xmlFreeDoc` during static destruction) - PRE-EXISTING, reproduces on the original code | open, needs valgrind (task) |
| Archive | none | clean |
| Hash | none | clean |
| Format | `toString()` used for output, not identity | clean |
| MongoDb | no per-object state maps | clean |
| built-ins (String, Conversion, ...) | `string_substr` / conversions on bad input throw a *catchable* error, not a crash | clean |

## Framework fix

Added `Symbols::ValuePtr::instanceId(obj)`: a process-unique, stable integer for a
class/object instance, lazily stamping `__instance_id__` on first use. Curl, MariaDB and
Memcached now key on it (their local `*_oid_` helpers are gone).

## Outcome

- Framework `instanceId` added; Curl/MariaDB/Memcached migrated. Curl collision fixed
  and tested; MariaDB/Memcached compile-verified.
- Imagick: composite + mode registered, crop/blur/rotate/flip revived, instance
  collision fixed (prior commits).
- Xml2: `$class_name` marker and per-object handle reads fixed - the ~76 per-object
  methods now work and are independent (verified: two documents serialise to their own
  content, no crosstalk). Still open: a PRE-EXISTING teardown SIGSEGV in the document
  ownership model, the dual doc-storage systems (docHolder vs documentHolder) that leave
  `getRootElement` unable to bridge, and `isWellFormed`. Filed as a task.
- `XmlDocument` given a proper Rule of Five (was a copyable owning handle).
