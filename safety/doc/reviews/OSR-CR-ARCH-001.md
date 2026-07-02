# 통합 검토 기록: OSR-CR-ARCH-001 (아키텍처 및 교차 문서 일관성 검토)

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CR-ARCH-001 |
| 검토 대상 | 아키텍처 전제(MPU/특권 모델, 실행 모델) 및 Phase 1 산출물 전반의 교차 문서 일관성 |
| 검토 유형 | 통합 검토 기록 — ISO 26262-2:2018 Cl.6.4.9 확인 조치 구조 준용 |
| 검토자 | Agent-QA — **독립성 주의: sub-I2** (§5 독립성 선언 참조) |
| 검토 기준 | ISO 26262-2:2018 Cl.6.4.9 및 Table 1; Part 3 Cl.6/Cl.7; Part 6 Cl.7; Part 8 Cl.7/9/11 |
| 입력 문서 | HARA/FSC/SSRS/SAFETY_CASE/SAFETY_PLAN/CM/TQ/PR/QA 체크리스트/FMEA/TEAM.md/ADR-001~004/ARCHITECTURE.md/README, 검토 기록 OSR-CR-* 7건, 저장소 상태(git tag, 디렉토리) |
| 검토일 | 2026-07-02 |

**목적:** 2026-07-02 수행된 아키텍처 및 교차 문서 일관성 검토의 발견 사항(F-01~F-14)과
각 처분(disposition)을 기록한다. 본 기록은 당일 변경 세트(ADR-005, OSR-AOU-001, HARA v1.3,
FSC v1.3, SSRS v1.2, SAFETY_CASE v0.6, 계획 문서 개정, 초기 소스 코드)의 **감사 추적
(audit trail)** 이다.

---

## 1. 결정 사항 레지스터 (Decisions Applied 2026-07-02)

| 결정 ID | 내용 | 반영 위치 |
|--------|------|----------|
| D1 | 특권/FFI 모델 확립: ARMv7-M에는 MPU 설정 잠금이 없음을 인정. QM 태스크는 FreeRTOS ARM_CM4_MPU 포트로 비특권 실행(예방적 보호), FreeRTOS 커널은 특권 유지 — 커널 결함에 대한 MPU 보호는 주기적 MPU 설정 무결성 검사에 의한 **검출적** 보호로 재정의 | docs/ADR-005 |
| D2 | SafetyFunction은 FreeRTOS 태스크가 아닌 **독립 타이머 ISR**에서 실행 (스케줄러 의존 제거) | docs/ADR-005, kernel/safety |
| D3 | HARA ASIL 표기 정정: H-05/H-07/H-09/H-10을 Table 4 정합 등급(ASIL-B)으로 기재하고 문서화된 보수적 상향으로 ASIL-C 적용 (H-04 패턴 동일; HARA-NI-005~008) | HARA v1.3, OSR-CR-HARA-003 |
| D4 | Safe State Level 3 = **즉시 AIRCR.SYSRESETREQ**, 워치독은 백스톱 (기존 "while(1) + 워치독 만료 대기"의 지연 제거) | docs/ADR-005, kernel/safety/src/safe_state 구현 |
| D5 | DMA는 MPU로 게이트되지 않음 → DMA 마스터 통제는 통합자 의무로 위임 (**AoU-07**) | docs/ADR-005, safety/doc/AOU.md |
| D6 | HARA §1.4 분석 범위 정정 — 안전 메커니즘을 무력화하는 HW 고장(클럭/전원/MPU 설정 소실)을 범위에 포함 | HARA v1.3 |
| D7 | SG-04를 검증 가능한 형태로 재정식화 | HARA v1.3 |
| D8 | 정본 AoU 레지스트리 신설: OSR-AOU-001 v1.0 (AoU-01..10) | safety/doc/AOU.md |
| D9 | README 주장 수위 하향: "targeting ASIL-D" (달성 주장 아님) | README |
| D10 | 내부 검토를 검증 지원 조치로 재분류; 외부 ISA를 늦어도 Phase 2 종료 전 투입 (Phase 1 산출물 소급 I3 확인 검토) | docs/TEAM.md v0.5, SAFETY_PLAN v1.1 §6a |

---

## 2. 발견 사항 및 처분 (Findings F-01~F-14)

### 2.1 아키텍처/기술 발견 사항

