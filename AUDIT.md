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
| MariaDB | connection keyed by `toString()` -> all connections shared | FIXED, VERIFIED against MariaDB 11 (docker) |
| MariaDB | connect() hardcoded port 3306; useSSL not flagged optional | FIXED (host:port parsing, useSSL optional) |
| Memcached | connection keyed by `toString()` -> all connections shared | FIXED, VERIFIED against memcached 1.6 (docker) |
| Memcached | connect() passed whole string as hostname + hardcoded port 11211 | FIXED (host:port parsing) |
| **Xml2** | `__class__` marker instead of `$class_name` -> ...fails "Object missing $class_name" | FIXED |
| **Xml2** | instance handle stored as a CLASS-level property (`setObjectProperty("XmlDocument", ...)`) -> all instances of a class share one handle | FIXED (per-object handle reads) |
| **Xml2** | `createFromString` baseUrl not flagged `optional` | FIXED |
| **Xml2** | SIGSEGV at teardown - inline-static holders destroyed out of order with `~XmlModule` (use-after-free) | FIXED (valgrind: 0 errors) |
| **Xml2** | dual doc storage left `getRootElement` unable to reach createFromString docs | FIXED (bridge via createNodeObject) |
| **Xml2** | `isWellFormed` called `xmlValidateDocument` (DTD validity), false for any DTD-less doc | FIXED |
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

- Framework `instanceId` added; Curl/MariaDB/Memcached migrated. All three collision
  fixes VERIFIED end-to-end: Curl via file://, MariaDB against MariaDB 11 and Memcached
  against memcached 1.6 (both in Docker). Verification also surfaced hardcoded-port bugs
  in both DB modules (connect could only reach the default port) - now parse host:port.
  Integration scripts in test_scripts/integration/ (not in the unattended ctest suite).
- Imagick: composite + mode registered, crop/blur/rotate/flip revived, instance
  collision fixed (prior commits).
- Xml2: fully fixed. $class_name marker + per-object handle reads (dispatch + document
  independence), teardown SIGSEGV (holders no longer touched by ~XmlModule; valgrind 0
  errors), getRootElement bridges createFromString documents, isWellFormed corrected,
  baseUrl made optional. XmlDocument given a proper Rule of Five. Regression:
  xml_independence.vs. Two document systems (docHolder/documentHolder) still coexist but
  now interoperate; collapsing them to one is future cleanup, not a bug.
