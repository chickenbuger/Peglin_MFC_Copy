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
| 현재 스프린트 | Sprint 4 |
| 현재 버전 | Version 4.1 |
| 스프린트 목표 | 사용자 설정·플레이 기록 보존과 배포 가능한 실행 환경 확립 |
| 상태 | Sprint 4 P0 1/4 완료 — 스테이지별 기록 구현 예정 |

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
- [Next Version Plan 0 — Sprint 0 완료 기록](./Next_Version_Plan_0.md)
- [Next Version Plan 1 — Sprint 1 계획](./Next_Version_Plan_1.md)
- [Next Version Plan 2 — Sprint 2 완료 기록](./Next_Version_Plan_2.md)
- [Next Version Plan 3 — Sprint 3 완료 기록](./Next_Version_Plan_3.md)
- [Next Version Plan 4 — Sprint 4 계획](./Next_Version_Plan_4.md)
- [빌드 환경 및 명령](../BUILDING.md)

## 스프린트 전환 절차

1. `Next_Version_Plan_[현재 스프린트].md`의 필수 체크리스트를 모두 완료한다.
2. 빌드, 실행, 기능 및 회귀 검증 결과를 현재 스프린트의 새 `Version` 문서에 남긴다.
3. 미완료 항목이 없음을 확인한다.
4. 다음 스프린트의 `Next_Version_Plan`을 만든다.
5. 다음 스프린트의 `Version [다음 스프린트].1`부터 개선 작업을 시작한다.
