cmake_minimum_required(VERSION 3.6)
project(retain-ptr)

find_package(PythonInterp 3.5)
find_package(PythonLibs 3.5)

include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

# TODO: Add custom command to also build retain.html in build dir

set(TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/tests)
set(TEST_BINARY_DIR ${PROJECT_BINARY_DIR}/tests)

set(USE_COVERAGE "--coverage")
set(WARN_MAYBE_UNINIT "-Wmaybe-unintialized")
set(WARN_EVERYTHING "-Weverything")
set(WARN_NOEXCEPT "-Wnoexcept")
set(WARN_PEDANTIC "-Wpedantic")
set(WARN_ONE_LINE "-WL")
set(WARN_ERROR "-Werror")
set(WARN_EXTRA "-Wextra")
set(WARN_ODR "-Wodr")
set(WARN_ALL "-Wall")

set(WARN_NO_POTENTIAL_EXPR "-Wno-potentially-evaluated-expression")
set(WARN_NO_INTERNAL_DECL "-Wno-unneeded-internal-declaration")
set(WARN_NO_CXX98_PEDANTIC "-Wno-c++98-compat-pedantic")
set(WARN_NO_DOUBLE_PROMO "-Wno-double-promotion")
set(WARN_NO_CXX98_COMPAT "-Wno-c++98-compat")
set(WARN_NO_WEAK_VTABLES "-Wno-weak-vtables")
set(WARN_NO_UNUSED_MEMFN "-Wno-unused-member-function")
set(WARN_NO_UNUSED_FUNC "-Wno-unused-function")
set(WARN_NO_FLOAT_EQUAL "-Wno-float-equal")
set(WARN_NO_DEPRECATED "-Wno-deprecated-declarations")
set(WARN_NO_ATTRIBUTES "-Wno-attributes")
set(WARN_NO_EXIT_TIME "-Wno-exit-time-destructors")
set(WARN_NO_MACRO_EXP "-Wno-disabled-macro-expansion")
set(WARN_NO_SHADOW "-Wno-shadow")
set(WARN_NO_PADDED "-Wno-padded")

check_cxx_compiler_flag(${USE_COVERAGE} CAN_USE_COVERAGE)

check_cxx_compiler_flag(${WARN_MAYBE_UNINIT} CAN_WARN_MAYBE_UNINIT)
check_cxx_compiler_flag(${WARN_EVERYTHING} CAN_WARN_EVERYTHING)
check_cxx_compiler_flag(${WARN_NOEXCEPT} CAN_WARN_NOEXCEPT)
check_cxx_compiler_flag(${WARN_PEDANTIC} CAN_WARN_PEDANTIC)
check_cxx_compiler_flag(${WARN_ONE_LINE} CAN_WARN_ONE_LINE)
check_cxx_compiler_flag(${WARN_ERROR} CAN_WARN_ERROR)
check_cxx_compiler_flag(${WARN_EXTRA} CAN_WARN_EXTRA)
check_cxx_compiler_flag(${WARN_ODR} CAN_WARN_ODR)
check_cxx_compiler_flag(${WARN_ALL} CAN_WARN_ALL)

check_cxx_compiler_flag(${WARN_NO_POTENTIAL_EXPR} CAN_WARN_NO_POTENTIAL_EXPR)
check_cxx_compiler_flag(${WARN_NO_CXX98_PEDANTIC} CAN_WARN_NO_CXX98_PEDANTIC)
check_cxx_compiler_flag(${WARN_NO_INTERNAL_DECL} CAN_WARN_NO_INTERNAL_DECL)
check_cxx_compiler_flag(${WARN_NO_DOUBLE_PROMO} CAN_WARN_NO_DOUBLE_PROMO)
check_cxx_compiler_flag(${WARN_NO_CXX98_COMPAT} CAN_WARN_NO_CXX98_COMPAT)
check_cxx_compiler_flag(${WARN_NO_WEAK_VTABLES} CAN_WARN_NO_WEAK_VTABLES)
check_cxx_compiler_flag(${WARN_NO_UNUSED_MEMFN} CAN_WARN_NO_UNUSED_MEMFN)
check_cxx_compiler_flag(${WARN_NO_UNUSED_FUNC} CAN_WARN_NO_UNUSED_FUNC)
check_cxx_compiler_flag(${WARN_NO_FLOAT_EQUAL} CAN_WARN_NO_FLOAT_EQUAL)
check_cxx_compiler_flag(${WARN_NO_DEPRECATED} CAN_WARN_NO_DEPRECATED)
check_cxx_compiler_flag(${WARN_NO_ATTRIBUTES} CAN_WARN_NO_ATTRIBUTES)
check_cxx_compiler_flag(${WARN_NO_EXIT_TIME} CAN_WARN_NO_EXIT_TIME)
check_cxx_compiler_flag(${WARN_NO_MACRO_EXP} CAN_WARN_NO_MACRO_EXP)
check_cxx_compiler_flag(${WARN_NO_SHADOW} CAN_WARN_NO_SHADOW)
check_cxx_compiler_flag(${WARN_NO_PADDED} CAN_WARN_NO_PADDED)

