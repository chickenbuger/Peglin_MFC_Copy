# Peglin MFC 빌드 환경

이 문서는 Sprint 0에서 확인한 Windows 개발 환경과 재현용 빌드 명령을 정리한다.

## 필수 구성요소

- Windows 10 또는 Windows 11 x64
- Visual Studio 2022 Community 17.14 계열
- `MSVC v143` C++ 빌드 도구
- `C++ v14.44 (17.14) MFC for v143 build tools (x86 & x64)`
  - 구성요소 ID: `Microsoft.VisualStudio.Component.VC.14.44.17.14.MFC`
- Windows 10/11 SDK
  - 현재 PC 설치 버전: `10.0.19041.0`, `10.0.22621.0`, `10.0.26100.0`
  - 프로젝트 요구 버전: `10.0.26100.0`
  - 설치 확인 경로: `C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0`
  - 프로젝트가 이 정확한 버전을 사용하므로 새 개발 PC에도 같은 SDK를 설치해야 한다.
- C++ 언어 표준: C++20 (`/std:c++20`), 모든 구성에 동일 적용
- 소스 문자 집합: UTF-8 (`/utf-8`), 한국어·영어 리터럴과 외부 문자열 카탈로그 일치
- 컴파일러 경고 수준: `/W4`, 모든 구성에 동일 적용

Visual Studio Installer의 **개별 구성 요소** 탭에서 위 MFC 구성요소를 검색해 설치한다. 현재 확인된 MFC 라이브러리 위치는 다음과 같다.

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\atlmfc\lib\x64
```

## 명령줄 빌드

저장소 루트의 PowerShell에서 실행한다.

```powershell
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

