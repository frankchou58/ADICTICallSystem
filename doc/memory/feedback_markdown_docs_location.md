---
name: feedback-markdown-docs-location
description: All markdown documentation files in ADICTICallSystem must live under the doc/ directory
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 1d57ebf4-e5cf-4f12-a395-d18e8bd4b5fa
---

Put every markdown (.md) file in this project under `D:\ADICTI\ADICTICallSystem\doc\`, not next to the code they document (e.g. not in `ADICTICallSystem.API/README.md`).

**Why:** User explicitly asked to centralize project documentation in the existing `doc/` folder rather than scattering README files across each sub-project.

**How to apply:** Whenever creating or moving a `.md` file anywhere under `D:\ADICTI\ADICTICallSystem\`, place it in `doc/` (create a subfolder there if needed to disambiguate multiple projects, e.g. `doc/ADICTICallSystem.API-README.md` or `doc/ADICTICallSystem.API/README.md`) instead of inside the project's own directory tree.

**Memory files too:** the user also asked for a visible copy of this Claude memory system itself under `D:\ADICTI\ADICTICallSystem\doc\memory\` (mirroring `MEMORY.md` and each individual memory file). The authoritative copy stays in the system memory path (`C:\Users\user\.claude\projects\d--ADICTI-ADICTICallSystem\memory\`) so auto-recall keeps working — whenever a memory file is created/edited/deleted here, mirror the same change into `doc/memory/` in the project.
