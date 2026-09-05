# Peglin MFC Copy

MFC와 C++20으로 제작한 Peglin 스타일 게임 프로젝트다.

- 현재 버전: **Version 10.2 — 1차 마감**
- 기준 브랜치: **main**
- 완료 범위: **Sprint 0~9 전체와 Version 10.1 전투 UI·연출 개선**
- 마감 기록·검증 결과: [Version 10.2](./DEV_LOG/Version_10.2.md)
- 후속 과제: [Sprint 10 잔여 계획](./DEV_LOG/Next_Version_Plan_10.md)
- 빌드 방법: [BUILDING.md](./BUILDING.md)
- 외부 콘텐츠: [CONTENT.md](./CONTENT.md)
- 개발 기록: [DEV_LOG/README.md](./DEV_LOG/README.md)
- 포트폴리오: [개발 개요 HTML](./Peglin_Development_Overview.html) — Sprint 9 성과 기준

## 실행 및 배포

Windows 10/11 x64에서 `PeglinMFC-10.2-win-x64.zip`을 풀고 `FinalProject_Peglin.exe`를 실행한다. 앱 로컬 MFC/CRT 런타임과 외부 콘텐츠가 포함된다.

소스에서 배포 패키지를 만들려면 [빌드 환경](./BUILDING.md)을 준비한 뒤 저장소 루트에서 실행한다.

```powershell
& '.\tools\Package-Release.ps1' -Version '10.2'
```

결과물은 `dist/PeglinMFC-10.2-win-x64.zip`에 생성된다. 배포 ZIP과 빌드 생성물은 Git 추적에서 제외한다.

## 1차 마감 범위

분기형 런, 페그·오브·유물 전투, 상점과 보스, 런 저장·복구, 입력 재설정, 게임패드, 오디오, 콘텐츠 미리보기·핫 리로드와 전투 UI를 포함한다. Sprint 10 원계획의 P0 진행률은 1/7이며, 남은 기능과 그 기능에 대한 통합 검증은 후속 과제로 보류한다.
