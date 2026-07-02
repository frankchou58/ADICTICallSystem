---
name: feedback-utf8-bom-mfc-files
description: New C/C++ files written into the ADICTICallCenter MFC project must have a UTF-8 BOM if they contain non-ASCII (Chinese) text
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 1d57ebf4-e5cf-4f12-a395-d18e8bd4b5fa
---

When creating or fully rewriting a `.h`/`.cpp` file in the `ADICTICallCenter` MFC project (both `D:\ADICTI\ADICTICallCenter\` and `D:\ADICTI\ADICTICallSystem\ADICTICallCenter\`) that contains any Chinese/non-ASCII characters (e.g. Chinese comments), prepend a UTF-8 BOM (`EF BB BF`) to the file before considering the write done.

**Why:** All existing files in this project are saved with a UTF-8 BOM. Files written without one still contain valid UTF-8 bytes, but MSVC has no way to know that — it falls back to the system ANSI code page (950 / Big5) to decode the file. Multi-byte UTF-8 sequences in Chinese comments get misdecoded as Big5, corrupting the perceived byte stream. This produces bizarre, seemingly-random compiler errors (`C2447 '{': missing function header`, `C2065: undeclared identifier`) at line numbers that don't match the real file content — very hard to diagnose unless you notice the accompanying `warning C4819: the file contains characters that cannot be represented in the current code page`. Confirmed by comparing the first bytes of a working file (`EF BB BF 0D 0A ...`) against a newly-written one (no BOM) and finding C4819 warnings pointing at the new file.

**How to apply:** After using Write to create/replace a file with Chinese content in this project, check the first bytes (e.g. `xxd file | head -1`) for `efbb bf`; if missing, prepend it (e.g. `printf '\xEF\xBB\xBF' | cat - file > file.new && mv file.new file`). ASCII-only files (no Chinese text) don't need this — plain ASCII decodes identically under UTF-8 and Big5/ANSI.

See [[ADICTICallCenter-MFC改版說明]] doc for the full incident writeup.
