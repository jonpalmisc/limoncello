cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(Limoncello LANGUAGES C CXX)

find_package(LLVM 17 REQUIRED CONFIG)
include(AddLLVM)

# TODO: Create proper sub-libraries and remove globbing & filtering.
file(GLOB_RECURSE LIMONCELLO_CORE_SOURCE include/*.h lib/*.h lib/*.cpp)

add_llvm_library(LimoncelloCore ${LIMONCELLO_CORE_SOURCE})
target_compile_features(LimoncelloCore PRIVATE cxx_std_20)
target_compile_definitions(LimoncelloCore PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(LimoncelloCore PUBLIC include ${LLVM_INCLUDE_DIRS})

add_llvm_library(Limoncello MODULE src/Plugin.cpp PLUGIN_TOOL opt)
target_link_libraries(Limoncello PRIVATE LimoncelloCore)
