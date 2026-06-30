cmake_minimum_required(VERSION 3.25)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

project(
   ipeDisasm
   DESCRIPTION "PoC of a small cfg builder for msp430 binaries"
   LANGUAGES C CXX
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()


set(CMAKE_C_FLAGS "-Wall -Werror -g ${CMAKE_C_FLAGS}")

add_executable(ipe-disasm main.cpp elf_parser.cpp cfg.cpp Instruction.cpp)

add_custom_target(
   run
   DEPENDS ipe-disasm
   COMMAND ./ipe-disasm ../ipe-main.elf
)
