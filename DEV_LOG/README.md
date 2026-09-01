# Peglin MFC 개발 기록

이 폴더는 현재 상태의 분석, 버전별 변경 결과, 다음 스프린트 계획을 한곳에서 관리한다.

## 버전 규칙

- 버전 표기: `Version [스프린트 번호].[스프린트 내 기록 번호]`
- 파일명: `Version_[스프린트 번호].[기록 번호].md`
- 다음 계획 파일명: `Next_Version_Plan_[스프린트 번호].md`
- 각 스프린트의 계획에 포함된 **필수 완료 항목을 모두 구현하고 검증한 뒤에만** 다음 스프린트로 이동한다.
- 같은 스프린트에서 추가 분석이나 수정 결과를 기록할 때는 뒤 번호를 올린다. 예: `Version_0.1.md` → `Version_0.2.md`.
- 스프린트가 전환되면 기록 번호를 1부터 다시 시작한다. 예: 스프린트 0 완료 후 첫 기록은 `Version_1.1.md`.

## 현재 상태

| 항목 | 값 |
| --- | --- |
| 현재 스프린트 | Sprint 8 진행 중 |
| 현재 버전 | Version 8.4 |
| 스프린트 목표 | 다양한 화면 환경의 UI 안정성, 콘텐츠 이해도와 제작 도구, 입력 설정 및 시연 품질 개선 |
| 상태 | Sprint 8 P0 4/8 완료 · 게임패드 조준 설정·장치 상태 QA 완료 |

## 문서

