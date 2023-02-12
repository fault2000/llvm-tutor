cmake_minimum_required(VERSION 3.13.4)
project(llvm-tutor)

#===============================================================================
# 1. LLVM 설치 경로 검증
# 이것은 약간의 sanity check이다.
#===============================================================================
set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")

# 1.1 "include" 디렉토리 확인
set(LT_LLVM_INCLUDE_DIR "${LT_LLVM_INSTALL_DIR}/include/llvm")
if(NOT EXISTS "${LT_LLVM_INCLUDE_DIR}")
message(FATAL_ERROR
  " LT_LLVM_INSTALL_DIR (${LT_LLVM_INCLUDE_DIR}) is invalid.")
endif()

# 1.2 LLVMConfig.cmake 파일 존재 확인(위치는 OS에 따름)
set(LT_VALID_INSTALLATION FALSE)

# Ubuntu + Darwin
if(EXISTS "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/LLVMConfig.cmake")
  set(LT_VALID_INSTALLATION TRUE)
endif()

# Fedora
if(EXISTS "${LT_LLVM_INSTALL_DIR}/lib64/cmake/llvm/LLVMConfig.cmake")
  set(LT_VALID_INSTALLATION TRUE)
endif()

if(NOT ${LT_VALID_INSTALLATION})
  message(FATAL_ERROR
    "LLVM installation directory, (${LT_LLVM_INSTALL_DIR}), is invalid. Couldn't
    find LLVMConfig.cmake.")
endif()

#===============================================================================
# 2. LLVM 설정 로드
#    For more: http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
#===============================================================================
# LLVMConfig.cmake의 위치를 추가하여 CMake가 경로를 찾을 수 있도록 한다
# (따라서 find_package가 찾을 수 있게 한다)
# 주의: Fedora에서, 'dnf'와 설치된 미리 컴파일된 바이너리를 사용할 때,
# LLVMConfig.cmake는 "/usr/lib64/cmake/llvm"에 위치한다. 하지만 이 경로는
# find_package(llvm)을 사용할 때 기본으로 확인되는 다른 경로들 사이에 있다.
# 즉 여기서 추가할 필요는 없다.
list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")

find_package(LLVM 15 REQUIRED CONFIG)

# 또다른 sanity check
if(NOT "15" VERSION_EQUAL "${LLVM_VERSION_MAJOR}")
  message(FATAL_ERROR "Found LLVM ${LLVM_VERSION_MAJOR}, but need LLVM 15")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LT_LLVM_INSTALL_DIR}")

message("LLVM STATUS:
  Definitions ${LLVM_DEFINITIONS}
  Includes    ${LLVM_INCLUDE_DIRS}
  Libraries   ${LLVM_LIBRARY_DIRS}
  Targets     ${LLVM_TARGETS_TO_BUILD}"
)

# LLVM 헤더와 라이브러리 경로 설정
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

#===============================================================================
# 3. LLVM-TUTOR 빌드 설정
#===============================================================================
# LLVM이 쓰는 것과 같은 C++ standard 사용
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# 빌드 유형
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE
      STRING "Build type (default Debug):" FORCE)
endif()

# 컴파일러 플래그
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall\
  -fdiagnostics-color=always")

# LLVM은 보통 RTTI없이 빌드된다. 그것과 일치되게 진행한다.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# -fvisibility-inlines-hidden은 LLVM을 빌드할 때 설정되고 Darwin에서 경고는 
# llvm-tutor가 이 플래그 없이 빌드될 때 발생한다(따라서 그렇지 않으면 빌드가 정상적인 것이다).
# 일관성을 위해, 여기 추가한다.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fvisibility-inlines-hidden" SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
if (${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

# 빌드 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

#===============================================================================
# 4. 하위 목표 추가
# 이것을 마지막에 함으로써 모든 정의와 link/include 경로가 하위 프로젝트에서 사용가능해진다.
#===============================================================================
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)
add_subdirectory(HelloWorld)
