---
name: feedback_pdo_odbc_quirks
description: ADICTICallSystem.API runs on pdo_odbc (no pdo_sqlsrv) — several non-obvious binding quirks to watch for in every new controller
metadata:
  type: feedback
---

This machine's PHP 7.4 build has no `pdo_sqlsrv`; `ADICTICallSystem.API` connects to SQL Server via `pdo_odbc` + "ODBC Driver 18 for SQL Server" (see `config/config.php`'s `db.driver`). That driver combination has several distinct gotchas that have each independently caused real bugs (found while building `MachineController`/`CustomerController`/`EmployeeController`/`CallRecordController`). Check for all of these whenever writing or reviewing a new PDO query in this API:

1. **Non-ASCII text needs `Database::bindText()`/`decodeText()`, not plain `bindValue()`.** `pdo_odbc` binds strings via the OS ANSI codepage (Big5/950 on this box), not UTF-8. Any bound value that might contain Chinese — including `LIKE` search patterns, not just exact inserts — silently corrupts or never matches unless routed through `Database::bindText()` on write and `Database::decodeText()` on read.
2. **Never mix manual `bindValue()`/`bindParam()` with a subsequent `execute($array)` in the same statement.** `pdo_odbc` emulates named parameters by position internally; mixing styles silently misaligns them (a real case: `alias` ended up holding `machine_type`'s value). Use `Controller::executeWithParams($stmt, $params, $textFields)` instead — it binds everything explicitly and calls `execute()` with no args.
3. **`execute($array)` doesn't reliably send NULL for PHP `null` values** in this array-based path — it can send an empty string instead, which fails to cast into `INT`/`DATETIME2` columns ("Invalid character value for cast specification"). `executeWithParams()` fixes this too (explicit `PARAM_NULL` when the value is null).
4. **The same named parameter can't be reused twice in one query** (e.g. one `:x` bound to three different `LIKE` clauses) — `pdo_odbc` has no native named-parameter support, so reuse desyncs positional binding for every parameter that follows, including `:offset`/`:limit` in paginated queries (surfaces as "FETCH row count must be >= 0"). Use distinct parameter names (`:x1`, `:x2`, `:x3`) bound to the same value instead.
5. **Datetime strings must be `Y-m-d H:i:s` (space separator), not ISO 8601 `Y-m-d\TH:i:s\Z`.** This ODBC driver rejects the `T`/`Z` separators outright on implicit string→`DATETIME2` cast.

Why this matters: each of these has been independently rediscovered from scratch across different controllers in the same session — they're easy to reintroduce in a new endpoint if not checked explicitly. See `doc/OperatorWeb-說明.md`'s "重寫過程中一併修好的問題" section for the concrete bugs/fixes, and `doc/ADICTICallSystem.API-說明書.md`'s 2026-07-01 update for the original `bindText`/`decodeText` writeup.
