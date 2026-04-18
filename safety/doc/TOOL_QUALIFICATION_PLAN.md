# Software Tool Qualification Plan

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-TQ-001 |
| 표준 참조 | ISO 26262 Part 8 Cl.11 |
| 작성일 | 2026-04-18 |
| 버전 | 1.0 |
| 상태 | Draft |

---

## 1. 목적 (Purpose)

본 문서는 OpenSafetyRTOS 개발에 사용되는 소프트웨어 도구의 신뢰성을 확보하기 위한 도구 자격부여(Tool Qualification) 계획을 정의한다.  
ISO 26262 Part 8 Cl.11의 요구사항에 따라 각 도구의 Tool Confidence Level(TCL)을 결정하고, TCL에 따른 적절한 자격부여 방법을 수행한다.

---

## 2. Tool Confidence Level (TCL) 결정 기준

TCL은 도구 오류가 개발 산출물에 미치는 영향과 오류 검출 가능성에 따라 결정된다.

| TCL | 정의 | 자격부여 필요 수준 |
|-----|------|-------------------|
| TCL1 | 도구 오류가 발생하더라도 다른 수단(코드 리뷰, 테스트 등)으로 검출 가능한 도구 | 자격부여 불필요 — 결과물에 대한 검토로 충분 |
| TCL2 | 도구 오류가 부분적으로 검출 가능한 도구 (일부 시나리오에서 오류 검출 불가) | 검증(Validation) 필요 — 알려진 오류 케이스를 활용한 테스트 수행 |
| TCL3 | 도구 오류가 검출되기 어려운 도구 (출력 결과가 직접 안전 기능에 영향) | 완전한 자격부여(Full Qualification) 필요 — 공급사 인증 또는 검증 테스트 스위트 수행 |

> **참고**: TCL은 도구의 사용 방식(Tool Use Case)에 따라 달라질 수 있다. 동일 도구라도 사용 방법에 따라 TCL이 변경될 수 있으므로 도구 버전 업그레이드 또는 사용 방식 변경 시 재평가한다.

---

## 3. 도구 목록 및 TCL (Tool List and TCL)

| 도구 | 버전 | 용도 | TCL | 자격부여 방법 | 증거 위치 |
|------|------|------|-----|------------|-----------|
| GCC arm-none-eabi | 12.x 이상 | C/C++ 컴파일러 (ASIL-D 코드 포함) | TCL3 | 컴파일러 검증 테스트 스위트(GCC Torture Tests 등) 실행 또는 인증된 컴파일러 버전(예: TASKING, IAR) 사용 | `safety/doc/tool-qualification/gcc-qualification.md` |
| CMake | 3.20 이상 | 빌드 시스템 생성기 | TCL1 | 빌드 결과물(바이너리, 링크 맵)을 수동 검토하여 의도한 소스가 포함되었는지 확인 | 빌드 로그 및 코드 리뷰로 대체 |
| Cppcheck | 2.x 이상 | 정적 분석 (결함 검출) | TCL2 | 알려진 오류 패턴이 포함된 테스트 케이스로 Cppcheck 실행하여 검출 여부 확인 | `safety/doc/tool-qualification/cppcheck-validation.md` |
| gcov / lcov | GCC 내장 / 1.x | 코드 커버리지 측정 | TCL2 | 수동으로 작성한 커버리지 검증 케이스로 gcov 출력값의 정확성 확인 | `safety/doc/tool-qualification/gcov-validation.md` |
| Unity / CMock | 2.x | 단위 테스트 프레임워크 | TCL2 | Unity/CMock 프레임워크 자체 테스트 스위트 실행 및 결과 확인 | `safety/doc/tool-qualification/unity-validation.md` |
| PC-lint Plus / Polyspace | 최신 상용 버전 | MISRA-C:2012 준수 검사 | TCL3 | 공급사(Gimpel / MathWorks) 제공 인증서(Certificate of Conformance) 활용 | `safety/doc/tool-qualification/lint-polyspace-cert.md` |
| arm-none-eabi-objdump | binutils 내장 | 바이너리 역어셈블 및 검사 | TCL1 | 출력 결과 수동 검토로 충분 (결정에 자동 판단 불사용) | 코드 리뷰 이력으로 대체 |

---

## 4. 자격부여 증거 보관 위치

모든 도구 자격부여 관련 증거 문서는 아래 디렉토리에 보관한다.

```
safety/doc/tool-qualification/
├── gcc-qualification.md          # GCC 컴파일러 검증 결과
├── cppcheck-validation.md        # Cppcheck 검증 테스트 결과
├── gcov-validation.md            # gcov/lcov 검증 결과
├── unity-validation.md           # Unity/CMock 검증 결과
└── lint-polyspace-cert.md        # PC-lint Plus / Polyspace 인증서 참조
```

각 증거 문서는 다음 정보를 포함해야 한다.
- 도구 이름 및 버전
- 자격부여 수행일
- 수행자
- 사용된 검증 방법 및 테스트 케이스 참조
- 결과 (합격/불합격)
- 승인자 (Agent-QA)

---

## 5. 도구 버전 고정 (Tool Version Locking)

### 5.1 버전 명시

- `CMakeLists.txt` 에 각 도구의 최소 요구 버전을 명시한다.
- 예시:
  ```cmake
  cmake_minimum_required(VERSION 3.20)
  # GCC arm-none-eabi >= 12.0 required
  ```

### 5.2 CI에서 버전 검증

- CI/CD 파이프라인에서 도구 버전을 자동으로 확인하는 스텝을 추가한다.
- 버전 불일치 시 빌드를 실패 처리한다.
- 예시 확인 명령:
  ```bash
  arm-none-eabi-gcc --version | grep -E "12\.[0-9]+" || exit 1
  cmake --version | grep -E "3\.(2[0-9]|[3-9][0-9])" || exit 1
  ```

---

## 6. 재자격부여 (Re-qualification)

다음 사항 발생 시 해당 도구에 대해 재자격부여를 수행한다.

| 트리거 | 대상 도구 | 재자격부여 범위 |
|--------|-----------|----------------|
| 도구 버전 업그레이드 | 업그레이드된 도구 | 전체 자격부여 재수행 |
| 도구 사용 방식 변경 | 변경된 도구 | 변경된 Use Case 범위 |
| 자격부여 테스트 케이스 오류 발견 | 해당 도구 | 오류 수정 후 재수행 |
| 새로운 도구 추가 | 신규 도구 | TCL 결정부터 전체 수행 |

재자격부여 결과는 기존 증거 문서에 이력으로 추가하거나 신규 버전 문서로 발행한다.  
Agent-QA의 승인을 받아야 한다.
