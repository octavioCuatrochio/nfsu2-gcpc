# NFSU2 (GameCube, Speed.elf) decompilation tooling

Everything used to build `../NFSU2.rep` and export `../gc_decomp/` / `../pc_decomp/`.

- `extensions/dw1-emoose/`: DWARF v1 analyzer (fork of rafalh/ghidra-dwarf1), with 20+ parser fixes for this GCC 2.95 SN Systems binary. Not vendored: recreate from upstream + `extensions/patches/`.
- `extensions/gc-loader/`: GameCube loader / Gekko_Broadway CPU definition, patched for Ghidra 12.2 (Gradle 9 build, demangler field fix) and with PowerPC EABI callee-saved registers added to the cspec (`*.cspec.orig` is the original). Not vendored: recreate from upstream + `extensions/patches/`.
- `scripts/`: Ghidra scripts. The pipeline order is in `rebuild.sh`. `ExportDecomp.java` writes one .c file per class/namespace, `types.h`, and `functions.tsv`. The rest are diagnostics from development.
- `scripts/expected_layouts.json`: struct layouts read directly from the DWARF, used by `VerifyLayouts.java`.
- `logs/final_build.log`: log of the build that produced the current project.

## Get, build and install the extensions (Ghidra 12.2, needs Java 25+)
Upstream repos and pinned commits are in `extensions/patches/README.md`.

    export GHIDRA=/path/to/ghidra_12.2_PUBLIC
    cd extensions
    git clone https://github.com/emoose/ghidra-dwarf1 dw1-emoose && git -C dw1-emoose checkout 5b0666c && git -C dw1-emoose apply ../patches/dw1-emoose.patch
    git clone https://github.com/Cuyler36/Ghidra-GameCube-Loader gc-loader && git -C gc-loader checkout 921504c && git -C gc-loader apply ../patches/gc-loader.patch
    for d in dw1-emoose gc-loader; do (cd $d && "$GHIDRA/support/gradle/gradlew" -PGHIDRA_INSTALL_DIR="$GHIDRA" buildExtension); done
    unzip -o dw1-emoose/dist/*.zip -d ~/.config/ghidra/ghidra_12.2_PUBLIC/Extensions
    unzip -o gc-loader/dist/*.zip  -d ~/.config/ghidra/ghidra_12.2_PUBLIC/Extensions

## Rebuild project + export
    ./rebuild.sh     # about 1-1.5 h; output in tooling/out/