add_library(retain-ptr INTERFACE)
target_include_directories(retain-ptr INTERFACE
  ${PROJECT_SOURCE_DIR}/include)

if (PYTHONLIBS_FOUND)
  add_library(pywrap INTERFACE)
  target_include_directories(pywrap INTERFACE
    ${PROJECT_SOURCE_DIR}/example)
  target_include_directories(pywrap SYSTEM INTERFACE
    ${PYTHON_INCLUDE_DIR})
  target_link_libraries(pywrap INTERFACE ${PYTHON_LIBRARIES})
endif ()


add_library(doctest-main OBJECT ${TEST_SOURCE_DIR}/doctest.cxx)
target_compile_options(doctest-main
  PUBLIC
    $<$<BOOL:${CAN_USE_COVERAGE}>:${USE_COVERAGE}>
    $<$<BOOL:${CAN_WARN_MAYBE_UNINIT}>:${WARN_MAYBE_UNINIT}>
    $<$<BOOL:${CAN_WARN_EVERYTHING}>:${WARN_EVERYTHING}>
    $<$<BOOL:${CAN_WARN_NOEXCEPT}>:${WARN_NOEXCEPT}>
    $<$<BOOL:${CAN_WARN_PEDANTIC}>:${WARN_PEDANTIC}>
    $<$<BOOL:${CAN_WARN_EXTRA}>:${WARN_EXTRA}>
    $<$<BOOL:${CAN_WARN_ODR}>:${WARN_ODR}>
    $<$<BOOL:${CAN_WARN_ALL}>:${WARN_ALL}>

    $<$<BOOL:${CAN_WARN_NO_CXX98_PEDANTIC}>:${WARN_NO_CXX98_PEDANTIC}>
    $<$<BOOL:${CAN_WARN_NO_POTENTIAL_EXPR}>:${WARN_NO_POTENTIAL_EXPR}>
    $<$<BOOL:${CAN_WARN_NO_INTERNAL_DECL}>:${WARN_NO_INTERNAL_DECL}>
    $<$<BOOL:${CAN_WARN_NO_DOUBLE_PROMO}>:${WARN_NO_DOUBLE_PROMO}>
    $<$<BOOL:${CAN_WARN_NO_CXX98_COMPAT}>:${WARN_NO_CXX98_COMPAT}>
    $<$<BOOL:${CAN_WARN_NO_WEAK_VTABLES}>:${WARN_NO_WEAK_VTABLES}>
    $<$<BOOL:${CAN_WARN_NO_UNUSED_MEMFN}>:${WARN_NO_UNUSED_MEMFN}>
    $<$<BOOL:${CAN_WARN_NO_UNUSED_FUNC}>:${WARN_NO_UNUSED_FUNC}>
    $<$<BOOL:${CAN_WARN_NO_FLOAT_EQUAL}>:${WARN_NO_FLOAT_EQUAL}>
    $<$<BOOL:${CAN_WARN_NO_DEPRECATED}>:${WARN_NO_DEPRECATED}>
    $<$<BOOL:${CAN_WARN_NO_ATTRIBUTES}>:${WARN_NO_ATTRIBUTES}>
    $<$<BOOL:${CAN_WARN_NO_EXIT_TIME}>:${WARN_NO_EXIT_TIME}>
    $<$<BOOL:${CAN_WARN_NO_MACRO_EXP}>:${WARN_NO_MACRO_EXP}>
    $<$<BOOL:${CAN_WARN_NO_SHADOW}>:${WARN_NO_SHADOW}>
    $<$<BOOL:${CAN_WARN_NO_PADDED}>:${WARN_NO_PADDED}>)


if (TARGET pywrap)
  add_executable(test-python ${TEST_SOURCE_DIR}/python.cxx
    $<TARGET_OBJECTS:doctest-main>)
  add_test(python test-python)
  target_compile_options(test-python
    PRIVATE
      $<TARGET_PROPERTY:doctest-main,INTERFACE_COMPILE_OPTIONS>)
  target_include_directories(test-python SYSTEM PRIVATE ${TEST_SOURCE_DIR})
  target_include_directories(test-python PUBLIC
    $<TARGET_PROPERTY:pywrap,INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(test-python PUBLIC retain-ptr pywrap)
  target_link_libraries(test-python PRIVATE
    $<$<BOOL:${CAN_USE_COVERAGE}>:${USE_COVERAGE}>)
endif ()
