set (CMAKE_SYSTEM_NAME Generic)
set (CMAKE_SYSTEM_PROCESSOR powerpc64)

set (CPD_PS3_DEV "$ENV{PS3DEV}")

set (CMAKE_C_COMPILER "${CPD_PS3_DEV}/ppu/bin/ppu-gcc")
set (CMAKE_CXX_COMPILER "${CPD_PS3_DEV}/ppu/bin/ppu-g++")

link_directories (
  "${CPD_PS3_DEV}/ppu/lib"
)

include_directories (BEFORE
  "${CPD_PS3_DEV}/ppu/include"
  "${CPD_PS3_DEV}/ppu/include/simdmath"
)

set (CMAKE_C_FLAGS "-Wall -Wextra -mcpu=cell -mhard-float -fmodulo-sched -ffunction-sections -fdata-sections")
set (CMAKE_C_FLAGS_RELEASE "-O3")
set (CMAKE_C_FLAGS_DEBUG "-gdwarf -Og")

set (CMAKE_CXX_FLAGS "-D_GLIBCXX11_USE_C99_STDIO ${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set (CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set (CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
set (CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

set (CPD_COMPILE_PS3 TRUE)
