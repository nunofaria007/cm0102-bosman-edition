# Brief for research agents

## The project
A native macOS port of Championship Manager 01/02, rebuilt in C and SDL2 from the original Windows program. The port is not an emulation wrapper.

- Repo: `/Users/max/Downloads/Game/cm0102-mac`
- Original: `/Users/max/Downloads/Game/cm0102.exe` (x86 PE32, MSVC, thiscall, 9,384 procedures)
- Game data of the original: `/Users/max/Downloads/Game/Data` and `/Users/max/Downloads/Game/Pictures`
- Never copy the original or its data into the repo.

Read `GLOSSARY.md` and use its terms. Read the specs in `docs/specs/` for what the port has. `docs/research/2026-10-13-display-and-scrman-inventory.md` lists 82 functions of display.cpp and scrman.cpp with their signatures.

## Tools
Use the REA MCP tools on the original. Load them with ToolSearch (`select:mcp__rea__open_binary,mcp__rea__procedure_pseudo_code,mcp__rea__batch_decompile,mcp__rea__procedure_assembly,mcp__rea__procedure_callers,mcp__rea__procedure_callees,mcp__rea__procedure_info,mcp__rea__search_strings,mcp__rea__xrefs,mcp__rea__read_bytes`). Call `open_binary` on the exe path first.

## Traps
- Ghidra pseudocode drops the thiscall ECX and register arguments, and garbles functions with large stack frames. Read the assembly before you state a signature or say which object a call works on.
- Do not run xrefs on palette globals such as 0xAEBE04. They return thousands of addresses.
- The assert strings name the source file and line of the original. `FUN_005e8290` is the error box. Use them to tell which source file a function belongs to.
- The original structs are packed to 1 byte.

## Rules
- Research only. Write exactly one file, the one your task names. Do not change any other file in the repo. Do not commit.
- Give each fact its evidence: the address of the function or instruction that shows it.
- Separate what the code shows from what you infer. Mark an inference as an inference.
- If you cannot determine something, write "unknown". Do not guess.
- The port is for single player. Note network code, but do not analyse it in depth.
- The goal of the port is a game that plays the same. Put first what a real screen or a real saved game needs. Name what looks unreachable or unused, so that the port can leave it out.
- Final reply: at most 300 words, with the findings that matter most for a plan, and the path of your file.
- Do not run `rm` with a wildcard or on a path in a variable. A safety check stops it and asks Max, even in bypass mode. To start again with an empty work folder, make a new folder (`mktemp -d` in your scratch area) and use that.
