cmake_minimum_required(VERSION 3.13.4)
project(llvm-tutor-hello-world)

#===============================================================================
# 1. LLVM 설정 로드
#===============================================================================
# 이것을 유효한 LLVM 설치 경로로 설정
set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")

# LLVMConfig.cmake의 위치를 추가하여 CMake가 경로를 탐색할 수 있게 함
# (find_package가 찾을 수 있게)
list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")

# FIXME: This is a warkaround for #25. Remove once resolved and use
find_package(LLVM 15 REQUIRED CONFIG)

# HelloWorld는 LLVM의 헤더를 포함한다 - 따라서 include 경로를 업데이트한다
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

#===============================================================================
# 2. LLVM-TUTOR 빌드 설정
#===============================================================================
# LLVM이 쓰는 것과 같은 C++ standard 사용
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# LLVM은 보통 RTTI 없이 빌드된다. 그것과 일치되게 진행한다.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

#===============================================================================
# 3. 목표 추가
#===============================================================================
add_library(HelloWorld SHARED HelloWorld.cpp)

# Darwin에서 공유 객체 속 정의되지 않은 symbol을 허용(이것은 Linux에서 기본 행동이다)
target_link_libraries(HelloWorld
  "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
