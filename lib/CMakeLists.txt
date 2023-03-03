# 플러그인과 해당하는 소스 파일의 목록
# ======================================================
set(LLVM_TUTOR_PLUGINS
    StaticCallCounter
    DynamicCallCounter
    FindFCmpEq
    ConvertFCmpEq
    InjectFuncCall
    InsertExistFunc
    MBAAdd
    MBASub
    RIV
    DuplicateBB
    OpcodeCounter
    MergeBB
    )

set(StaticCallCounter_SOURCES
  StaticCallCounter.cpp)
set(DynamicCallCounter_SOURCES
  DynamicCallCounter.cpp)
set(FindFCmpEq_SOURCES
  FindFCmpEq.cpp)
set(ConvertFCmpEq_SOURCES
  ConvertFCmpEq.cpp)
set(InjectFuncCall_SOURCES
  InjectFuncCall.cpp)
set(InsertExistFunc_SOURCES
  InsertExistFunc.cpp)
set(MBAAdd_SOURCES
  MBAAdd.cpp
  Ratio.cpp)
set(MBASub_SOURCES
  MBASub.cpp
  Ratio.cpp)
set(RIV_SOURCES
  RIV.cpp)
set(DuplicateBB_SOURCES
  DuplicateBB.cpp)
set(OpcodeCounter_SOURCES
  OpcodeCounter.cpp)
set(MergeBB_SOURCES
  MergeBB.cpp)

# 플러그인 라이브러리 설정
# ==============================
foreach( plugin ${LLVM_TUTOR_PLUGINS} )
    # 'plugin'과 상응하는 라이브러리 생성
    add_library(
      ${plugin}
      SHARED
      ${${plugin}_SOURCES}
      )

    # 'plugin'을 위한 include 디렉토리 설정
    target_include_directories(
      ${plugin}
      PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    )

    # Darwin에선(Linux와 달리), 공유 객체 내의 정의되지 않은 심볼은 link-edit 끝에서 허락되지 않는다.
    # 플러그인은 여기서 정의된다:
    #  - _are_ 공유 객체
    #  - LLVM 공유 객체로부터 심볼 참조, 즉 이러한 공유 객체가 메모리에 로드될 때까지 정의되지 않는 심볼
    #    (그리고 따라서 정적 링킹 동안 _undefined_)
    # 빌드는 다음과 같은 에러와 함께 실패할 것이다:
    #    "Undefined symbols for architecture x86_64"
    # 여러 LLVM 심볼들이 정의되지 않았다. 이러한 심볼들이 나중에 로드되고 런타임 때 해결되기에,
    # 이러한 오류들은 false positive이다.
    # 이 행동은 다음과 같이 '-undefined' OS X 링커를 통해 수정될 수 있다.
    target_link_libraries(
      ${plugin}
      "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>"
      )
endforeach()
