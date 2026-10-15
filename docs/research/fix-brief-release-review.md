# Fix brief: release review findings

Spec: `docs/specs/2026-10-14-re-signed-edition-release-spec.md`. Rules: `docs/research/build-brief-release.md` (no committed game data, no `rm` with a wildcard or a variable path, plain STE English, commit trailer). Work on your own branch. Do not push.

Fix each item as decided here. Do not redesign. If a decision proves impossible, stop and report.

1. **UTF-8 paths on Windows** (`src/recomp_rt/win/display.c` `fopen` of `settings.txt`, `src/recomp_rt/rt.c:33`). Add `FILE *host_fopen(const char *utf8_path, const char *mode)` to `src/recomp_rt/host.h`: `fopen` in `host/posix.c`, `_wfopen` after a UTF-8 to UTF-16 conversion in `host/win32.c` (reuse its existing conversion helper). Use it in both places. Grep the runtime for any other `fopen` on a host path and convert those too.
2. **Clean files manifest** (`tools/clean_files/make_clean_files.py`). The tool writes its manifest into the output folder, never over the committed `tools/clean_files/manifest.tsv`. After making the files it compares them with the committed manifest and fails with the differing paths. A flag `--update-manifest` rewrites the committed file on purpose. Either check `CAB_FILE_COUNT` or remove it.
3. **Installer** (`packaging/windows/installer.nsi`):
   - Upgrade path: in `.onInit`, `SetRegView 64`, then read the install folder from the uninstall key and use it if present. Drop `InstallDirRegKey`.
   - Folder safety: if `$INSTDIR\game` exists and `$INSTDIR\${PROGRAM}` does not, stop with a message box that the folder already holds a folder called "game" and the player must choose another folder. Only remove `$INSTDIR\game` when it is ours.
   - Saves on uninstall: the uninstaller never deletes saves and never asks. It keeps `%APPDATA%\cm0102` and its last page says where the saves are.
   - Finish page "Start the game": start it through `explorer.exe` (`MUI_FINISHPAGE_RUN_FUNCTION` with `Exec '"$WINDIR\explorer.exe" "$INSTDIR\${PROGRAM}"'`) so the game runs as the logged-in user, not the elevated installer.
   - Rebuild the installer and test install, upgrade over a custom folder and uninstall under CrossOver as the earlier slice did (scratch bottle, fan pack or `/Users/max/Projects/cm0102/clean-3968` as game files).
4. **Release script** (`tools/release/make_release.sh`):
   - Refuse to run when `git status --porcelain` shows anything, untracked files included.
   - Write `site-assets.zip` to a new temporary file and move it over the old one, so removed pictures do not stay. No `rm`.
   - Windows package check: add `tools/windows/check_installer.py` that checks the built `cm0102_recomp.exe` (PE32+ x86-64, file version resource equal to the CMake version, imports only Windows system DLLs, an icon resource present) and that the setup file name holds the version. Run it in the script after the Windows build.
5. **mac_check** (`CMakeLists.txt` near line 204): pass `--archs arm64` when no x86_64 SDL2 prefix is given.
6. **make_app.sh**: print a warning for a library only the x86_64 build has; skip the copy when a Frameworks folder is empty; replace the `eval` with a plain call.
7. **check_package.py:102**: strip a leading `./` only, not every leading dot.
8. **files.c:511**: shorten the long comment.

Run `tests/recomp/test_load.py` if you touch the runtime, and `python3 tests/smoke/smoke_run.py build/cm0102_recomp /Users/max/Projects/cm0102/clean-3968 /Users/max/Projects/cm0102/smoke-ref --mode classic` once at the end.

Final reply: at most 300 words, the branch name, the files you touched, what passed, what did not.
