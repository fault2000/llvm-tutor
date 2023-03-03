# llvm-tutor

LLVM 15에 기반을 둔 LLVM pass의 예시이다.

 llvm-tutor는 언급하는 패스들을 자체적으로 포함한 모음이며 LLVM 개발자가 되려는 자와 초보자를 위한 지침서이다. 주 기능은 다음과 같다:

- Out-of-tree - 바이너리 LLVM 설치를 대상으로 빌드됨(사용자의 컴퓨터에서 LLVM을 빌드할 필요가 없음)
- Complete - CMake 빌드 스크립트, LIT 테스트, CI 구성과 기록들을 포함
- Modern - LLVM의 마지막 버전을 기반으로 함(매 release마다 업데이트됨)

# Overview

 LLVM은 매우 풍부하고 강력하며 유명한 API를 구현했다. 하지만, 많은 복잡한 기술들과 같이 배우고 마스터하기에 꽤나 벅차고 압도적일 수 있다. 이 LLVM 튜토리얼의 목표는 LLVM이 사실 다루기 쉽고 재미있을 수 있다는 것을 보여주는 것이다. 이는 자연스럽게 쓰인 LLVM을 사용해 구현된 범위를 스스로 포함하고, 테스트 가능한 LLVM 패스들을 통해 증명될 것이다.

 이 문서는 환경을 어떻게 설정하는지, 예시들을 빌드하고 실행하는지, 디버깅을 하는 것에 대해 설명한다. 또한 구현된 예시들에 대한 높은 수준의 개요와 LLVM 패스를 작성하는 것에 대한 예비 지식 정보를 포함한다. 소스 파일에는 코드 자체를 제외하고 구현 과정을 안내하는 주석이 포함되어 있다. 모든 예시들은 LIT 테스트와 레퍼런스 input 파일들을 통해 보완된다.

 혹시 Clang을 위한 비슷한 튜토리얼에 관심이 있다면 [clang-tutor](https://github.com/banach-space/clang-tutor/)을 방문해보라.

# HelloWorld: 당신의 첫 패스

 HelloWorld.cpp의 HelloWorld 패스는 참조를 포함하는 예시이다. 이에 해당하는 CMakeLists.txt는 out-of-source 패스를 위한 최소한의 설정을 구현한다.

 입력된 모듈 속의 정의된 모든 함수에 대해, HelloWorld는 함수의 이름과 그것이 받는 모든 인자의 수를 출력한다. 당신은 다음과 같이 빌드할 수 있다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
mkdir build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR <source/dir/llvm/tutor>/HelloWorld/
make
```

/* 역자 추가

<installation/dir/of/llvm/15>이 가리키는 것은 LLVM 설치 위치이다. 만약 설치를 아직 하지 않았다면 직접 설치하고 경로를 지정해주든, <추가예정>으로 가서 설치하고 오자. 참고로 apt 설치는 믿지 마라. 오류 걸린다.

*/

 테스트 전에, 입력 파일을 준비해야 한다.

```bash
# Generate an LLVM test file
$LLVM_DIR/bin/clang -O1 -S -emit-llvm <source/dir/llvm/tutor>/inputs/input_for_hello.c -o input_for_hello.ll
```

 마지막으로, HelloWorld를 opt와 함께 실행한다(Linux에선 libHelloWorld.so, Mac에선 libHelloWorld.dylib)

```bash
# Run the pass
$LLVM_DIR/bin/opt -load-pass-plugin ./libHelloWorld.{so|dylib} -passes=hello-world -disable-output input_for_hello.ll
# Expected output
(llvm-tutor) Hello from: foo
(llvm-tutor)   number of arguments: 1
(llvm-tutor) Hello from: bar
(llvm-tutor)   number of arguments: 2
(llvm-tutor) Hello from: fez
(llvm-tutor)   number of arguments: 3
(llvm-tutor) Hello from: main
(llvm-tutor)   number of arguments: 2
```

 HelloWorld 패스는 입력 모듈을 수정하지 않는다. -disable-output 플래그는 opt가 출력 bitcode 파일을 출력하는 것을 방지하는데 사용된다.

# Part 1: llvm-tutor 세부사항

## 개발 환경

  이 프로젝트는 Ubuntu 22.04와 Mac OS X 11.7에서 테스트되어져왔다. llvm-tutor를 빌드하기 위해 다음이 필요하다:

- LLVM 15
- C++17을 지원하는 C++ 컴파일러
- CMake 3.13.4 혹은 더 높은 버전

 패스를 실행하기 위해, 다음이 필요하다:

- clang-15(입력될 LLVM 파일을 생성하기 위해)
- opt(패스를 실행하기 위해)

 테스트를 위한 추가적인 준비물은 다음과 같다(LLVM을 설치함으로써 이들은 충족된다):

- lit(일명 llvm-lit, 테스트를 실행하기 위한 LLVM 도구)
- FileCheck(LIT 준비물, 이는 테스트가 예상된 출력을 생성하는지 검사하는데 사용된다)

### Mac OS X에서 LLVM 15 설치

다윈 운영체제에서는 LLVM을 homebrew를 통해 설치할 수 있다:

```bash
brew install llvm@15
```

만약 이미 오래된 버전의 LLVM을 설치했다면, 다음으로 LLVM을 업그레이드할 수 있다:

```bash
brew upgrade llvm
```

설치(혹은 업그레이드)가 완료되면, 모든 필요한 헤더 파일, 라이브러리와 도구들은 /usr/local/opt/llvm/ 내의 존재할 것이다.

### Ubuntu에서 LLVM 15 설치

Jammy Jellyfish버전 우분투에서, 공식 레포지토리에서 최신 LLVM을 설치할 수 있다.

```bash
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo apt-add-repository "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-15 main"
sudo apt-get update
sudo apt-get install -y llvm-15 llvm-15-dev llvm-15-tools clang-15
```

이는 필요한 헤더 파일과 라이브러리, 도구들을 /usr/lib/llvm-15/에 설치할 것이다.

### 소스로부터 LLVM 15 빌딩

소스로부터 빌딩하는 것은 느리고 디버그하기 까다로울 수 있다. 필수적이진 않지만, 당신이 LLVM 15를 얻는데 선호하는 방법일 수 있다. 다음 과정이 Linux와 Mac OS X에서 작동한다:

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout release/15.x
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=host -DLLVM_ENABLE_PROJECTS=clang <llvm-project/root/dir>/llvm/
cmake --build .
```

더 자세한 세부 사항은 공식 문서를 읽어보자.

## 빌딩 및 테스팅

### 빌딩

  다음과 같이 llvm-tutor(그리고 제공된 패스 플러그인 전부)를 빌드할 수 있다:

```bash
cd <build/dir>
cmake -DLT_LLVM_INSTALL_DIR=<installation/dir/of/llvm/15> <source/dir/llvm/tutor>
make
```

 LT_LLVM_INSTALL_DIR 변수는 LLVM 15의 설치 혹은 빌드 디렉토리의 root로 설정되어져야 한다. 그래야 include와 라이브러리 경로를 설정하는데 쓰이는 해당하는 LLVMConfig.cmake 스크립트의 위치를 찾는데 사용된다.

### 테스팅

 llvm-tutor을 실행하기 위해, 당신은 llvm-lit(일명 lit)을 설치해야 한다. 이것은 LLVM 15 패키지에 포함되어 있진 않지만, pip을 통해 설치할 수 있다:

```bash
# Install lit - note that this installs lit globally
pip install lit
```

테스트를 실행하는 것은 간단하다:

```bash
$ lit <build_dir>/test
```

이로써 모든 테스트가 통과되는 것을 볼 수 있다.

### 공유 객체로써의 LLVM 플러그인

 llvm-tutor에서 모든 LLVM 패스는 개별적인 공유 객체로 구현된다. 이 공유 객체는 근본적으로 opt를 위해 동적으로 불러와질 수 있는 플러그인이다. 모든 플러그인은 <build/dir>/lib 디렉토리에 빌드되어진다.

 동적으로 로드된 공유 객체의 확장자가 Linux와 Mac OS에서 다르다는 점을 염두에 두자. 예를 들어, HelloWorld 패스의 경우:

- 리눅스에선 libHelloWorld.so
- MacOS에선 libHelloWorld.dylib

 일관성을 위해서, [README.md](http://README.md) 파일 내에서 모든 예시는 *.so 확장자를 사용한다. Mac OS에서 작동시킬 경우, *.dylib을 대신 사용하라.

## 패스 개요

 사용 가능한 패스들은 분석, 변형 혹은 CFG로 카테고리화되어진다. 분석과 변형 패스의 차이점은 다소 자명하다([여기](https://www.notion.so/llvm-tutor-1f8b88f601de4bd6818b84c4360eed1a)에 좀 더 기술적 설명이 있다). CFG 패스는 단순히 Control Flow Graph를 수정하는 변형 패스이다. 지정된 카테고리 때문에, 이는 자주 좀 더 복잡하고 추가적인 기록 보관을 필요로 한다.

 다음 표에서 패스들은 주제적으로 그룹되며 복잡도 수준에 따라 정렬된다.

| 이름 | 설명 | 카테고리 |
| --- | --- | --- |
| HelloWorld | 모든 함수를 방문하고 그들의 이름 출력 | 분석 |
| OpcodeCounter | 입력된 모듈의 LLVM IR opcode 요약 출력 | 분석 |
| InjectFuncCall | printf 호출을 삽입하여 입력된 모듈 계측 | 변형 |
| StaticCallCounter | 컴파일 시간에 직접 함수 호출 개수 셈(정적 분석) | 분석 |
| DynamicCallCounter | 실행 시간 동안 직접 함수 호출 개수 셈(동적 분석) | 변형 |
| MBASub | 정수 sub 명령 모호하게 만듬(???) | 변형 |
| MBAAdd | 8-bit 정수 add 명령 모호하게 만듬 | 변형 |
| FindFCmpEq | 부동 소수점 등식 비교 찾기 | 분석 |
| ConvertFCmpEq | 직접 부동 소수점 등식 비교를 다른 비교로 변환 | 변형 |
| RIV | 각 basic block이 접근가능한 integer 값 검색 | 분석 |
| DuplicateBB | basic block 복제, RIV 분석 결과 필요 | CFG |
| MergeBB | 복제된 basic block 병합 | CFG |

 처음 프로젝트를 빌드하고 나면, 모든 패스를 제각각 실험해볼 수 있다. HelloWorld를 제외한 모든 패스는 아래에서 더 자세히 기술된다.

 LLVM 패스들은 LLVM IR 파일들과 같이 작동한다. 다음과 같은 방식으로 파일을 만들 수 있다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Textual form
$LLVM_DIR/bin/clang -O1 -emit-llvm input.c -S -o out.ll
# Binary/bit-code form
$LLVM_DIR/bin/clang -O1 -emit-llvm input.c -c -o out.bc
```

 바이너리, **.bc(기본) 혹은 Textual/LLVM 어셈블리 양식(*.ll, -S 플래그 필요)을 선택하든 상관없다. 확실히는, 후자가 더 인간이 읽기 쉽다. 비슷한 로직이 opt에 적용되어 있으며 기본적으로 *.bc 파일을 생성한다. -S 플래그를 사용하여 결과를 *.ll 파일로 써서 얻을 수 있다.

 주목할 점은 clang이 다음과 같을 때 optnone 함수 속성을 추가한다.

- 최적화 레벨이 특정되지 않았거나
- -O0 가 특정되었거나.

 만약 -O0으로 컴파일하고 싶다면, -O0 -Xclang -disable-O0-optnone을 특정하거나 정적 isRequired 메소드를 당신의 패스에서 정의해줘야한다. 대체적으로, -O1 혹은 더 높은 플래그를 특정할 수 있다. 그렇지 않으면 새 패스 매니저가 패스를 등록하지만 패스는 실행되지 않을 것이다.

 앞에서 적었듯, 이 파일 내의 모든 예시는 패스 플러그인을 위해 *.so 확장을 사용한다. Mac OS에서 작동할 때, *.dylib을 대신 사용해야한다.

### OpcodeCounter

 OpcodeCounter는 입력 모듈의 모든 함수 내에서 마주치는 LLVM IR opcode의 요약을 출력하는 분석 패스이다. 이 패스는 미리 정의된 최적화 파이프라인 중 하나와 같이 자동적으로 실행될 수 있다. 하지만, 먼저 우리가 시도하고 테스트한 방법을 사용해 보자.

 우리는 input_for_cc.c를 OpcodeCounter의 테스트를 위해 사용할 것이다. OpcodeCounter가 분석 패스이기 때문에, 우리는 opt가 결과를 출력하기를 원한다. 이를 위해, 우리는 OpcodeCounter에 해당하는 출력 패스를 사용한다. 이 패스는 print<opcode-counter>로 불린다. 추가적인 인자를 필요하지 않지만, -disable-output을 추가하여 opt가 결과 LLVM IR 모듈을 출력하는 것을 방지하는 좋은 아이디어다. 우리는 모듈 그 자체보다 분석 결과만 관심을 두기 때문이다. 사실, 이 패스가 입력 IR을 수정하지 않기 때문에, 출력 모듈은 아무튼 입력과 등일할 것이기 때문이다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -emit-llvm -c <source_dir>/inputs/input_for_cc.c -o input_for_cc.bc
# Run the pass through opt
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libOpcodeCounter.so --passes="print<opcode-counter>" -disable-output input_for_cc.bc
```

 main의 경우, OpcodeCounter는 다음과 같은 요약을 출력한다(패스를 실행하는 동안, input_for_cc.bc 내의 정의된 다른 함수에 대한 요약 또한 출력된다.):

```bash
=================================================
LLVM-TUTOR: OpcodeCounter results for `main`
=================================================
OPCODE               #N TIMES USED
-------------------------------------------------
load                 2
br                   4
icmp                 1
add                  1
ret                  1
alloca               2
store                4
call                 4
-------------------------------------------------
```

### 최적화 파이프라인에 자동 등록

 간단히 최적화 레벨을 특정(예를 들어, -O{1|2|3|s})함으로써 OpcodeCounter를 실행할 수 있다. 이는 현존하는 최적화 패스 파이프라인에 자동 등록을 통해 성취될 수 있다. 주의할 점은 여전히 불러와질 플러그인 파일을 특정해야한다는 것이다.

```bash
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libOpcodeCounter.so --passes='default<O1>' input_for_cc.bc
```

 예시에서는 new pass Manager(플러그인 파일이 -load 대신 -load-pass-plugin으로 특정됨)를 사용했다. 자동 등록은 legacy pass manager와도 동작한다:

```bash
$LLVM_DIR/bin/opt -load <build_dir>/lib/libOpcodeCounter.so -O1 input_for_cc.bc
```

 이는 OpcodeCounter.cpp의 신규 PM의 경우 122번 라인, 레거시 PM의 경우 159번 라인에서 구현됩니다. 이 섹션은 LLVM내의 패스 매니저에 대한 더 많은 정보를 담고 있다.

### InjectFuncCall

 이 패스는 코드 계측에 대한 HelloWorld 예시이다. 입력 모듈 내에 정의된 모든 함수에 대해, InjectFuncCall은 다음과 같은 printf 호출을 추가(주입)한다:

```c
printf("(llvm-tutor) Hello from: %s\n(llvm-tutor)   number of arguments: %d\n", FuncName, FuncNumArgs)
```

 이 호출은 각 함수의 시작마다 추가된다(즉, 모든 다른 명령 전). FuncName은 함수의 이름이고 FuncNumArgs는 함수가 받는 인자의 수이다.

 input_for_hello.c를 InjectFuncCall을 시험하기 위해 사용했다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -O0 -emit-llvm -c <source_dir>/inputs/input_for_hello.c -o input_for_hello.bc
# Run the pass through opt - New PM
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libInjectFuncCall.so --passes="inject-func-call" input_for_hello.bc -o instrumented.bin
# Run the pass through opt - Legacy PM
$LLVM_DIR/bin/opt -enable-new-pm=0 -load <build_dir>/lib/libInjectFuncCall.so -legacy-inject-func-call input_for_hello.bc -o instrumented.bin
```

 이것은 input_for_hello.bc의 계측된 버전인 instrumented.bin을 생성한다. InjectFuncCall이 예상된 대로 실행되었는지 검증하기 위해, 당신은 출력 파일을 검사(그리고 printf에 대한 추가 호출을 포함하는지 검증) 혹은 실행해볼 수 있다:

```bash
$LLVM_DIR/bin/lli instrumented.bin
(llvm-tutor) Hello from: main
(llvm-tutor)   number of arguments: 2
(llvm-tutor) Hello from: foo
(llvm-tutor)   number of arguments: 1
(llvm-tutor) Hello from: bar
(llvm-tutor)   number of arguments: 2
(llvm-tutor) Hello from: foo
(llvm-tutor)   number of arguments: 1
(llvm-tutor) Hello from: fez
(llvm-tutor)   number of arguments: 3
(llvm-tutor) Hello from: bar
(llvm-tutor)   number of arguments: 2
(llvm-tutor) Hello from: foo
(llvm-tutor)   number of arguments: 1
```

### InjectFuncCall vs HelloWorld

 당신은 아마 InjectFuncCall이 HelloWorld와 약간 비슷하다는 것을 눈치챘을 것이다. 두 케이스 모두 패스가 모든 함수를 방문하고, 그들의 이름과 인자 수를 출력한다. 두 패스 간에 차이는 같은 입력 파일, 예를 들어 input_for_hello.c에 대해 생성된 출력을 비교할 때 꽤나 명확해진다. Hello from이 출력된 횟수는 다음과 같다:

- InjectFuncCall의 경우 매 함수 호출 시 하나, 혹은
- HelloWorld의 경우 함수 정의 시 하나

 이는 완벽히 이치에 맞으며 두 패스가 어떻게 다른지에 대한 단서를 준다. Hello 출력 여부는 다음 중 하나에서 결정된다:

- InjectFuncCall의 경우 실행 시간, 혹은
- HelloWorld의 경우 컴파일 시간

 또한, 주의할 점은 InjectFuncCall의 경우 opt와 함께 패스를 먼저 실행해야 하고 그 다음 계측된 IR 모듈을 실행하여 출력을 볼 수 있다. HelloWorld의 경우 opt와 함께 패스를 실행하기만 해도 충분하다.

### StaticCallCounter

 StaticCallCounter 패스는 입력 LLVM 모듈 내의 정적 함수 호출 수를 센다. 정적은 이러한 함수 호출이 컴파일 시간 호출(즉, 컴파일 중에 볼 수 있음)이라는 사실을 의미한다. 이는 동적 함수 호출과 대조적이다. 예를 들어, 실행 시간에 마주치는 함수 호출(컴파일된 모듈이 실행중일때). 이러한 차이는 루프 내의 함수 호출을 분석할 때 명확해진다:

```bash
for (i = 0; i < 10; i++)
	foo();
```

 실행 시간동안 foo 함수가 10번 실행됨에도 불구하고, StaticCallCounter는 오직 한 번의 함수 호출을 보고한다.

 이 패스는 직접 함수 호출만을 고려한다. 함수 포인터를 통한 함수 호출은 고려 대상이 아니다.

 우리는 StaticCallCounter를 테스트하기 위해 input_for_cc.c를 사용한다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -emit-llvm -c <source_dir>/inputs/input_for_cc.c -o input_for_cc.bc
# Run the pass through opt - Legacy PM
$LLVM_DIR/bin/opt -enable-new-pm=0 -load <build_dir>/lib/libStaticCallCounter.so -legacy-static-cc -analyze input_for_cc.bc
```

 다음과 같은 출력을 볼 수 있을 것이다.

```bash
=================================================
LLVM-TUTOR: static analysis results
=================================================
NAME                 #N DIRECT CALLS
-------------------------------------------------
foo                  3
bar                  2
fez                  1
-------------------------------------------------
```

 -analyze라는 위에 있는 추가적인 커맨드 라인 옵션을 살펴보자. 이것은 opt에게 stdout으로 분석 결과를 출력하라고 알려주는데 필요하다. 이 옵션에 대한 자세한 설명은 **여기<추가예정>**에서 논의된다.

당신은 static으로 불리는 스탠드얼론 도구를 통해 StaticCallCounter를 실행할 수 있다. static은 StaticMain.cpp로 구현된 LLVM 기반 도구이다. opt를 사용하지 않고도 StaticCallCounter를 실행할 수 있는 명령줄 래퍼이다:

```bash
<build_dir>/bin/static input_for_cc.bc
```

 이것이 상대적으로 기본적인 정적 분석 도구에 대한 예시이다. 이것의 구현은 LLVM 내의 기본적인 패스 관리가 작동하는 방법을 설명한다(예를 들어, opt에 의존하는 대신 도구 스스로 처리한다.

### DynamicCallCounter

 DynamicCallCounter 패스는 실행 시간(실행 중 마주치는) 함수 호출의 수를 센다. 이는 함수가 호출되는 매 시간마다 실행되는 호출을 세는 명령을 삽입함으로써 이루어진다. 입력 모듈에서 정의된 함수에 대한 호출만 세어진다. 이 패스는 InjectFuncCall에서 소개된 아이디어를 바탕으로 빌드되었다. 당신은 아마 먼저 예시를 경험하는 것을 원할지도 모른다.

 우리는 input_for_cc.c로 DynamicCallCounter를 테스트한다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -emit-llvm -c <source_dir>/inputs/input_for_cc.c -o input_for_cc.bc
# Instrument the input file
$LLVM_DIR/bin/opt -load-pass-plugin=<build_dir>/lib/libDynamicCallCounter.so -passes="dynamic-cc" input_for_cc.bc -o instrumented_bin
```

 이는 input_for_cc.c의 계측된 버전인 instrumented.bin을 생성한다. DynamicCallCounter가 예상한대로 작동했는지 검증하기 위해, 출력 파일을 검사하거나(새로운 호출을 세는 명령이 포함되었는지 확인) 실행해볼 수 있다:

```bash
# Run the instrumented binary
$LLVM_DIR/bin/lli  ./instrumented_bin
```

 다음과 같은 결과를 볼 수 있을 것이다.

```bash
=================================================
LLVM-TUTOR: dynamic analysis results
=================================================
NAME                 #N DIRECT CALLS
-------------------------------------------------
foo                  13
bar                  2
fez                  1
main                 1
```

### DynamicCallCounter vs StaticCallCounter

 DynamicCallCounter와 StaticCallCounter가 보고하는 함수 호출의 수는 서로 다르다. 하지만 두 결과 모두 정확하다. 이 결과들은 각각 실행 시간과 컴파일 시간 함수 호출과 상응하다. 또한 StaticCallCounter의 경우 opt를 통해 패스를 실행하는 것만으로도 요약을 출력하는데 충분하다. DynamicCallCounter의 경우 계측된 바이너리를 실행해야만 결과를 볼 수 있다. 이는 HelloWorld와 InjectFuncCall을 비교할 때 우리가 관찰했던 것과 비슷하다.

### Mixed Boolean Arithmetic Transformations

 이 패스들은 [mixed boolean arithmetic](https://tel.archives-ouvertes.fr/tel-01623849/document) 변형을 구현했다. 비슷한 변형이 코드 모호화에 자주 사용되며(당신은 [Hacker's Delight](https://www.amazon.co.uk/Hackers-Delight-Henry-S-Warren/dp/0201914654)에서 그들을 알고 있을 수 있다) LLVM 패스가 무엇에 사용될 수 있고 어떻게 사용될 수 있는지를 보여주는 좋은 예이다.

 비슷한 변형이 소스 코드 레벨에서 가능하다. 관련된 Clang 플러그인이 clang-tutor에서 사용가능하다.

### MBASub

 MBASub 패스는 다음과 같은 다소 기본적인 표현식을 구현한다.

```bash
a - b == (a + ~b) + 1
```

 기본적으로, 위의 공식에 따라 정수 뺄셈의 모든 경우를 대체한다. 해당하는 LIT 검사들은 공식과 구현이 둘 다 정확한지 확인한다.

 input_for_mba_sub.c를 사용해 MBASub를 검사한다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S <source_dir>/inputs/input_for_mba_sub.c -o input_for_sub.ll
$LLVM_DIR/bin/opt -load-pass-plugin=<build_dir>/lib/libMBASub.so -passes="mba-add" -S input_for_sub.ll -o out.ll
```

### MBAAdd

 MBAAdd 패스는 8 bit 정수의 경우만 유효한 약간 더 진화된 공식을 구현한다.

```bash
a + b == (((a ^ b) + 2 * (a & b)) * 39 + 23) * 151 + 111
```

 MBASub와 비슷하게, 위의 공식에 따라 모든 정수 add의 경우를 대체하지만, 오직 8-bit 정수만이다. LIT 테스트는 공식과 구현이 정확한지 확인한다.

 input_for_add.c를 사용해 MBAAdd를 검사한다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -O1 -emit-llvm -S <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
$LLVM_DIR/bin/opt -load-pass-plugin=<build_dir>/lib/libMBAAdd.so -passes="mba-add" -S input_for_mba.ll -o out.ll
```

 또한 모호화의 수준을 0.0부터 1.0까지 설정할 수 있다. 0.0은 모호화가 없다는 것에 해당하고, 1.0은 모든 add 명령이 위 공식으로 대체됨을 의미한다. 하지만, 이러한 추가적인 기능이 작동하기 위해선 기존 패스 매니저를 사용해야 한다.

```bash
$LLVM_DIR/bin/opt -load <build_dir>/lib/libMBAAdd.so -legacy-mba-add -mba-ratio=0.3 <source_dir>/inputs/input_for_mba.c -o out.ll
```

### RIV

 RIV는 입력 함수 내의 각각의 basic block BB에 대해 정수 값에 닿을 수 있는(예를 들어, BB 내에서 보이는(사용할 수 있는)) 집합을 산출하는 분석 패스이다. 패스가 입력 파일의 LLVM IR 표현에 작동하므로 LLVM IR 의미에서 정수 타입을 가지는 모든 값을 고려한다. 자세히는, LLVM IR 수준에서 boolean은 1bit 넓이의 정수(즉, i1)로 표현되므로 boolean도 결과에 포함된다는 것을 알 수 있다.

 이 패스는 LLVM내의 다른 분석 패스로부터 결과를 요청하는 방법에 대해 설명한다. 특히, 입력 함수의 basic block을 위한 dominance tree를 얻고자 사용되는 LLVM의 [Dominator Tree](https://en.wikipedia.org/wiki/Dominator_(graph_theory)) 분석 패스에 의존한다.

 input_for_riv.c를 사용해 RIV를 검사한다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_riv.c -o input_for_riv.ll
# Run the pass through opt - Legacy PM
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libRIV.so -passes="print<riv>" -disable-output input_for_riv.ll
```

 그러면 다음과 같은 결과를 볼 수 있을 것이다.

```bash
=================================================
LLVM-TUTOR: RIV analysis results
=================================================
BB id      Reachable Ineger Values
-------------------------------------------------
BB %entry
             i32 %a
             i32 %b
             i32 %c
BB %if.then
               %add = add nsw i32 %a, 123
               %cmp = icmp sgt i32 %a, 0
             i32 %a
             i32 %b
             i32 %c
BB %if.end8
               %add = add nsw i32 %a, 123
               %cmp = icmp sgt i32 %a, 0
             i32 %a
             i32 %b
             i32 %c
BB %if.then2
               %mul = mul nsw i32 %b, %a
               %div = sdiv i32 %b, %c
               %cmp1 = icmp eq i32 %mul, %div
               %add = add nsw i32 %a, 123
               %cmp = icmp sgt i32 %a, 0
             i32 %a
             i32 %b
             i32 %c
BB %if.else
               %mul = mul nsw i32 %b, %a
               %div = sdiv i32 %b, %c
               %cmp1 = icmp eq i32 %mul, %div
               %add = add nsw i32 %a, 123
               %cmp = icmp sgt i32 %a, 0
             i32 %a
             i32 %b
             i32 %c
```

 -analyze라는 위에 있는 추가적인 커맨드 라인 옵션을 살펴보자. 이것은 opt에게 stdout으로 분석 결과를 출력하라고 알려주는데 필요하다. 이 옵션에 대한 자세한 설명은 **여기<추가예정>**에서 논의된다.

### DuplicateBB

 이 패스는 모듈 내의 접근 가능한 정수 값(RIV 패스를 통해 밝혀진) basic block의 예외를 포함한 모든 basic block을 복사한다. 이러한 basic block의 예외는 접근 블럭에서 다음과 같은 함수이다:

- 인자를 받지 않고
- 전역 값을 정의하지 않는 모듈 내의 새겨진

 Basic block은 if-then-else 문을 먼저 삽입하고 기존 basic block의 모든 명령(PHI node의 예외와 함께)을 두 새로운 basic block(기존 basic block의 복사본)으로 복사하여 중복된다. if-then-else 문은 어느 복제된 basic block으로 분기될지 결정하는 사소하지 않은 메커니즘으로 소개된다. 이 조건은 다음과 같다:

```bash
if (var == 0)
  goto clone 1
else
  goto clone 2
```

- var는 RIV가 현재 basic block을 위해 설정한 무작위로 선정된 변수이다.
- clone 1과 clone 2는 복제된 basic block에 대한 라벨이다.

 완전한 변형은 이것처럼 보인다:

```bash
BEFORE:                     AFTER:
-------                     ------
                              [ if-then-else ]
             DuplicateBB           /  \
[ BB ]      ------------>   [clone 1] [clone 2]
                                   \  /
                                 [ tail ]

LEGEND:
-------
[BB]           - 기존 basic block
[if-then-else] - if-then-else 문을 포함한 새로운 basic block (DuplicateBB에 의해 삽입된)
[clone 1|2]    - BB의 복사본인 두 새로운 basic block (DuplicateBB에 의해 삽입된)
[tail]         - [clone 1]과 [clone 2]를 병합하는 새로운 basic block (DuplicateBB에 의해 삽입된)
```

 위에서 묘사된 것처럼, DuplicateBB는 자격을 갖춘 basic block을 4개의 새로운 basic block으로 교체한다. 이는 LLVM의 SplitBlockAndInsertIfThenElse를 통해 구현된다. DuplicateBB는 모든 필수적인 준비와 정리를 한다. 다시 말해, 이는 LLVM의 SplitBlockAndInsertIfThenElse의 정교한 래퍼이다.

 이 패스는 RIV 패스에 의해 달라지며, DuplicateBB가 작동하기 위해서 필요하기도 하다. input_for_duplicate_bb.c를 우리의 표본 입력으로 사용하자. 먼저, LLVM 파일을 생성한다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_duplicate_bb.c -o input_for_duplicate_bb.ll
```

 input_for_duplicate_bb.ll 내의 함수 foo는 다음처럼 생겨야 한다.(모든 메타데이터는 제거되었다):

```llvm
define i32 @foo(i32) {
  ret i32 1
}
```

 주의할 점은 오직 하나의 basic block(진입 블록)은 하나뿐이고 foo는 하나의 인자만을 받는다(이는 RIV의 결과가 비어있지 않은 집합이 됨을 뜻한다). 이제 DuplicateBB를 foo에게 적용한다:

```bash
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libRIV.so -load-pass-plugin <build_dir>/lib/libDuplicateBB.so -passes=duplicate-bb -S input_for_duplicate_bb.ll -o duplicate.ll
```

 계측 후 foo는 다음과 같을 것이다(모든 메타데이터는 제거되었다).

```llvm
define i32 @foo(i32) {
lt-if-then-else-0:
  %2 = icmp eq i32 %0, 0
  br i1 %2, label %lt-if-then-0, label %lt-else-0

clone-1-0:
  br label %lt-tail-0

clone-2-0:
  br label %lt-tail-0

lt-tail-0:
  ret i32 1
}
```

 여기엔 하나 대신 4개의 basic block이 있다. 모든 새로운 basic block은 기존 basic block의 수적 id를 끝에 포함한다(이 경우 0). lt-if-then-else-0은 새로운 if-then-else 조건을 포함한다. clone-1-0과 clone-2-0은 foo 내의 기존 basic block의 복사본이다. lt-tail-0은 clone-1-0과 clone-2-0을 병합하기 위해 필요한 추가적인 basic block이다.

### MergeBB

 MergeBB는 동일한 자격을 갖춘 basic block을 병합할 것이다. 어느 정도는, 이 패스는 DuplicateBB에 의해 소개된 변형을 되돌린다. 이는 아래처럼 보여진다:

```bash
BEFORE:                     AFTER DuplicateBB:                 AFTER MergeBB:
-------                     ------------------                 --------------
                              [ if-then-else ]                 [ if-then-else* ]
             DuplicateBB           /  \               MergeBB         |
[ BB ]      ------------>   [clone 1] [clone 2]      -------->    [ clone ]
                                   \  /                               |
                                 [ tail ]                         [ tail* ]

LEGEND:
-------
[BB]           - 기존 basic block
[if-then-else] - if-then-else 문을 포함하는 새로운 basic block (**DuplicateBB**)
[clone 1|2}    - BB의 복사본인 두 새로운 basic block (**DuplicateBB**)
[tail]         - [clone 1]과 [clone 2]를 병합하는 새로운 basic block (**DuplicateBB**)
[clone]        - 병합 후의 [clone 1]과 [clone 2]. 이 블록은 [BB]와 매우 흡사해야 함 (**MergeBB**)
[label*]       - **MergeBB**에 의해 업데이트된 후의 [label]
```

 다시 말해 DuplicateBB는 모든 자격이 있는 basic block을 4개의 새로운 basic block으로 대체하며, 이들 중 2개는 기존 block의 복사본이다. MergeBB는 이 두 복사본을 다시 하나로 병합하지만, DuplicateBB에 의해 추가된 남아있는 두 block을 제거하지는 않는다(그러나 그들을 업데이트할 것이다).

 다음과 같은 foo의 IR 구현을 입력으로 사용해보자. basic block 3와 5는 동일하고 안전하게 병합될 수 있다.

```llvm
define i32 @foo(i32) {
  %2 = icmp eq i32 %0, 19
  br i1 %2, label %3, label %5

; <label>:3:
  %4 = add i32 %0,  13
  br label %7

; <label>:5:
  %6 = add i32 %0,  13
  br label %7

; <label>:7:
  %8 = phi i32 [ %4, %3 ], [ %6, %5 ]
  ret i32 %8
}
```

 우리는 이제 foo에 MergeBB를 적용할 것이다:

```bash
$LLVM_DIR/bin/opt -load <build_dir>/lib/libMergeBB.so -legacy-merge-bb -S foo.ll -o merge.ll
```

 계측 이후 foo는 다음과 같을 것이다(모든 메타데이터는 제거되었다):

```llvm
define i32 @foo(i32) {
  %2 = icmp eq i32 %0, 19
  br i1 %2, label %3, label %3

3:
  %4 = add i32 %0, 13
  br label %5

5:
  ret i32 %4
}
```

 보다시피, 입력 모듈의 basic block 3과 5는 하나의 basic block으로 병합되었다.

### DuplicateBB의 출력에서 MergeBB 실행

 DuplicateBB로부터의 출력에서 MergeBB의 효과를 보는 것은 매우 흥미롭다. 우리가 DuplicateBB에서 사용한 것과 같은 입력으로부터 시작하자:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_duplicate_bb.c -o input_for_duplicate_bb.ll
```

 이제 우리는 DuplicateBB와 MergeBB(이 순서대로)를 foo에 적용할 것이다. 다시 말하지만 DuplicateBB는 RIV를 필요로 하기에 총 3개의 플러그인을 로드해야함을 뜻한다:

```bash
$LLVM_DIR/bin/opt -load-pass-plugin <build_dir>/lib/libRIV.so -load-pass-plugin <build_dir>/lib/libMergeBB.so -load-pass-plugin <build-dir>/lib/libDuplicateBB.so -passes=duplicate-bb,merge-bb -S input_for_duplicate_bb.ll -o merge_after_duplicate.ll
```

 그리고 여기 출력이 있다:

```llvm
define i32 @foo(i32) {
lt-if-then-else-0:
  %1 = icmp eq i32 %0, 0
  br i1 %1, label %lt-clone-2-0, label %lt-clone-2-0

lt-clone-2-0:
  br label %lt-tail-0

lt-tail-0:
  ret i32 1
}
```

 DuplicateBB에 의해 생성된 출력과 이것을 비교하자. 복사본 중 오직 하나인 lt-clone-2-0만이 보존되어졌고, lt-if-then-else-0는 그에 맞춰 업데이트된다. if 조건의 값과 상관없이(더 정확히는, 변수 %1) control flow는 lt-clone-2-0으로 점프한다.

### FindFCmpEq

 FindFCmpEq 패스는 직접 두 값의 동일성을 검사하는 모든 부동 소수점 비교 명령을 찾는다. 이러한 종류의 비교는 부동 소수점 연산이 내재하는 버림 오류로 인한 논리적 문제의 지표가 종종 될 수 있기 때문에 중요하다.

 FindFCmpEq는 두 패스로 구현된다: 분석 패스(FindFCmpEq)와 출력 패스(FindFCmpEqPrinter)이다. 기존 구현(FindFCmpEqWrapper)는 이 두 패스를 동시에 사용한다.

 input_for_fcmp_eq.ll을 사용해 FindFCmpEq를 테스트한다.

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
# Generate the input file
$LLVM_DIR/bin/clang -emit-llvm -S -c <source_dir>/inputs/input_for_fcmp_eq.c -o input_for_fcmp_eq.ll
# Run the pass
$LLVM_DIR/bin/opt --load-pass-plugin <build_dir>/lib/libFindFCmpEq.so --passes="print<find-fcmp-eq>" -disable-output input_for_fcmp_eq.ll
```

 발견된 직접 부동 소수점 등식 비교 명령이 나열된 다음 출력을 볼 수 있을 것이다:

```llvm
Floating-point equality comparisons in "sqrt_impl":
  %cmp = fcmp oeq double %0, %1
Floating-point equality comparisons in "compare_fp_values":
  %cmp = fcmp oeq double %0, %1
```

### ConvertFCmpEq

 ConvertFCmpEq 패스는 FindFCmpEq의 분석 결과를 사용해 직접 부동 소수점 등식 명령을 미리 계산된 버림 한계점을 사용하여 논리적으로 동일한 명령으로 바꾸는 변형이다.

 FindFCmpEq에서 그랬던 것처럼, input_for_fcmp_eq.ll을 사용해 ConvertFCmpEq를 테스트한다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -Xclang -disable-O0-optnone \
  -c <source_dir>/inputs/input_for_fcmp_eq.c -o input_for_fcmp_eq.ll
$LLVM_DIR/bin/opt --load-pass-plugin <build_dir>/lib/libFindFCmpEq.so \
  --load-pass-plugin <build_dir>/lib/libConvertFCmpEq.so \
  --passes=convert-fcmp-eq -S input_for_fcmp_eq.ll -o fcmp_eq_after_conversion.ll
```

 레거시 구현의 경우, opt 커맨드는 다음처럼 바뀔 것이다:

```bash
$LLVM_DIR/bin/opt -load <build_dir>/lib/libFindFCmpEq.so \
  <build_dir>/lib/libConvertFCmpEq.so -convert-fcmp-eq \
  -S input_for_fcmp_eq.ll -o fcmp_eq_after_conversion.ll
```

 libFindFCmpEq.so와 libConvertFCmpEq.so 둘 다 반드시 로드되어야 하며 로드 순서도 중요하다. ConvertFCmpEq가 FindFCmpEq를 필요로 하기 때문에, 그것의 라이브러리는 ConvertFCmpEq 전에 반드시 로드되어져야 한다. 만약 두 패스 모두 같은 라이브러리의 부분으로 빌드되어졌다면, 이는 필요하지 않다.

 변형 후, 두 fcmpoeq 명령어는 IEEE 754 이중 정밀 기계 엡실론 상수(IEEE 754 double-precision machine epsilon constant)를 반올림 임계값으로 사용하여 차이 기반 fcmpolt 명령어로 변환됩니다:

```llvm
  %cmp = fcmp oeq double %0, %1
```

… 는 이제 다음과 같아진다:

```llvm
  %3 = fsub double %0, %1
  %4 = bitcast double %3 to i64
  %5 = and i64 %4, 9223372036854775807
  %6 = bitcast i64 %5 to double
  %cmp = fcmp olt double %6, 0x3CB0000000000000
```

 값은 서로 감산되고 그들 차이의 절대값이 계산된다. 이 절대적인 차이가 기계의 엡실론 값보다 작으면 원래의 두 부동소수점 값이 동일한 것으로 간주된다.

## 디버깅

 디버거를 실행하기 전, 아마 LLVM_DEBUG와 STATISTIC macro의 결과를 분석하길 원할 수 있다. 예를 들어, MBAAdd의 경우:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
$LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.so -passes=mba-add input_for_mba.ll -debug-only=mba-add -stats -o out.ll
```

 커맨드 라인에 있는 -debug-only=mba-add와 -stats 플래그가 다음과 같은 출력을 활성화한다.

```llvm
  %12 = add i8 %1, %0 ->   <badref> = add i8 111, %11
  %20 = add i8 %12, %2 ->   <badref> = add i8 111, %19
  %28 = add i8 %20, %3 ->   <badref> = add i8 111, %27
===-------------------------------------------------------------------------===
                          ... Statistics Collected ...
===-------------------------------------------------------------------------===

3 mba-add - The # of substituted instructions
```

 여러분도 보시다시피, MBAAdd로부터 좋은 요약을 얻었다. 많은 경우 뭐가 잘못되었는지에 대해 충분히 이해할 수 있을 정도로 충분할 것이다. 이러한 매크로가 작동하기 위해서 llvm과 llvm-tutor의 디버그 빌드가 필요함을 명심해야 한다(DCMAKE_BUILD_TYPE=Release 대신 DCMAKE_BUILD_TYPE=Debug 사용)

 더 까다로운 문제의 경우 디버거를 사용하라. 아래에서 MBAAdd를 어떻게 디버그하는지 설명한다. 보다 구체적으로 MBAAdd::run 진입 시 breakpoint를 설정하는 방법이다. 이것이 당신이 시작하기에 충분하기를 바란다.

### Mac OS X

 OS X의 기본 디버거는 LLDB이다. 당신은 보통 다음처럼 사용할 것이다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
lldb -- $LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.dylib -passes=mba-add input_for_mba.ll -o out.ll
(lldb) breakpoint set --name MBAAdd::run
(lldb) process launch
```

 혹은, 똑같이, LLDB alias를 사용하여:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
lldb -- $LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.dylib -passes=mba-add input_for_mba.ll -o out.ll
(lldb) b MBAAdd::run
(lldb) r
```

이 시점에서, LLDB는 MBAAdd::run 진입 시 break된다.

### Ubuntu

 대부분의 Linux 시스템에서, GDB는 가장 유명한 디버거이다. 보통 세션은 다음처럼 보일 것이다:

```bash
export LLVM_DIR=<installation/dir/of/llvm/15>
$LLVM_DIR/bin/clang -emit-llvm -S -O1 <source_dir>/inputs/input_for_mba.c -o input_for_mba.ll
gdb --args $LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.so -passes=mba-add input_for_mba.ll -o out.ll
(gdb) b MBAAdd.cpp:MBAAdd::run
(gdb) r
```

이 시점에서, GDB는 MBAAdd::run의 진입 시 break된다.

# Part 2: LLVM의 패스들

## LLVM의 패스 매니저에 대해

 LLVM은 꽤나 복잡한 프로젝트(부드럽게 말하자면)이고 패스가 그 중심에 있다. 이것은 모든 다중 패스 컴파일러에게 진실이다. 패스를 관리하기 위해, 컴파일러는 패스 매니저가 필요하다. LLVM은 현재 한 개가 아닌 두 개의 패스 매니저를 사용하고 있다. 이것은 중요한데, 왜냐하면 어떤 패스 매니저를 사용할지 결정하는 것에 따라 패스의 구현의 생김이 약간 달라지기 때문이다.

### Overview of Pass Managers in LLVM

 전에 언급했듯이, LLVM 내엔 두 패스 매니저가 있다.

- New Pass Manager aka Pass Manager(이것이 코드 기반에서 언급되는 방식이다) LLVM 내의 최적화 파이프라인에서 기본 패스 매니저이다.
- Legacy Pass Manager, 많은 시간 동안 기본 패스 매니저로 사용되어 졌으나 더 이상 사용되지 않고 LLVM 14 이후 삭제됨

 legacy Pass Manager가 사용되어지지 않게 되었기 때문에, 당신의 패스를 위해선 새 패스 매니저를 사용하는 것이 아마 최고일 것이다. 만약 당신이 이미 사용하고 있다면, 좋다! 그렇지 않아도 걱정하진 마라, legacy에서 new 패스 매니저로 패스를 복사하는 것은 상대적으로 직관적이다. llvm-tutor에서 대부분의 패스는 두 패스 매니저에서 동시에 작동한다.

### New vs Legacy PM When Running Opt

 패스 매니저 간에 차이점을 설명하기 위해, MBAAdd를 실행해보자. 이것이 new 패스 매니저와 같이 사용하는 방법이다:

```bash
$LLVM_DIR/bin/opt -S -load-pass-plugin <build_dir>/lib/libMBAAdd.so -passes=mba-add input_for_mba.ll -o out.ll
```

 그리고 이것이 Legacy 패스 매니저와 실행하는 방법이다:

```bash
$LLVM_DIR/bin/opt -S --enable-new-pm=0 -load <build_dir>/lib/libMBAAdd.so -legacy-mba-add input_for_mba.ll -o out.ll
```

 여기엔 3가지 차이가 있다:

- 플러그인을 로드하는 방법: -load-pass-plugin vs -load,
- 실행할 패스/플러그인을 특정하는 방법: -passes=mba-add vs -legacy-mba-add,
- Legacy 패스 매니저를 사용할 때, new PM을 —enable-new-pm=0으로 비활성화시켜줘야 함

 이 차이점은 new 패스 매니저가 단순히 패스 파이프라인을 정의(-passes=)해주기만 해도 된다는 사실에서 기인한다. 반면 Legacy Pass Manager의 경우 opt를 위해 새 커맨드 라인 옵션을 등록해주어야한다.

## 분석 패스 vs 변형 패스

 패스의 구현은 분석 혹은 변형 패스인지에 따른다.

- 변형 패스는 일반적으로 PassInfoMixin에서 상속된다,
- 분석 패스는 AnalysisInfoMixin에서 상속된다.

 이것이 New 패스 매니저의 중요한 특징 중 하나이며 분석과 변형 패스를 매우 분명하게 나눈다. 분석 패스는 bookkeeping이 좀 더 필요하므로 코드가 좀 더 필요하다. 예를 들어, New 패스 매니저에서 식별할 수 있도록 AnalysisKey 인스턴스를 추가해야 한다.

 작은 스탠드얼론 예시의 경우, 분석과 변형 패스간의 차이가 덜 나게 된다. HelloWorld가 좋은 예시이다. 이것은 입력 모듈을 변형시키지 않고, 따라서 실제로 이는 분석 패스이다. 하지만, 구현을 가능한 한 간단하게 하기 위해, 변형 패스를 위한 API를 사용했다.

 llvm-tutor 내에서 다음 패스들은 참조 분석 및 변형 예시로 사용할 수 있다:

- OpcodeCounter - 분석 패스
- MBASub - 변형 패스

 다른 예시 또한 llvm의 관습을 고수하지만, 다른 복잡성을 포함하고 있을 수 있다. 하지만, HelloWorld의 경우에만 엄격성보다 단순성이 선호되었다(즉, 변환이나 분석 패스가 둘 다 아니다).

### Printing passes for the new pass manager

 분석 패스를 위한 출력 패스는 기본적으로 다음과 같은 변형 패스이다:

- 기존 패스로부터 분석 결과를 요청한다, 그리고
- 결과를 출력한다.

 다시 말해, 이것은 그저 wrapper 패스이다. 이러한 패스는 print<analysis-pass-name> 커맨드라인 옵션 아래에 등록하는 관습이 있다.

## 동적 플러그인 vs 정적 플러그인

 기본적으로, llvm-tutor 내의 모든 예시는 동적 플러그인으로 빌드되었다. 하지만, LLVM 동적, 정적 플러그인 둘 다를 위한 기반을 제공한다([documentation](https://llvm.org/docs/WritingAnLLVMPass.html#building-pass-plugins)). 정적 플러그인은 정적으로 당신의 실행 파일(opt)에 연결된 단순한 라이브러리이다. 이 방법으로, 동적 플러그인과 다르게, 런타임 때 -load-pass-plugin으로 로드되어질 필요가 없다.

 정적 플러그인은 일반적으로 llvm-project/llvm 내에서 개발되며, llvm-tutor의 모든 예는 이러한 방식으로 작동하도록 조정될 수 있다. static_registration.sh을 사용하여 MBASub에 대해 수행될 수 있는지 확인할 수 있다. 이 스크립트는 다음과 같다:

- 필요한 소스와 테스트 파일을 llvm-project/llvm에 복사
- 내부 CMake 스크립트를 수정하여 내부 트리 버전 MBASub이 실제 빌드되도록 함
- MBA를 위한 내부 트리 테스트에서 -load와 -load-pass-plugin 제거

 이 스크립트는 llvm-project/llvm을 수정하지만 llvm-tutor은 온전하게 놔둠을 명심하자. 스크립트를 실행하고 난 후 opt를 다시 빌드해야 한다. 두 추가적인 CMake 플래그가 설정되어져야 하며 LLVM_BUILD_EXAMPLES와 LLVM_MBASUB_LINK_INTO_TOOLS이다:

```bash
# LLVM_TUTOR_DIR: directory in which you cloned llvm-tutor
cd $LLVM_TUTOR_DIR
# LLVM_PROJECT_DIR: directory in which you cloned llvm-project
bash utils/static_registration.sh --llvm_project_dir $LLVM_PROJECT_DIR
# LLVM_BUILD_DIR: directory in which you previously built opt
cd $LLVM_BUILD_DIR
cmake -DLLVM_BUILD_EXAMPLES=On -DLLVM_MBASUB_LINK_INTO_TOOLS=On .
cmake --build . --target opt
```

 한 번 opt가 다시 빌드되면, MBASub는 opt와 정적으로 연결되었을 것이다. 이제 다음과 같이 실행 가능하다:

```bash
$LLVM_BUILD_DIR/bin/opt --passes=mba-sub -S $LLVM_TUTOR_DIR/test/MBA_sub.ll
```

 이번엔 MBASub를 로드하기 위해 -load-pass-plugin을 사용할 필요가 없음을 명심하자. 정적 등록을 위해 필요한 단계에 대해 더 깊게 알고 싶다면, static_registation.sh를 살펴보거나 다음을 실행해보자:

```bash
cd $LLVM_PROJECT_DIR
git diff
git status
```

 이는 llvm-project/llvm 내부의 스크립트로 인해 바뀐 모든 변화를 출력할 것이다.

## LLVM 내의 최적화 패스

 당신 스스로의 변형 혹은 분석 패스를 작성하는 것에서 떨어져서, 아마 LLVM 내부의 사용 가능한 패스와 친해지고 싶을 수 있다. 이는 LLVM이 어떻게 작동하는지, 무엇이 LLVM을 강력하고 성공적으로 만들었는지 배울 수 있는 좋은 자원이다. 또한 보통 컴파일러가 어떻게 작동하는지 알아낼 수 있는 좋은 자원이기도 하다. 따라서, 많은 패스들이 컴파일러 개발의 이론으로부터 알려진 기본적인 개념을 구현했다.

 LLVM 내의 사용가능한 패스들의 목록은 조금 벅찰 수 이다. 아래는 좋은 시작 포인트인 선택된 소수의 목록이다. 각 진입점은 LLVM내의 구현으로 향하는 링크와 짧은 설명 그리고 llvm-tutor 내부의 사용가능한 테스트 파일로 향하는 링크를 담고 있다. 이 테스트 파일은 해당하는 패스에 대한 주석이 달려 있는 테스트 케이스의 모음을 포함하고 있다. 이러한 테스트의 목적은 비교적 간단한 예시를 통해 테스트되는 패스의 기능을 설명하는 것에 두고 있다.

| 이름 | 설명 | llvm-tutor 내 테스트 파일 |
| --- | --- | --- |
| dce | Dead Code 제거 | dec.ll |
| memcpyopt | memcpy에 대한 호출 최적화(예를 들어, memset으로 교체) | memcpyopt.ll |
| reassociate | reassociate(예를 들어, 4+(x+5)→x+(4+5)). 이는 LICM 같은 추가적인 최적화를 가능하게 함 | reassociate.ll |
| always-inline | alwaysinline으로 함수를 언제나 인라인으로 작성 | always-inline.ll |
| loop-deletion | 사용되지 않는 루프 제거 | loop-deletion.ll |
| licm | https://en.wikipedia.org/wiki/Loop-invariant_code_motion(루프 내 필요없는 연산 루프 외부로 이동) | licm.ll |
| slp | https://llvm.org/docs/Vectorizers.html#the-slp-vectorizer | slp_x86.ll, slp_aarch64.ll |

 이 리스트는 작고, 스탠드얼론 예시를 통해 상대적으로 쉽게 설명되는 LLVM의 변형 패스들에 집중한다. 다음처럼 개별적인 테스트를 실행해볼 수 있다:

```bash
lit <source/dir/llvm/tutor>/test/llvm/always-inline.ll
```

 개별적인 패스들을 실행하기 위해서, 테스트 파일로부터 하나의 실행 라인을 추출한 후 실행할 수 있다:

```bash
$LLVM_DIR/bin/opt -inline-threshold=0 -always-inline -S <source/dir/llvm/tutor>/test/llvm/always-inline.ll
```

# 참조

Below is a list of LLVM resources available outside the official online documentation that I have found very helpful. Where possible, the items are sorted by date.

- **LLVM IR**
    - *”LLVM IR Tutorial-Phis,GEPs and other things, ohmy!”*, V.Bridgers, F. Piovezan, EuroLLVM, ([slides](https://llvm.org/devmtg/2019-04/slides/Tutorial-Bridgers-LLVM_IR_tutorial.pdf), [video](https://www.youtube.com/watch?v=m8G_S5LwlTo&feature=youtu.be))
    - *"Mapping High Level Constructs to LLVM IR"*, M. Rodler ([link](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/))
- **Examples in LLVM**
    - Control Flow Graph simplifications: [llvm/examples/IRTransforms/](https://github.com/llvm/llvm-project/tree/release/13.x/llvm/examples/IRTransforms)
    - Hello World Pass: [llvm/lib/Transforms/Hello/](https://github.com/llvm/llvm-project/blob/release/13.x/llvm/lib/Transforms/Hello)
    - Good Bye World Pass: [llvm/examples/Bye/](https://github.com/llvm/llvm-project/tree/release/13.x/llvm/examples/Bye)
- **LLVM Pass Development**
    - *"Writing an LLVM Optimization"*, Jonathan Smith [video](https://www.youtube.com/watch?v=MagR2KY8MQI&t)
    - *"Getting Started With LLVM: Basics "*, J. Paquette, F. Hahn, LLVM Dev Meeting 2019 [video](https://www.youtube.com/watch?v=3QQuhL-dSys&t=826s)
    - *"Writing an LLVM Pass: 101"*, A. Warzyński, LLVM Dev Meeting 2019 [video](https://www.youtube.com/watch?v=ar7cJl2aBuU)
    - *"Writing LLVM Pass in 2018"*, Min-Yih Hsu [blog](https://medium.com/@mshockwave/writing-llvm-pass-in-2018-preface-6b90fa67ae82)
    - *"Building, Testing and Debugging a Simple out-of-tree LLVM Pass"* Serge Guelton, Adrien Guinet, LLVM Dev Meeting 2015 ([slides](https://llvm.org/devmtg/2015-10/slides/GueltonGuinet-BuildingTestingDebuggingASimpleOutOfTreePass.pdf), [video](https://www.youtube.com/watch?v=BnlG-owSVTk&index=8&list=PL_R5A0lGi1AA4Lv2bBFSwhgDaHvvpVU21))
- **Legacy vs New Pass Manager**
    - *"New PM: taming a custom pipeline of Falcon JIT"*, F. Sergeev, EuroLLVM 2018 ([slides](http://llvm.org/devmtg/2018-04/slides/Sergeev-Taming%20a%20custom%20pipeline%20of%20Falcon%20JIT.pdf), [video](https://www.youtube.com/watch?v=6X12D46sRFw))
    - *"The LLVM Pass Manager Part 2"*, Ch. Carruth, LLVM Dev Meeting 2014 ([slides](https://llvm.org/devmtg/2014-10/Slides/Carruth-TheLLVMPassManagerPart2.pdf),[video](http://web.archive.org/web/20160718071630/http://llvm.org/devmtg/2014-10/Videos/The%20LLVM%20Pass%20Manager%20Part%202-720.mov))
    - *”Passes in LLVM, Part 1”*, Ch. Carruth, EuroLLVM 2014 ([slides](https://llvm.org/devmtg/2014-04/PDFs/Talks/Passes.pdf), [video](https://www.youtube.com/watch?v=rY02LT08-J8))
- **LLVM Based Tools Development**
    - *"Introduction to LLVM"*, M. Shah, Fosdem 2018, [link](http://www.mshah.io/fosdem18.html)
    - *"Building an LLVM-based tool. Lessons learned"*, A. Denisov, [blog](https://lowlevelbits.org/building-an-llvm-based-tool.-lessons-learned/), [video](https://www.youtube.com/watch?reload=9&v=Yvj4G9B6pcU)

# Credits

This is first and foremost a community effort. This project wouldn't be possible without the amazing LLVM online documentation, the plethora of great comments in the source code, and the llvm-dev mailing list. Thank you!

It goes without saying that there's plenty of great presentations on YouTube, blog posts and GitHub projects that cover similar subjects. I've learnt a great deal from them - thank you all for sharing! There's one presentation/tutorial that has been particularly important in my journey as an aspiring LLVM developer and that helped to democratise out-of-source pass development:

"Building, Testing and Debugging a Simple out-of-tree LLVM Pass" Serge Guelton, Adrien Guinet (slides, video)
Adrien and Serge came up with some great, illustrative and self-contained examples that are great for learning and tutoring LLVM pass development. You'll notice that there are similar transformation and analysis passes available in this project. The implementations available here reflect what I found most challenging while studying them.

# License

The MIT License (MIT)

Copyright (c) 2019 Andrzej Warzyński

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