& $msbuild '.\FinalProject_Peglin.sln' /m /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /v:minimal
& $msbuild '.\FinalProject_Peglin.sln' /m /t:Rebuild /p:Configuration=Release /p:Platform=x64 /v:minimal
& $msbuild '.\FinalProject_Peglin.sln' /m /t:Rebuild /p:Configuration=Debug /p:Platform=x86 /v:minimal
& $msbuild '.\FinalProject_Peglin.sln' /m /t:Rebuild /p:Configuration=Release /p:Platform=x86 /v:minimal
```

솔루션의 32비트 플랫폼 이름은 `x86`이며, 프로젝트 내부에서는 `Win32` 구성으로 매핑된다.

## 출력 위치

| 구성 | 실행 파일 |
| --- | --- |
| Debug x64 | `x64/Debug/FinalProject_Peglin.exe` |
| Release x64 | `x64/Release/FinalProject_Peglin.exe` |
| Debug x86 | `Debug/FinalProject_Peglin.exe` |
| Release x86 | `Release/FinalProject_Peglin.exe` |

## MFC 런타임 및 배포 정책

프로젝트는 `UseOfMfc=Dynamic`을 유지한다. Version 4.3부터 Release x64 배포 패키지는 Visual Studio의 정식 `VC/Redist/MSVC` 폴더에서 다음 v143 x64 DLL을 앱 로컬 방식으로 포함한다.

- `mfc140u.dll`
- `msvcp140.dll`
- `vcruntime140.dll`
- `vcruntime140_1.dll`

Universal CRT와 Windows 시스템 DLL은 지원 대상 Windows 10/11이 제공한다. 배경·캐릭터·아이콘·툴바 비트맵은 EXE 리소스에 포함되므로 외부 이미지 파일은 필요하지 않다.

저장소 루트에서 다음 명령으로 Release x64를 빌드하고 `dist/PeglinMFC-[버전]-win-x64` 폴더와 ZIP을 생성한다.

```powershell
& '.\tools\Package-Release.ps1' -Version '7.11'
```

패키징 스크립트는 Windows PowerShell 5와 PowerShell 7에서 실행할 수 있다. Visual Studio 설치 위치와 최신 v143 Redist를 자동 탐색하고, 이미지 변환·크기·색심도와 외부 스테이지·게임플레이·한국어·영어 카탈로그를 먼저 검증한다. 결과물에 `Preflight.ps1`, `SHA256SUMS.txt`, `README.txt`를 포함하며, `Preflight.ps1`은 Windows x64, 필수 파일, PE 아키텍처와 SHA-256 무결성을 검사한다. ZIP 생성 후에는 경로 탈출, 파일 변조, MFC 런타임·외부 카탈로그 누락을 자동 탐지한다.

## 핵심 자동화 테스트

솔루션 재빌드 시 `PeglinCoreTests`가 앱과 함께 생성된다. 각 실행 파일은 0 길이 발사, 벽·페그 반사, 피해·턴 정산과 승패 상태 전이를 검증하며 하나라도 실패하면 종료 코드 1을 반환한다.

```powershell
& '.\Debug\tests\x64\PeglinCoreTests.exe'
& '.\Release\tests\x64\PeglinCoreTests.exe'
& '.\Debug\tests\Win32\PeglinCoreTests.exe'
& '.\Release\tests\Win32\PeglinCoreTests.exe'
```

## 현재 검증 상태

2026-09-01 Version 7.11 기준 Windows SDK `10.0.26100.0`, `/W4`, `/utf-8`로 네 구성 모두 오류 0개, 자체 코드 경고 0개다. 구성별 939개·총 3,756개 자동 검증과 5종 몬스터, 9개 적 안정 ID, Refresh Peg 보장, 오브·유물 보상 안내, 이동 페그, 시작 전 경로 재선택, Goblin Market 상점, 몬스터별 칸 거리·공격 사거리를 검증했다. 자산 파이프라인은 17개 리소스와 16개 PNG→24-bit BMP 변환을 검증한다.

Version 7.4 Release x64 ZIP은 `dist/PeglinMFC-7.4-win-x64.zip`에 생성되며 크기는 7,054,453 bytes, SHA-256은 `62D7B57F5D7DD058785EE8717488EFA7FBE3636102C41F3F73247A878DB811FF`다.

Version 7.6 Release x64 ZIP은 `dist/PeglinMFC-7.6-win-x64.zip`에 생성되며 크기는 7,054,180 bytes, SHA-256은 `954A08EA35F4DE35DB77B67495FCB1E860132781A4ED01D7B6275E2043366854`다.

Version 7.7 Release x64 ZIP은 `dist/PeglinMFC-7.7-win-x64.zip`에 생성되며 크기는 7,055,120 bytes, SHA-256은 `8ED490955ED0E678C90B94FE382262F642F767EF9D7997E58594B96F12DB03E5`다.

Version 7.8 Release x64 ZIP은 `dist/PeglinMFC-7.8-win-x64.zip`에 생성되며 크기는 7,141,823 bytes, SHA-256은 `E31582C5998ED755C6F28B66F5EE2F6A58543DE0D74BFD6EDFA1EDB1AEE57670`다.

Version 7.9 Release x64 ZIP은 `dist/PeglinMFC-7.9-win-x64.zip`에 생성되며 크기는 7,142,646 bytes, SHA-256은 `FE4ECA51907F6D03BF498A59E79A71FF84B6DA9D2E26105651A03DB403C8A84D`다.

Version 7.10 Release x64 ZIP은 `dist/PeglinMFC-7.10-win-x64.zip`에 생성되며 크기는 7,263,894 bytes, SHA-256은 `5613357A126FA72ED18EBF404783A871BC2A0B3052F252BD111503358E7D8D80`다.

Version 7.11 Release x64 ZIP은 `dist/PeglinMFC-7.11-win-x64.zip`에 생성되며 크기는 7,265,630 bytes, SHA-256은 `E6DC32DD7F28CF49B6B13577BF24F8CCB9AC796CC4B7151337B9DBEEF282BB14`다.
