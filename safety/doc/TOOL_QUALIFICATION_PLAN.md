# Software Tool Qualification Plan

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-TQ-001 |
| 표준 참조 | ISO 26262 Part 8 Cl.11 |
| 작성일 | 2026-04-18 |
| 개정일 | 2026-07-02 |
| 버전 | 1.1 |
| 상태 | Released (운영 중) |

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

## 2a. 도구별 TI/TD 분석 (Tool Impact / Tool error Detection) — ISO 26262 Part 8 Cl.11.4.5 (v1.1 신설)

ISO 26262 Part 8 Cl.11에 따라 TCL은 단순 선언이 아니라, 각 사용 사례(Use Case)에 대해
**TI(Tool Impact: 도구 오류가 안전 관련 산출물에 오류를 주입하거나 오류 검출을 실패하게
할 수 있는가)** 와 **TD(Tool error Detection: 그 오류가 예방·검출될 확신 수준)** 를 평가하여
결정한다 (TI1 → TCL1; TI2∧TD1 → TCL1; TI2∧TD2 → TCL2; TI2∧TD3 → TCL3).

| 도구 | 사용 사례(Use Case) | 도구 오류 시나리오 | TI | 오류 검출/예방 수단 | TD | 결정 TCL |
|------|-------------------|------------------|----|------------------|----|---------|
| GCC arm-none-eabi | ASIL-D 소스 → 오브젝트 코드 생성 | 잘못된 코드 생성(miscompilation) — 소스와 의미가 다른 바이너리 | TI2 | 오브젝트 코드는 전수 리뷰되지 않음. 타깃 상 단위/통합 테스트가 일부 오류를 검출하나 체계적 검출 보장 없음 | TD3 | **TCL3** |
| CMake | 빌드 오케스트레이션 (컴파일 대상/순서/플래그 결정) | 소스 누락, 잘못된 플래그 전달, 오래된 오브젝트 링크 | TI2 | 링크 맵/빌드 로그 검토로 포함 소스·플래그 전수 확인 가능, CI에서 클린 빌드 강제 — 높은 확신으로 검출 | TD1 | **TCL1** |
| Cppcheck | 결함 검출 정적 분석 | 실제 결함을 보고하지 않음(false negative, 무음 실패) | TI2 | 2차 도구(PC-lint Plus/Polyspace) 병행, 코드 리뷰, MC/DC 100% 단위 테스트가 부분적으로 중복 검출 | TD2 | **TCL2** |
| gcov / lcov | MC/DC·분기 커버리지 측정 | 미커버 코드를 커버된 것으로 과대 보고 | TI2 | 커버리지 리포트와 테스트 케이스 목록의 수동 대조, 의도적 미커버 케이스로 계측 정합성 확인 | TD2 | **TCL2** |
| Unity / CMock | 단위 테스트 실행/판정 | 실패 테스트를 통과로 오판정 | TI2 | 의도적 실패 테스트(canary)를 CI에 포함하여 프레임워크 판정 로직 상시 검증, 결과 리포트 리뷰 | TD2 | **TCL2** |
| PC-lint Plus / Polyspace | MISRA-C:2012 준수 판정 | MISRA 위반을 보고하지 않음(무음 실패) | TI2 | MISRA 위반의 제3 검출 수단이 제한적 (Cppcheck 일부 중복에 그침) — 보수적으로 TD3 평가 | TD3 | **TCL3** |
| arm-none-eabi-objdump | 바이너리 수동 검사 보조 | 역어셈블 출력 오류 | TI2 | 출력이 자동 판정에 사용되지 않고 전량 수동 검토됨, 맵 파일과 교차 확인 | TD1 | **TCL1** |

> 위 TI/TD 평가는 §3의 TCL 선언을 대체하는 공식 근거이다. 사용 사례가 변경되면
> (예: objdump 출력을 자동 검증 스크립트의 판정 근거로 사용) 해당 행을 재평가한다.

---

## 3. 도구 목록 및 TCL (Tool List and TCL)

| 도구 | 버전 | 용도 | TCL | 자격부여 방법 | 증거 위치 |
|------|------|------|-----|------------|-----------|
| GCC arm-none-eabi | 12.x 이상 (정확 버전은 자격부여 시 고정) | C/C++ 컴파일러 (ASIL-D 코드 포함) | TCL3 | **확정 방법(v1.1, §3a 참조):** 1c 검증(Validation) 경로 — 상용 컴파일러 검증 스위트(예: Solid Sands SuperTest) 또는 동등 수준의 문서화된 검증 스위트를 **사용 언어 서브셋·고정 옵션 세트에 대해** 실행 + 옵션 가드 + MISRA 서브셋 제한. Torture Tests 단독으로는 불충분 | `safety/doc/tool-qualification/gcc-qualification.md` |
| CMake | 3.20 이상 | 빌드 시스템 생성기 | TCL1 | 빌드 결과물(바이너리, 링크 맵)을 수동 검토하여 의도한 소스가 포함되었는지 확인 | 빌드 로그 및 코드 리뷰로 대체 |
| Cppcheck | 2.x 이상 | 정적 분석 (결함 검출) | TCL2 | 알려진 오류 패턴이 포함된 테스트 케이스로 Cppcheck 실행하여 검출 여부 확인 | `safety/doc/tool-qualification/cppcheck-validation.md` |
| gcov / lcov | GCC 내장 / 1.x | 코드 커버리지 측정 | TCL2 | 수동으로 작성한 커버리지 검증 케이스로 gcov 출력값의 정확성 확인 | `safety/doc/tool-qualification/gcov-validation.md` |
| Unity / CMock | 2.x | 단위 테스트 프레임워크 | TCL2 | Unity/CMock 프레임워크 자체 테스트 스위트 실행 및 결과 확인 | `safety/doc/tool-qualification/unity-validation.md` |
| PC-lint Plus / Polyspace | 최신 상용 버전 | MISRA-C:2012 준수 검사 | TCL3 | 공급사(Gimpel / MathWorks) 제공 인증서(Certificate of Conformance) 활용 | `safety/doc/tool-qualification/lint-polyspace-cert.md` |
| arm-none-eabi-objdump | binutils 내장 | 바이너리 역어셈블 및 검사 | TCL1 | 출력 결과 수동 검토로 충분 (결정에 자동 판단 불사용) | 코드 리뷰 이력으로 대체 |

