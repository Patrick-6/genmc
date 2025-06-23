cmake_minimum_required(VERSION 3.12...4.0)
project(genmc VERSION 0.11.0 LANGUAGES CXX C)

enable_testing()
include(CTest)

### Package information
include(GNUInstallDirs)
set(PKG_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/runtime")
add_compile_definitions(PACKAGE_NAME="GenMC")
add_compile_definitions(PACKAGE_BUGREPORT="michalis.kokologiannakis@inf.ethz.ch")
add_compile_definitions(PACKAGE_URL="https://plv.mpi-sws.org/genmc")
add_compile_definitions("PACKAGE_VERSION=\"${CMAKE_PROJECT_VERSION}\"")
add_compile_definitions(INCLUDE_DIR="${CMAKE_INSTALL_PREFIX}/${PKG_INCLUDE_DIR}")
add_compile_definitions(SRC_INCLUDE_DIR="${CMAKE_SOURCE_DIR}/include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
install(DIRECTORY include/ DESTINATION "${PKG_INCLUDE_DIR}" FILES_MATCHING PATTERN "*.h")

### Toolchains required
find_library(HAVE_LIBDL dl REQUIRED)
find_library(HAVE_LIBFFI ffi REQUIRED)

# For linking to LLVM, see: https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
# We unset the cached variables here, to look for different LLVM installations if `CMAKE_PREFIX_PATH` changes
unset(LLVM_DIR CACHE)
unset(LLVM_FOUND CACHE)
find_package(LLVM REQUIRED CONFIG)

find_program(CLANGPATH clang PATHS ${LLVM_TOOLS_BINARY_DIR} NO_CACHE NO_DEFAULT_PATH REQUIRED)
find_program(LLVM_CONFIG_PATH llvm-config PATHS ${LLVM_TOOLS_BINARY_DIR} NO_CACHE NO_DEFAULT_PATH REQUIRED)
add_compile_definitions(CLANGPATH="${CLANGPATH}")
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} (${LLVM_CONFIG_PATH})")
message(STATUS "Using clang ${CLANGPATH}")

separate_arguments(LLVM_DEFINITIONS)
execute_process(COMMAND "${LLVM_CONFIG_PATH}" --build-mode OUTPUT_VARIABLE LLVM_BUILDMODE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

add_compile_definitions(LLVM_BUILDMODE="${LLVM_BUILDMODE}")
add_compile_definitions(LLVM_VERSION="${LLVM_PACKAGE_VERSION}")

execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT
  ERROR_VARIABLE GIT_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE GIT_RESULT
)
if(NOT GIT_RESULT EQUAL 0)
  message(STATUS "Unable to determine git commit hash: ${GIT_ERROR}")
  set(GIT_COMMIT "uknown")
endif()
add_compile_definitions(GIT_COMMIT="${GIT_COMMIT}")

### Coverage/linting
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(-g --coverage)
  add_link_options(--coverage)
endif()

option(ENABLE_LINT "Enable clang-tidy reporting" OFF)
if(ENABLE_LINT)
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy
    "-header-filter='^(?!llvm/).+'"
    "-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
  )
endif()

add_subdirectory(src)
add_subdirectory(tests)
OPTION(BUILD_DOC "Enable documentation building" ON)
if(BUILD_DOC)
  add_subdirectory(${CMAKE_SOURCE_DIR}/doc)
endif()
