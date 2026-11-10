# Alternative to i686-w64-mingw32.cmake for hosts where llvm-mingw's own clang can't run (needs a newer glibc):
# the host's clang + a standalone ld.lld, using llvm-mingw only as the sysroot (headers, CRT, libc++, compiler-rt).
#   cmake -B build-win32 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/i686-host-clang.cmake \
#         -DLLVM_MINGW=<llvm-mingw dir> -DHOST_LLVM=/usr/lib/llvm-22 -DLLD=<ld.lld> -DRESOURCE_DIR=<dir>
# RESOURCE_DIR = host clang's include/ + llvm-mingw's lib/clang/<v>/lib/windows (compiler-rt for i386).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(LLVM_MINGW /opt/llvm-mingw CACHE PATH "llvm-mingw install (sysroot only)")
set(HOST_LLVM /usr/lib/llvm-22 CACHE PATH "host LLVM with clang")
set(LLD ${HOST_LLVM}/bin/ld.lld CACHE FILEPATH "ld.lld")
set(RESOURCE_DIR "" CACHE PATH "clang resource dir with lib/windows from llvm-mingw")
# pentium4 = llvm-mingw's i686 default: floats live in SSE registers, so casts to float really round. A plain i686
# host clang keeps them on the x87 stack and skips roundings (referee mismatches in slerp / list box scrolling).
set(_flags "--target=i686-w64-mingw32 -march=pentium4 --sysroot=${LLVM_MINGW} -rtlib=compiler-rt -unwindlib=libunwind -stdlib=libc++ --ld-path=${LLD}")
if(RESOURCE_DIR)
  string(APPEND _flags " -resource-dir=${RESOURCE_DIR}")
endif()
set(CMAKE_C_COMPILER ${HOST_LLVM}/bin/clang)
set(CMAKE_CXX_COMPILER ${HOST_LLVM}/bin/clang++)
set(CMAKE_C_FLAGS_INIT "${_flags} -isystem ${LLVM_MINGW}/i686-w64-mingw32/include")
set(CMAKE_CXX_FLAGS_INIT "${_flags} -nostdinc++ -isystem ${LLVM_MINGW}/i686-w64-mingw32/include/c++/v1 -isystem ${LLVM_MINGW}/i686-w64-mingw32/include")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${LLVM_MINGW}/i686-w64-mingw32/lib")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${LLVM_MINGW}/i686-w64-mingw32/lib")
set(CMAKE_AR ${HOST_LLVM}/bin/llvm-ar)
set(CMAKE_RANLIB ${HOST_LLVM}/bin/llvm-ranlib)
set(CMAKE_FIND_ROOT_PATH ${LLVM_MINGW}/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
