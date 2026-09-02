Peglin MFC Copy — Windows x64 Release
=====================================

요구 환경
---------
- Windows 10 또는 Windows 11 x64
- 쓰기 가능한 사용자 LocalAppData 폴더

실행
----
FinalProject_Peglin.exe를 실행합니다.

콘텐츠 제작 미리보기
--------------------
FinalProject_Peglin.exe --content-preview 로 실행하면 전체 스테이지의 페그·이동
궤적·적 배치를 확인하고 R 키로 외부 콘텐츠를 안전하게 다시 읽을 수 있습니다.

사전 검사
---------
PowerShell에서 다음 명령으로 필수 파일, x64 PE와 SHA-256 무결성을 확인할 수 있습니다.

  powershell -ExecutionPolicy Bypass -File .\Preflight.ps1

이 패키지에 포함된 파일
------------------------
- FinalProject_Peglin.exe
- Visual C++ v143 앱 로컬 런타임: mfc140u.dll, msvcp140.dll,
  vcruntime140.dll, vcruntime140_1.dll
- Preflight.ps1, SHA256SUMS.txt, PACKAGE_VERSION.txt
- content\stages.v1.ini: 버전 1 스테이지·보스 행동 정의
- content\gameplay.v1.ini: 버전 1 오브·유물·적 효과 정의
- content\strings.*.v1.ini: 한국어·영어 UI 문자열
- content\audio.v1.ini, content\audio\*.wav: 효과음·화면별 배경음

자산 정책
---------
배경, 플레이어, 적, 아이콘과 툴바 비트맵은 Windows 리소스로 EXE에 포함됩니다.
스테이지와 게임플레이 규칙은 버전 카탈로그에서 읽으며 누락·손상 시 검증된 내장
기본값으로 안전하게 실행합니다. 오디오 카탈로그나 WAV가 손상되면 무음으로 실행합니다.
외부 이미지 파일은 필요하지 않습니다.

사용자 데이터
-------------
- 설정: %LOCALAPPDATA%\PeglinMFC\settings.v1.ini
- 기록: %LOCALAPPDATA%\PeglinMFC\records.v1.ini
- 진행 중인 런: %LOCALAPPDATA%\PeglinMFC\run.v1.ini
패키지를 제거해도 사용자 데이터는 자동 삭제하지 않습니다.

런타임 정책
-----------
Visual Studio 설치 없이 실행할 수 있도록 Visual Studio의 재배포 가능 디렉터리에서
필요한 x64 MFC/CRT DLL을 앱 로컬 방식으로 포함합니다. Universal CRT와 Windows 시스템
DLL은 지원 대상 Windows 10/11 운영체제가 제공합니다.
