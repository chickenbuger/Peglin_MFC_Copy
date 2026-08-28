# Version 0.11 — 검증된 Windows SDK 고정

## 버전 정보

| 항목 | 값 |
| --- | --- |
| 버전 | Version 0.11 |
| 스프린트 | Sprint 0 |
| 작업일 | 2026-08-29 (Asia/Seoul) |
| 목적 | 개발 PC마다 달라질 수 있는 Windows SDK 자동 선택 제거 |
| 판정 | **빌드 SDK 고정 완료** |

## 변경 사항

- 프로젝트 전역 `WindowsTargetPlatformVersion`을 `10.0.26100.0`으로 고정했다.
- Platform Toolset `v143` 및 MFC 동적 연결 정책과 함께 정확한 SDK 요구 사항을 `BUILDING.md`에 기록했다.
- SDK 설치 확인 경로와 새 개발 PC에서 같은 버전을 설치해야 한다는 조건을 명시했다.

## 검증

- `C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0` 경로와 `ucrt`, `um`, `shared`, `winrt` 헤더 구성을 확인했다.
- MSBuild 속성 평가 결과 `WindowsTargetPlatformVersion=10.0.26100.0`을 확인했다.
- 고정된 SDK로 Debug/Release × x64/x86 네 구성 재빌드가 모두 성공했다.
- Debug x64 실행 파일이 정상 게임 화면을 표시하고 오류 대화상자 없이 종료됐다.

## 결론

`10.0` 자동 선택으로 개발 환경마다 SDK가 달라지던 변수를 제거하고, 재현 가능한 Windows 빌드 기준을 확정했다.
