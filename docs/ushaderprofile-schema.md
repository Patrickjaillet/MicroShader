# `.ushaderprofile` format

A `.ushaderprofile` file is what "Save profile..." writes and "Load
profile..." reads (`ui/golf_controls.cpp`, `ui/golf_profile.cpp`): the
current pass toggles, the protected-names list, and the selected
budget preset, saved as a single JSON object with a `.ushaderprofile`
extension.

This document, together with
[`ushaderprofile.schema.json`](ushaderprofile.schema.json) (a
[JSON Schema](https://json-schema.org/) draft 2020-12 document), is
the canonical, published spec for the format, so external tooling —
scripts, a shader-showdown organizer's build pipeline, an editor
plugin — can generate or consume `.ushaderprofile` files without
reverse-engineering `golf_profile.cpp`.

## Example

```json
{
  "schema_version": 1,
  "aggressive": true,
  "eliminate_dead_locals": true,
  "eliminate_dead_stores": true,
  "fold_constants": false,
  "reduce_constant_vectors": true,
  "strip_trailing_void_return": false,
  "compound_assignments": true,
  "increment_decrement": false,
  "ternary_from_if_else": true,
  "merge_declarations": false,
  "strip_redundant_braces": true,
  "strip_redundant_parens": false,
  "strip_duplicate_precision": true,
  "eliminate_dead_functions": false,
  "inline_single_call_functions": true,
  "simplify_algebraic_identities": false,
  "eliminate_common_subexpressions": true,
  "frequency_aware_renaming": false,
  "protected_names": "iTime,iResolution,mainImage",
  "budget_preset": "JS13K-style 13KB"
}
```

(`fixtures/sample.ushaderprofile` in the repository is this exact
file, and is what `tests/golf_profile_roundtrip_test.cpp` loads and
round-trips against the real Rust golfing engine.)

## Fields

| Field | Type | Meaning |
| --- | --- | --- |
| `schema_version` | integer | See [Schema versioning](#schema-versioning) below. |
| `aggressive` | boolean | Master toggle. When `false`, every pass toggle below is ignored and only the always-on baseline (identifier renaming, numeric literal shortening, whitespace stripping) runs. |
| `eliminate_dead_locals` … `eliminate_common_subexpressions` | boolean | The 16 individually-toggleable aggressive passes, one field per pass, using the same field names as the corresponding `GolfPassToggles` struct member in `ui/golf_controls.h`. |
| `frequency_aware_renaming` | boolean | Opt-in Phase 29.1 rename heuristic. When `true`, the engine tries a deterministic compression-aware identifier mapping and keeps it only if the final post-pass DEFLATE estimate is strictly better than the historical rename order. Missing field defaults to `false`. |
| `factor_repeated_vector_args` | boolean | Phase 29.3 pass: collapses `vecN(a,a,...,a)` constructor calls whose arguments are all the same pure identifier expression down to `vecN(a)`. Introduced in schema version 2. Missing field defaults to `true`. |
| `swizzle_alphabet` | integer | Phase 29.2 swizzle-letter-alphabet choice: `0` = Auto (try `.xyzw`/`.rgba`/`.stpq` against the DEFLATE estimator and keep the smallest), `1` = `.xyzw`, `2` = `.rgba`, `3` = `.stpq`. Introduced in schema version 2. Missing field defaults to `0` (Auto). |
| `fuse_statement_sequences` | boolean | Phase 30.3 pass: fuses a maximal run of two or more adjacent assignment/increment-decrement/call expression-statements in the same block into a single comma-operator statement. Introduced in schema version 3. Missing field defaults to `true`. |
| `protected_names` | string | Comma-separated identifiers the golfing engine must never rename. Entries are trimmed of surrounding whitespace; empty entries (e.g. a trailing comma) are dropped. May be `""`. |
| `budget_preset` | string | The name of one of the built-in size-budget presets in `ui/budget_presets.cpp` (`Shadertoy`, `X/Twitter shader`, `JS13K-style 13KB`, `4KB intro`, `8KB intro`, `64KB intro`). An unrecognized or missing value falls back to no preset selected ("Custom") rather than a load failure. |

Field order is not significant. Whitespace and indentation are not
significant — `golf_profile.cpp`'s own writer always emits the layout
shown above, but its reader (`find_field_slice`) locates each field by
key, not position.

## Compatibility rules

- **Unknown fields are ignored.** A reader that sees a key it doesn't
  recognize (a future field, or one written by a newer uShader) must
  skip it rather than fail. This is how `golf_profile.cpp` itself
  behaves, and `ushaderprofile.schema.json` sets
  `"additionalProperties": true` to match.
- **Missing optional fields fall back to a default**, documented per
  field above (`budget_preset` → "Custom"; `protected_names` → `""`;
  `schema_version` → `1`, see below). A missing field that the schema
  marks `required` means the file isn't a valid `.ushaderprofile` and
  should be rejected the same way `deserialize_golf_profile()` does —
  it only requires finding a `{` to attempt a parse, but a field
  that's absent reads back as its boolean/string zero value, which
  for a required pass toggle is indistinguishable from an explicit
  `false`.
- **`.ushaderprofile` is JSON**, but deliberately not validated
  against the schema at load time by uShader itself — the hand-rolled
  reader in `golf_profile.cpp` (no JSON library, per the Offline-First
  Isolation / embedded-only convention) is intentionally permissive.
  The published schema is for *external* tooling that wants stricter
  validation than uShader enforces on itself.

## Schema versioning

`schema_version` was introduced in schema version 1 itself (Phase 21
of `ROADMAP.md`); every `.ushaderprofile` written before Phase 21
simply omits the field. A reader — uShader's own, or third-party
tooling built against this document — must treat a missing
`schema_version` as `1`, not as an error: `golf_profile.cpp` never
required the field to exist, so a pre-Phase-21 profile file remains a
perfectly valid schema-version-1 file, just without the field spelled
out.

| `schema_version` | uShader release | Notes |
| --- | --- | --- |
| *(absent)* | ≤ 2.1.0 | Equivalent to `1`. No `schema_version` field was written. |
| `1` | 2.2.0+ | Adds the `schema_version` field itself; no other field changed shape or meaning. |
| `2` | Phase 29.2/29.3 | Adds `factor_repeated_vector_args` and `swizzle_alphabet`. Both are purely additive/optional fields (a profile without them behaves exactly as before), bumped anyway per this project's own convention of bumping on every new pass-toggle field, stricter than the "purely additive fields don't need a bump" rule below. |
| `3` | Phase 30.3 | Adds `fuse_statement_sequences`, for the same reason as version 2 above. Current format. |

Future format changes that add a required field, change a field's
type, or change what a value means (as opposed to purely additive,
ignorable fields) should bump `schema_version` and add a row to the
table above, together with a matching update to
`ushaderprofile.schema.json` (its `required` list and/or per-field
`type`) and to this document. Purely additive optional fields do not
need a version bump, per the "unknown fields are ignored" rule above.
`frequency_aware_renaming` is the first such additive optional field.
