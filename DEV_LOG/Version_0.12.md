# Version 0.12 — 충돌하는 콘솔 엔트리 제거

## 버전 정보

| 항목 | 값 |
| --- | --- |
| 버전 | Version 0.12 |
| 스프린트 | Sprint 0 |
| 작업일 | 2026-08-29 (Asia/Seoul) |
| 목적 | Unicode MFC 엔트리 포인트 충돌과 사용하지 않는 콘솔 출력 제거 |
| 판정 | **디버그 엔트리 정리 완료** |

## 변경 사항

- 모든 Debug 번역 단위에 전파되던 `/entry:WinMainCRTStartup /subsystem:console` linker pragma를 제거했다.
- 충돌·공 속도를 매 프레임 출력하던 `std::cout` 코드를 제거했다.
- 더 이상 필요 없는 `<iostream>`을 제거하고 `std::sqrt`가 필요한 파일에는 `<cmath>`를 직접 포함했다.
- Unicode MFC가 프로젝트의 Windows 서브시스템과 기본 `wWinMainCRTStartup` 진입점을 그대로 사용한다.

## 검증

- Debug/Release × x64/x86 네 구성 재빌드 성공.
- 네 구성 모두 `LNK4258`과 `WinMainCRTStartup` 관련 경고가 0개다.
- Debug x64 실행 시 `FinalProject_Peglin` 게임 창 하나만 생성되고 별도 콘솔 창이 생기지 않았다.
- 게임 화면 표시와 정상 종료를 확인했다.

## 결론

링커가 무시하던 충돌 설정과 출력 대상이 없는 콘솔 로그를 제거해 MFC 애플리케이션의 진입점·서브시스템 구성이 일관되게 됐다.
