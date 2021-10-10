project(InstructionTest C CXX ASM)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(../../lib lib)

set(SOURCES
    ${CRT0}
    Main.cpp

    ADD.cpp # also ADDX, ADDS
    AND.cpp
    Bcc.cpp
    BitOps.cpp # many
    CMP.cpp
    DAA.cpp # also DAS
    DIVX.cpp
    EXT.cpp
    INCDEC.cpp
    LDCSTC.cpp # also ANDC, ORC, XORC
    LDMSTM.cpp
    MOV.cpp
    MULX.cpp
    NEG.cpp
    NOT.cpp
    OR.cpp
    ROT.cpp
    Shift.cpp # SHAL, SHAR, SHLL, SHLR
    SUB.cpp # also SUBX, SUBS
    TAS.cpp
    XOR.cpp
)

# this is nasty but we really don't want addresses getting adjusted
string(REPLACE "-mrelax" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_executable(InstructionTest ${SOURCES})

target_link_libraries(InstructionTest H8)
target_link_options(InstructionTest PRIVATE -T ${LINKER_SCRIPT} -nostartfiles)
set_target_properties(InstructionTest PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

make_bootable(InstructionTest)