---

## 3a. GCC 컴파일러 자격부여 확정 방법 (v1.1 신설 — 기존 "또는" 선택지 해소)

기존 v1.0의 "Torture Tests 실행 **또는** 인증된 컴파일러 사용"이라는 미확정 선택지를
다음과 같이 확정한다. **GCC Torture Tests 단독 실행은 ASIL-D TCL3 자격부여로 불충분하다**
(테스트 목적이 컴파일러 회귀 검출이지 안전 규격 요구 커버리지가 아니며, 사용 옵션
조합·언어 서브셋에 대한 체계적 커버리지 논증이 없음).

**확정 방법 (ISO 26262 Part 8 Cl.11.4.9 — 1c "Validation of the software tool"):**

1. **정확 버전 고정:** arm-none-eabi-gcc 정확 버전(예: 12.3.rel1)을 자격부여 시점에 고정하고
   CI에서 강제한다 (§5 연계). 버전 변경 시 재자격부여 (§6).
2. **옵션 가드:** 허용 컴파일 옵션 세트(최적화 수준 포함, 예: `-O1` 고정, `-ffreestanding`,
   경고 옵션 등)를 명문화하고, CI가 허용 외 옵션 사용 시 빌드를 실패시킨다.
   자격부여는 이 고정 옵션 세트에 대해서만 유효하다.
3. **언어 서브셋 제한:** ASIL-D 코드에는 MISRA-C:2012 서브셋만 허용 (SSRS §5 연계).
   검증 스위트 커버리지는 이 서브셋 기준으로 논증한다.
4. **검증 스위트 실행:** 상용 컴파일러 검증 스위트(1순위: Solid Sands SuperTest 라이선스
   확보) 또는 — 예산 제약 시 — 문서화된 대체 스위트 조합(GCC 릴리즈 테스트 스위트 +
   MISRA 서브셋 대상 자체 검증 케이스)을 **고정 버전·고정 옵션·타깃(Cortex-M4)** 조합으로
   실행하고 결과를 `gcc-qualification.md`에 기록한다. 대체 조합 사용 시 SuperTest 대비
   커버리지 갭을 명시하고 아래 5번 완화책으로 보강한다.
5. **오류 검출 보강(잔여 위험 완화):** 타깃 상 MC/DC 100% 단위 테스트, 통합 테스트,
   릴리즈 바이너리에 대한 맵 파일/역어셈블 표본 검토를 컴파일러 오류의 2차 검출망으로
   운영한다 (이는 자격부여를 대체하지 않고 보완한다).

| 항목 | 내용 |
|------|------|
| 담당(Owner) | Agent-Build (수행), Agent-QA (승인), Safety Manager (예산/방법 최종 결정) |
| 수행 Phase | **Phase 3 착수 전 결정, Phase 3 종료 전 완료** — ASIL-D 릴리즈 빌드에 사용되기 전 자격부여 증거 확보 필수 |
| 증거 | `safety/doc/tool-qualification/gcc-qualification.md` |

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

---

## 7. 개정 이력 (Revision History)

| 버전 | 날짜 | 작성자 | 변경 내용 |
|------|------|--------|---------|
| 1.0 | 2026-04-18 | Agent-Docs | 최초 작성 (Draft) |
| 1.1 | 2026-07-02 | Agent-Docs | OSR-CR-ARCH-001 F-13 반영: §2a 신설 — 도구별 TI/TD 분석 표 추가 (Cl.11.4.5, TCL 선언의 공식 근거화); §3a 신설 — GCC TCL3 자격부여 방법 확정 (1c 검증 경로: 검증 스위트 + 정확 버전 고정 + 옵션 가드 + MISRA 서브셋; Torture Tests 단독 불충분 명시; 담당 Agent-Build/Agent-QA, Phase 3 종료 전 완료); §3 GCC 행 방법 갱신; 상태 Draft → Released. **주의: `safety/doc/tool-qualification/` 증거 디렉토리는 현재 비어 있음 — 자격부여는 계획만 존재하고 미수행 상태이며 Phase 3 전 수행 필수** |
