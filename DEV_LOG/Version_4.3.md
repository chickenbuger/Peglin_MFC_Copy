# Version 4.3 — Release x64 배포 패키징·사전 검사

## 버전 정보

| 항목 | 값 |
| --- | --- |
| 버전 | Version 4.3 |
| 스프린트 | Sprint 4 |
| 작업일 | 2026-08-30 (Asia/Seoul) |
| 목적 | 개발 PC의 Visual Studio 설치에 의존하지 않는 재현 가능한 Windows x64 패키지 생성 |
| 판정 | **P0-3 완료** |

## 구현 내용

- `tools/Package-Release.ps1`이 Visual Studio와 MSBuild를 탐색해 Release x64를 빌드한다.
- Visual Studio `VC/Redist/MSVC`의 최신 정식 v143 x64 폴더에서 다음 앱 로컬 런타임을 복사한다.
  - `mfc140u.dll`
  - `msvcp140.dll`
  - `vcruntime140.dll`
  - `vcruntime140_1.dll`
- 배경·플레이어·적·아이콘·툴바 비트맵은 EXE의 Windows 리소스에 포함되어 별도 이미지 파일이 필요하지 않음을 문서화했다.
- `distribution/Preflight.ps1`이 Windows x64, 필수 파일, EXE의 x64 PE 머신 값과 SHA-256 무결성을 검사한다.
- 패키지에는 실행 안내, 사용자 데이터 경로, 런타임 정책, 버전과 `SHA256SUMS.txt`를 포함한다.
- 기존 패키지를 지울 때 출력 루트와 패키지 경로가 저장소 내부 `dist` 하위인지 먼저 검증한다.
- `dist/`를 Git ignore에 추가해 생성물과 소스 기록을 분리했다.

## 생성 결과

- 폴더: `dist/PeglinMFC-4.3-win-x64`
- ZIP: `dist/PeglinMFC-4.3-win-x64.zip`
- 압축 전 파일 크기 합계: 6,974,357 bytes
- ZIP 크기: 3,170,271 bytes
- ZIP SHA-256: `ACEF304390BB71D6697427F0DD5CC04BE0EFA85B4B8E1D63CCF74FB2D17ED449`
- 사용한 Redist: Visual Studio 2022 Community `14.44.35112` x64

## 검증

- 패키지 자체 사전 검사: `PREFLIGHT PASS`.
- 필수 파일 9개와 ZIP 내부 항목을 확인했다.
- 패키지의 `FinalProject_Peglin.exe`를 직접 실행해 선택 화면과 저장된 기록이 정상 표시됐다.
- 실행 중 모듈 경로를 확인한 결과 MFC/CRT 네 DLL이 모두 패키지 폴더에서 로드됐다.
- 패키지 앱을 정상 종료했고 잔류 프로세스가 없다.
- Debug/Release × x64/x86 빌드 오류 0개, 프로젝트 자체 경고 0개.
- 구성별 `342 PASS / 0 FAIL`, 총 1,368개 회귀 검증 통과.
- 패키지 생성 뒤 `dist/` 산출물이 Git 변경 목록에 나타나지 않았다.

## 다음 단계

Version 4.4에서 저장 파일 마이그레이션·추가 실패 경로와 ZIP 추출 패키지 검증을 자동화해 저장·배포 게이트를 완성한다.