- [Version 0.1 — 현재 버전 분석](./Version_0.1.md)
- [Version 0.2 — MFC 개발 환경 구축](./Version_0.2.md)
- [Version 0.3 — 실제 실행 및 기본 입력 검증](./Version_0.3.md)
- [Version 0.4 — Visual Studio 생성물 추적 정리](./Version_0.4.md)
- [Version 0.5 — 0 길이 드래그 발사 취소](./Version_0.5.md)
- [Version 0.6 — 공 상태 완전 초기화](./Version_0.6.md)
- [Version 0.7 — GDI 객체 수명 안정화](./Version_0.7.md)
- [Version 0.8 — 단일 고정 시간 간격 게임 루프](./Version_0.8.md)
- [Version 0.9 — 포커스와 마우스 캡처 안전 처리](./Version_0.9.md)
- [Version 0.10 — 화면 내부 12×4 페그 배치](./Version_0.10.md)
- [Version 0.11 — 검증된 Windows SDK 고정](./Version_0.11.md)
- [Version 0.12 — 충돌하는 콘솔 엔트리 제거](./Version_0.12.md)
- [Version 0.13 — 미사용 프로토타입 상태 제거](./Version_0.13.md)
- [Version 0.14 — `/W4` 경고 0개 기준선](./Version_0.14.md)
- [Version 0.15 — Sprint 0 실행·안정성 검증 완료](./Version_0.15.md)
- [Version 1.1 — 명시적 게임 상태 전이](./Version_1.1.md)
- [Version 1.2 — 게임 모델과 뷰 책임 분리](./Version_1.2.md)
- [Version 1.3 — 2D 벡터 값 타입 도입](./Version_1.3.md)
- [Version 1.4 — 반발계수 기반 법선 반사](./Version_1.4.md)
- [Version 1.5 — 핵심 물리·턴 자동화 테스트](./Version_1.5.md)
- [Version 1.6 — 통합 게임 레이아웃 설정](./Version_1.6.md)
- [Version 1.7 — 피해·턴 전환 피드백](./Version_1.7.md)
- [Version 1.8 — 재현 가능한 데이터 기반 페그 배치](./Version_1.8.md)
- [Version 1.9 — Sprint 1 최종 검증 완료](./Version_1.9.md)
- [Version 1.10 — 게임 종료 메시지 단일 표시](./Version_1.10.md)
- [Version 2.1 — 점수와 콤보 규칙](./Version_2.1.md)
- [Version 2.2 — 데이터 기반 페그 종류와 효과](./Version_2.2.md)
- [Version 2.3 — 검증된 스테이지 정의와 로딩](./Version_2.3.md)
- [Version 2.4 — Sprint 2 규칙 회귀 테스트 확장](./Version_2.4.md)
- [Version 2.5 — Sprint 2 최종 검증 완료](./Version_2.5.md)
- [Version 3.1 — 애니메이션과 음소거 가능한 사운드 피드백](./Version_3.1.md)
- [Version 3.2 — 조준 예상선과 발사 세기 시각화](./Version_3.2.md)
- [Version 3.3 — 스테이지 선택과 결과 화면](./Version_3.3.md)
- [Version 3.4 — 난이도와 접근성 옵션](./Version_3.4.md)
- [Version 3.5 — Sprint 3 최종 검증 완료](./Version_3.5.md)
- [Version 4.1 — 사용자 설정 저장·불러오기](./Version_4.1.md)
- [Version 4.2 — 스테이지별 최고 기록](./Version_4.2.md)
- [Version 4.3 — Release x64 배포 패키징·사전 검사](./Version_4.3.md)
- [Version 4.4 — 저장·배포 자동화 테스트 확장](./Version_4.4.md)
- [Version 4.5 — Sprint 4 최종 배포 검증 완료](./Version_4.5.md)
- [Version 5.1 — 오브·유물 진행 기반](./Version_5.1.md)
- [Version 5.2 — 예고 가능한 보스 행동 패턴](./Version_5.2.md)
- [Version 5.3 — 검증된 외부 콘텐츠 카탈로그](./Version_5.3.md)
- [Version 5.4 — ImageGen 기반 공통 UI와 장비 화면](./Version_5.4.md)
- [Version 5.5 — Sprint 5 결합 회귀·배포 검증 완료](./Version_5.5.md)
- [Version 6.1 — ImageGen 게임플레이 아트와 공 이동 피드백](./Version_6.1.md)
- [Version 6.2 — 멀티 몬스터 전투와 적 종류 확장](./Version_6.2.md)
- [Version 6.3 — 1.5cm 조준 가이드와 페그 반사 예상](./Version_6.3.md)
- [Version 6.4 — 새 페그 필드 UI와 개별 체력 막대](./Version_6.4.md)
- [Version 6.5 — 마우스 클릭 UI 내비게이션](./Version_6.5.md)
- [Version 6.6 — 순차 스테이지 런과 클리어 보상](./Version_6.6.md)
- [Version 6.7 — 보유 오브 덱·리필과 다음 오브 미리보기](./Version_6.7.md)
- [Version 6.8 — 인게임 PEG FIELD 오버레이 제거](./Version_6.8.md)
- [Version 6.9 — 공격 타입·대상 범위와 전투 애니메이션](./Version_6.9.md)
- [Version 6.10 — 상단 게임 옵션 인게임 정보 토글](./Version_6.10.md)
- [Version 6.11 — 부드러운 페그 충돌 효과음](./Version_6.11.md)
- [Version 6.12 — 외부 오브·유물·적 효과 카탈로그](./Version_6.12.md)
- [Version 6.13 — UTF-8 지역화와 이미지 자산 파이프라인](./Version_6.13.md)
- [Version 6.14 — Sprint 6 런·GDI·배포 최종 검증](./Version_6.14.md)
- [Version 7.1 — 분기형 스테이지 맵과 보스 경로](./Version_7.1.md)
- [Version 7.2 — 신규 몬스터 시각 타입 확장](./Version_7.2.md)
- [Version 7.3 — Refresh Peg 재생성 보장](./Version_7.3.md)
- [Version 7.4 — 축소형 PEG FIELD와 사이드 오브 큐](./Version_7.4.md)
- [Version 7.5 — 공격 종료 시 Refresh Peg 최소 보장](./Version_7.5.md)
- [Version 7.6 — 간결한 스테이지 선택과 연출 완료 후 전환](./Version_7.6.md)
- [Version 7.7 — 오브·유물 이미지와 정확한 보상 효과 안내](./Version_7.7.md)
- [Version 7.8 — 지속 이동 페그 스테이지 패턴](./Version_7.8.md)
- [Version 7.9 — 시작 전 스테이지 재선택과 명시적 경로 확정](./Version_7.9.md)
- [Version 7.10 — 골드 기반 Goblin Market 상점 스테이지](./Version_7.10.md)
- [Version 7.11 — 몬스터 칸 거리와 개별 공격 사거리](./Version_7.11.md)
- [Version 7.12 — 매 턴 Refresh Peg 위치 변경과 파괴 후 재배치](./Version_7.12.md)
- [Version 7.13 — 일러스트 기반 스테이지 선택 카드와 런 경로 UI](./Version_7.13.md)
- [Version 7.14 — Steam Peglin 참고 UI 계층과 전체 화면 배치 최적화](./Version_7.14.md)
- [Version 7.15 — 파일 기반 오디오 카탈로그와 개별 볼륨](./Version_7.15.md)
- [Version 7.16 — 화면 전환·보상·피해 UI 애니메이션](./Version_7.16.md)
- [Version 7.17 — 게임패드 전체 런 입력 동등성](./Version_7.17.md)
- [Version 7.18 — 스테이지·난이도·오브 상세 통계](./Version_7.18.md)
- [Version 7.19 — 세이브 백업·손상 복구·선택형 초기화](./Version_7.19.md)
- [Version 7.20 — Sprint 7 최종 회귀·배포 검증](./Version_7.20.md)
- [Version 8.1 — DPI·해상도·창 크기 대응 레이아웃](./Version_8.1.md)
- [Version 8.2 — 상세 툴팁과 전투 로그](./Version_8.2.md)
- [Version 8.3 — 콘텐츠 검증 리포트와 미리보기 도구](./Version_8.3.md)
- [Version 8.4 — 게임패드 조준 설정과 장치 상태 QA](./Version_8.4.md)
- [Content Report 8.3 — 현재 외부 콘텐츠 검증·미리보기](./Content_Report_8.3.md)
- [Next Version Plan 0 — Sprint 0 완료 기록](./Next_Version_Plan_0.md)
- [Next Version Plan 1 — Sprint 1 계획](./Next_Version_Plan_1.md)
- [Next Version Plan 2 — Sprint 2 완료 기록](./Next_Version_Plan_2.md)
- [Next Version Plan 3 — Sprint 3 완료 기록](./Next_Version_Plan_3.md)
- [Next Version Plan 4 — Sprint 4 계획](./Next_Version_Plan_4.md)
- [Next Version Plan 5 — Sprint 5 계획](./Next_Version_Plan_5.md)
- [Next Version Plan 6 — Sprint 6 계획](./Next_Version_Plan_6.md)
- [Next Version Plan 7 — Sprint 7 완료 기록](./Next_Version_Plan_7.md)
- [Next Version Plan 8 — Sprint 8 계획](./Next_Version_Plan_8.md)
- [빌드 환경 및 명령](../BUILDING.md)

## 스프린트 전환 절차

1. `Next_Version_Plan_[현재 스프린트].md`의 필수 체크리스트를 모두 완료한다.
2. 빌드, 실행, 기능 및 회귀 검증 결과를 현재 스프린트의 새 `Version` 문서에 남긴다.
3. 미완료 항목이 없음을 확인한다.
4. 다음 스프린트의 `Next_Version_Plan`을 만든다.
5. 다음 스프린트의 `Version [다음 스프린트].1`부터 개선 작업을 시작한다.
