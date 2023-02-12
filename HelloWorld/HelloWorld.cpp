//=============================================================================
// 파일:
//    HelloWorld.cpp
//
// 설명:
//    모듈 내의 모든 함수를 방문하여, 그들의 이름과 인자 수를 stderr을 통해 출력한다.
//    엄격히 말해서, 이것은 분석 패스이다. (즉, 함수가 수정되지 않는다). 하지만,
//    단순하게 하기 위해서 여기엔 'print' 메소드가 없다(모든 분석 패스는 이를 구현해야 한다.)
//
// 사용법:
//    1. Legacy PM
//      opt -enable-new-pm=0 -load libHelloWorld.dylib -legacy-hello-world -disable-output `\`
//        <input-llvm-file>
//    2. New PM
//      opt -load-pass-plugin=libHelloWorld.dylib -passes="hello-world" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//-----------------------------------------------------------------------------
// HelloWorld 구현
//-----------------------------------------------------------------------------
// 패스의 내부 구성을 외부에 노출할 필요는 없다 - 모든 것을 익명 namespace에 둔다
namespace {

// 이 메소드는 패스가 무엇을 하는지 구현한다
void visitor(Function &F) {
    errs() << "(llvm-tutor) Hello from: "<< F.getName() << "\n";
    errs() << "(llvm-tutor)   number of arguments: " << F.arg_size() << "\n";
}

// New PM 구현
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // 주 진입점, IR 유닛을 받아 (&F)에서 패스와 해당하는 패스 매니저를 실행한다(필요할 경우 검색된다).
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    visitor(F);
    return PreservedAnalyses::all();
  }

  // isRequired가 true를 리턴하지 않으면, 이 패스는 optnone LLVM 속성이 있는 함수의 경우 스킵될 것이다.
  // clang의 -O0은 모든 함수를 optnone으로 지정한다
  static bool isRequired() { return true; }
};

// Legacy PM implementation
struct LegacyHelloWorld : public FunctionPass {
  static char ID;
  LegacyHelloWorld() : FunctionPass(ID) {}
  // 주 진입점 - 이름은 이것이 실행해야 하는 IR의 유닛을 전한다.
  bool runOnFunction(Function &F) override {
    visitor(F);
    // IR의 입력 유닛을 수정하지 않는다, 따라서 'false'이다.
    return false;
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-world") {
                    FPM.addPass(HelloWorld());
                    return true;
                  }
                  return false;
                });
          }};
}

// 이것이 패스 플러그인을 위한 주 인터페이스이다. 이것은 커맨드 라인에서 패스 파이프라인이 추가될 때
// 'opt'가 HelloWorld를 인식할 수 있도록 보장한다. 즉, '-passes=hello-world'를 통해
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getHelloWorldPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// 변수의 주소는 패스를 유일하게 인식하는데 사용된다. 실제 값은 상관없다.
char LegacyHelloWorld::ID = 0;

// 이것이 패스 플러그인을 위한 주 인터페이스이다. 이것은 커맨드 라인에서 패스 파이프라인이 추가될 때
// 'opt'가 LegacyHelloWorld를 인식할 수 있도록 보장한다. 즉, '--legacy-hello-world'를 통해
static RegisterPass<LegacyHelloWorld>
    X("legacy-hello-world", "Hello World Pass",
      true, // 이 패스는 CFG를 수정하지 않는다 => true
      false // 이 패스는 순수한 분석 패스가 아니다 => false
    );
