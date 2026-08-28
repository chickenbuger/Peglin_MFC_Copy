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
  - 프로젝트는 현재 `WindowsTargetPlatformVersion=10.0`으로 설치된 최신 SDK를 선택한다.
- C++ 언어 표준: C++20 (`/std:c++20`), 모든 구성에 동일 적용

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

## MFC 런타임 정책

프로젝트는 `UseOfMfc=Dynamic`을 사용한다. 개발 PC에서는 Visual Studio에 설치된 MFC 런타임으로 실행한다. 다른 PC에 배포할 때는 대상 아키텍처에 맞는 최신 Visual C++ 재배포 가능 패키지의 설치를 전제로 하며, Sprint 0에서 배포 패키징 방식을 확정하기 전까지 실행 파일만 단독 배포하지 않는다.

## 현재 검증 상태

2026-08-29 기준 네 구성 모두 오류 없이 실행 파일을 생성한다. 다만 `C4244`, `C4305`, `LNK4258` 경고가 남아 있으므로 Sprint 0의 경고 0개 빌드 게이트는 아직 통과하지 않았다.