| ID | 발견 사항 | 심각도 | 처분 (Disposition) |
|----|----------|--------|-------------------|
| F-01 | **특권/MPU 잠금 갭:** 기존 논증("MPU locked after init")은 ARMv7-M에 존재하지 않는 잠금 기능을 전제. 특권 소프트웨어(FreeRTOS 커널 포함)는 MPU를 언제든 재설정 가능 → "예방적" FFI 논증 불성립 | Major | **Resolved** — D1 (ADR-005): QM 비특권화 + 검출적 MPU 무결성 검사. 이슈 기록 OSR-ISSUE-001 |
| F-02 | **DMA/NVIC 비강제성:** DMA 마스터와 NVIC 경유 간섭 경로는 MPU로 강제 차단 불가. FSR-02-04의 "차단"은 하드웨어적으로 보장 불능 | Major | **Resolved** — D5: DMA 통제 AoU-07 위임 + 소프트웨어 검증(`safety_dma_validate_transfer()`)은 검출 계층으로 재정의; NVIC/VTOR 보호는 ADR-005 특권 모델로 재논증. OSR-ISSUE-001 연동 |
| F-03 | **SafetyFunction 태스크 실행 모델 모순:** "FreeRTOS(QM)가 스케줄하는 태스크가 FreeRTOS 고장을 감시"하는 순환 의존 (FMEA-003 자체가 이 취약성을 인정) | Major | **Resolved** — D2: 독립 타이머 ISR 실행으로 스케줄러 의존 제거. FMEA v0.2에 재평가 노트 추가 |
| F-04 | **Level 3 워치독 기아 지연:** 기존 Level 3(인터럽트 비활성 + while(1) + 워치독 만료 대기)은 최대 워치독 타임아웃만큼 위험 상태 지속 | Major | **Resolved** — D4: 즉시 AIRCR.SYSRESETREQ, 워치독 백스톱 |
| F-05 | **0xSAFEA55A 리터럴:** 매직 넘버 'S'는 16진수 자릿수가 아님 — 컴파일 불가능한 표기가 설계 문서에 잔존 | Minor | **Resolved** — 구현(safe_state)에서 유효한 리터럴로 확정, 문서 정렬 |
| F-06 | **CRC 명명 불일치:** CRC-32 다항식 표기(0x04C11DB7)와 구현 방식(반사/비반사, 초기값/최종 XOR) 명명이 문서 간 불일치 | Minor | **Resolved** — crc32 구현 모듈에서 파라미터 세트 명시, SSRS 참조 정렬 |
| F-07 | **단일 리전 QM Flash+SRAM 불가:** "Region 4 = QM Region (Flash + SRAM)"은 불연속 주소 공간을 단일 MPU 리전으로 커버할 수 없어 구현 불가능 | Major | **Resolved** — ADR-005 리전/특권 모델 재작업 및 SSRS v1.2 반영 |

### 2.2 문서/프로세스 발견 사항

| ID | 발견 사항 | 심각도 | 처분 (Disposition) |
|----|----------|--------|-------------------|
| F-08 | **README 과대 주장:** ASIL-D 달성을 암시하는 서술 — 인증 전 SEooC 후보 상태와 불일치 | Minor | **Resolved** — D9: "targeting ASIL-D"로 하향 |
| F-09 | **미해결 참조(dangling references):** OSR-AOU-001이 다수 문서에서 참조되나 파일 부재; AoU ID 충돌(AoU-04/06/08, AoU-XX); TSC 및 INTEGRATION_GUIDE 참조 대상 미작성; CM 계획 ID 체계와 실사용 ID 불일치("SS" vs OSR-SP-001 등) | Major | **부분 Resolved** — D8 (OSR-AOU-001 v1.0, OSR-ISSUE-003), CM_PLAN v1.1 §5.1 ID 체계 정정. **TSC/INTEGRATION_GUIDE는 미작성 잔존** — Phase 2/5 산출물로 계획 추적 |
| F-10 | **조항/표 인용 오류:** 확인 조치를 "Part 2 Cl.8/9/10"으로, HARA를 "Part 3 Cl.15"로 인용 (2018판 정답: Part 2 Cl.6.4.9, Part 3 Cl.6); Table 4 계산 오류 4건(H-05/H-07/H-09/H-10)이 확인 검토 2회를 무지적 통과 | Major | **Resolved** — SAFETY_PLAN v1.1 §6a, QA_REVIEW_CHECKLIST v1.1 (인용 정정); OSR-CR-HARA-003 §2 정오표 + HARA v1.3 (D3); 이슈 기록 OSR-ISSUE-002 |
| F-11 | **상태 모순:** SAFETY_PLAN 상태 필드가 승인 현실과 불일치(SSRS "Pending" 등); FMEA §5의 SSR ID가 승인된 SSRS 번호 체계와 불일치; 계획 문서 "버전 1.0/상태 Draft" 모순; QA 체크리스트 리뷰 이력 표 공란; SAFETY_CASE의 HARA 최종 승인 전환이 존재하지 않는 OSR-CR-HARA-003에 의존 | Major | **Resolved** — SAFETY_PLAN v1.1 (§4a 문서 상태 표), FMEA v0.2 (§5 재정렬), CM/TQ/PR 상태 정합화, QA 체크리스트 v1.1 (이력 표 기입), OSR-CR-HARA-003 발행(본 변경 세트), SAFETY_CASE v0.6 (타 에이전트) |
| F-12 | **검토 조건 무단 완화:** OSR-CR-FSC-002 조건 C2("SG-08 FSR 최소 4건", 전체 "28개 이상")가 FSC v1.2에서 3건/27개로 이행되었음에도 OSR-CR-FSC-003이 근거 없이 "완전 해소" 판정; 런타임 전원 이상 응답 FSR(FSR-08-04 상당) 누락 | Major | **Open** — 이슈 기록 OSR-ISSUE-004: FSC 차기 버전에서 FSR-08-04 추가 또는 3건 충분성의 공학적 근거 문서화 요구. 향후 재검토에 선행 조건 원문 대조표 필수화 (QA 체크리스트 반영) |
| F-13 | **Part 8 실행 증거 갭:** Phase 1 완료 선언에도 베이스라인 태그 0건(`git tag` 공란), `safety/doc/audits/` 부재(기능안전 감사 미수행), `safety/doc/issues/` 공란(검토 Major 지적이 문제 해결 프로세스에 미등록), `safety/doc/tool-qualification/` 공란(자격부여 미수행) | Major | **부분 Resolved** — CM_PLAN v1.1 §3.4 (Phase 1 태그 소급 생성 요구)/§8/§9, TQ_PLAN v1.1 (TI/TD 분석, GCC 방법 확정, Phase 3 전 수행 기한), PR v1.1 + OSR-ISSUE-001~005 소급 등록. **감사 기록(OSR-FSA) 및 태그·자격부여 실물 증거는 잔존 미이행** — 다음 릴리즈/Phase 게이트에서 이행 |
| F-14 | **확인 조치 독립성 부적합:** 단일 유지관리자 운영 AI 에이전트 구조는 I2 미달; ISO 26262-2:2018 Table 1은 HARA 확인 검토에 전 ASIL I3, ASIL-D 확인 조치 전반에 I3를 요구("shall"). 기존 TEAM.md의 "Recommended" 표기는 오기술 | Major (지배적 제약) | **Open (완화 적용)** — D10: TEAM.md v0.5 정정 + 검증 지원 조치 재분류, SAFETY_PLAN v1.1 §6a, ISA를 Phase 2 종료 전 투입 계획. 이슈 기록 OSR-ISSUE-005 (Close 조건: ISA 소급 I3 검토 완료) |

