Patches for the Ghidra extensions (applied to upstream clones, then built with gradle):
- dw1-emoose: https://github.com/emoose/ghidra-dwarf1 @ 5b0666c -> dw1-emoose.patch
- gc-loader:  https://github.com/Cuyler36/Ghidra-GameCube-Loader @ 921504c -> gc-loader.patch
Restore: git clone <url> <dir> && git -C <dir> checkout <commit> && git -C <dir> apply ../patches/<dir>.patch
