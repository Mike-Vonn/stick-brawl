## Security
NEVER access the ~/.ssh/ directory. Do not read, list, or cat any files in ~/.ssh/. No exceptions.

Never add Co-Authored-By lines to git commits.
Always show the proposed commit message and list of files to be committed to the user for approval before committing.
Never push to remote without explicitly asking the user first and getting approval. Always ask before running git push.
Never commit directly to main/master. Always work in a feature or dev branch and merge to main when the user approves. If on main, create or switch to a branch (often `dev`) before making changes.

## design/ Directory Convention

Every code repository must maintain a `design/` directory committed to the repo. This directory contains:

- **design.md** (required) — a detailed description of the system design. Must be kept up to date as the design changes or shifts.
- **Additional *.md files** as needed for specific topics (e.g., protocols, subsystems).
- **ToDo.md** tracking outstanding features, bug fixes, issues, and other work items.
- **Done.md** — When a ToDo item is resolved, move it from ToDo.md to Done.md with a brief note on what was done and why. This preserves the history so resolved bugs are not reintroduced.
- When a bug fix reveals a design constraint or architectural rule, update **design.md** to document it. Future sessions should understand *why* the code is the way it is.
- If a bug recurs after being fixed, that signals a need for a **test** that catches the regression. Write a test that fails without the fix and passes with it.

When starting work on a new or existing repo, check for this directory and create it if missing.

### Design Document Awareness

- **Before planning or executing work**, review `design/design.md`, `design/TODO.md`, and `design/Done.md`. Consider existing decisions, invariants, and patterns when making changes.
- **After completing work**, update all three docs as needed: new decisions/invariants in design.md, resolved items moved to Done.md, new items added to TODO.md.
- **When merging commits** (from remote, other branches, PRs), critically evaluate each commit against `design/design.md` and `design/TODO.md`. Discuss implications, conflicts, or design tensions with the user before merging.

### ToDo.md Formatting

**Sections** — Three sections, in order: **Bugs**, **Improvements**, **Features**

**Item format** — Each item is a bullet with a bold prefix label:
```
- **B1.** **Short title** — Description with file/line references.
```
- Bugs: `B#` (B1, B2, ...), Improvements: `I#` (I1, I2, ...), Features: `F#` (F1, F2, ...)

**Next-slot marker** — Each section ends with `*Next: B#*` (italicized) showing the next available number. When adding a new item, use this number and increment the marker.

**Permanent numbering** — Numbers are never reused. When an item is resolved, move it to `Done.md` with a brief note on the fix. Do not renumber remaining items.

**Referencing items in conversation:**
1. **First mention in a conversation** — Number + title + details
2. **After 5+ minute lull** — Number + title only (details omitted)
3. **Repeated references** in active discussion — Number only

**Commit messages** — Do NOT put B#/I#/F# labels in git commit messages. These labels are internal working references only.