---

## 3. 미결 항목 요약 (Open Items)

| 항목 | 추적 | 기한 |
|------|------|------|
| FSR-08-04 추가 또는 C2 완화 근거 문서화 | OSR-ISSUE-004 | FSC 차기 버전 |
| 외부 ISA 계약 및 Phase 1 산출물 소급 I3 확인 검토 | OSR-ISSUE-005 | **Phase 2 종료 전 (필수)** |
| Phase 1 베이스라인 태그 소급 생성 | CM_PLAN v1.1 §3.4 | 다음 릴리즈(머지) 시 |
| 기능안전 감사 기록(OSR-FSA-001) 최초 작성 및 `safety/doc/audits/` 신설 | QA_REVIEW_CHECKLIST CM2 | Phase 2 게이트 |
| 도구 자격부여 실물 증거 생성 (gcc 외) | TQ_PLAN v1.1 §3a | Phase 3 종료 전 |
| TSC / INTEGRATION_GUIDE 작성 (dangling 참조 해소 잔여분) | F-09 | Phase 2 / Phase 5 |
| FMEA 본문(FMEA-001~005) ADR-005 실행 모델 기준 재평가 | FMEA v0.2 노트 | Phase 2~3 |

---

## 4. 결론

2026-07-02 변경 세트는 F-01~F-11의 아키텍처·문서 결함을 해소하고(각 처분 참조),
F-12/F-13(부분)/F-14를 공개 이슈로 정식 등록하였다. **본 기록 이후의 안전 논증은
ADR-005의 특권/검출적 FFI 모델과 OSR-AOU-001 정본 레지스트리를 기준으로 한다.**
프로젝트의 지배적 제약은 F-14(독립성)이며, 외부 ISA 투입 전까지 모든 내부 승인은
잠정(provisional) 지위임을 재확인한다.

---

## 5. 독립성 선언 (Honest Independence Statement)

본 검토자(Agent-QA)는 검토 대상 산출물 작성 에이전트들과 별개 인스턴스이나, 전원 단일
유지관리자가 운영하는 AI 에이전트로서 ISO 26262-2:2018 Table 1 기준 **I2에 미달(sub-I2)**
한다. 본 기록은 ISO 26262 적합 확인 조치가 아닌 **검증 지원 조치**이며, 외부 ISA 확인
검토의 입력 자료이다 (docs/TEAM.md v0.5, OSR-ISSUE-005).

---

## 6. 서명

| 역할 | 이름 | 날짜 |
|------|------|------|
| 검토자 (Agent-QA — 검증 지원 조치, sub-I2) | Agent-QA | 2026-07-02 |
| Safety Manager 확인 | — (PM/PO 승인 필요) | — |
| 외부 ISA 확인 검토 | — (Phase 2 종료 전 소급 수행 예정) | — |

---

*본 기록은 2026-07-02 변경 세트의 감사 추적 문서로서 인증 증거 패키지에 포함되며,
관련 이슈 기록(OSR-ISSUE-001~005) 및 각 문서 개정 이력과 상호 참조된다.*
