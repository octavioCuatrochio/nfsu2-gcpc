# NFSU2 (GameCube, Speed.elf) decompilation tooling

Everything used to build `../NFSU2.rep` and export `../decomp/`.

- `extensions/dw1-emoose/`: DWARF v1 analyzer (fork of rafalh/ghidra-dwarf1), with 20+ parser fixes for this GCC 2.95 SN Systems binary. Prebuilt zip in `dist/`.
- `extensions/gc-loader/`: GameCube loader / Gekko_Broadway CPU definition, patched for Ghidra 12.2 (Gradle 9 build, demangler field fix) and with PowerPC EABI callee-saved registers added to the cspec (`*.cspec.orig` is the original). Prebuilt zip in `dist/`.
- `scripts/`: Ghidra scripts. The pipeline order is in `rebuild.sh`. `ExportDecomp.java` writes one .c file per class/namespace, `types.h`, and `functions.tsv`. The rest are diagnostics from development.
- `scripts/expected_layouts.json`: struct layouts read directly from the DWARF, used by `VerifyLayouts.java`.
- `logs/final_build.log`: log of the build that produced the current project.

## Install extensions (Ghidra 12.2, needs Java 25+)
    unzip -o extensions/dw1-emoose/dist/*.zip -d ~/.config/ghidra/ghidra_12.2_PUBLIC/Extensions
    unzip -o extensions/gc-loader/dist/*.zip  -d ~/.config/ghidra/ghidra_12.2_PUBLIC/Extensions

## Rebuild from source
    cd extensions/<name> && JAVA_HOME=/usr/lib/jvm/java-27-openjdk \
      ~/Desktop/ghidra_12.2_PUBLIC/support/gradle/gradlew -PGHIDRA_INSTALL_DIR=$HOME/Desktop/ghidra_12.2_PUBLIC buildExtension

## Rebuild project + export
    ./rebuild.sh     # about 1-1.5 h; output in tooling/out/
