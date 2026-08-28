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
| 현재 스프린트 | Sprint 0 |
| 현재 버전 | Version 0.13 |
| 스프린트 목표 | 현재 프로젝트 분석 및 재현 가능한 빌드·실행 환경 확립 |
| 상태 | 진행 중 — 미사용 프로토타입 상태·API 제거 완료. 경고 기준선 강화 예정 |

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
- [Next Version Plan 0 — Sprint 0 개선 계획](./Next_Version_Plan_0.md)
- [빌드 환경 및 명령](../BUILDING.md)

## 스프린트 전환 절차

1. `Next_Version_Plan_[현재 스프린트].md`의 필수 체크리스트를 모두 완료한다.
2. 빌드, 실행, 기능 및 회귀 검증 결과를 현재 스프린트의 새 `Version` 문서에 남긴다.
3. 미완료 항목이 없음을 확인한다.
4. 다음 스프린트의 `Next_Version_Plan`을 만든다.
5. 다음 스프린트의 `Version [다음 스프린트].1`부터 개선 작업을 시작한다.
