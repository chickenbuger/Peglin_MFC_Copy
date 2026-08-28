# Version 0.2 — MFC 개발 환경 구축

## 버전 정보

| 항목 | 값 |
| --- | --- |
| 버전 | Version 0.2 |
| 스프린트 | Sprint 0 |
| 작업일 | 2026-08-29 (Asia/Seoul) |
| 목적 | MFC 구성요소 설치, 구성별 빌드 설정 통일, 실행 가능한 기준선 확인 |
| 현재 판정 | **MFC 개발 환경 구축 완료 / Sprint 0 진행 중** |

## 결과 요약

Visual Studio Installer에 v143용 MFC x86/x64 구성요소를 추가했고 설치 등록, 라이브러리 파일, 재부팅 필요 여부를 확인했다. 프로젝트의 모든 Debug/Release 및 Win32/x64 구성에 C++20을 적용한 뒤 네 솔루션 구성을 모두 재빌드해 실행 파일 생성을 확인했다.

빌드된 Debug x64 실행 파일은 3초간 기동 상태를 유지했고 프로세스가 응답 중임을 확인한 뒤 종료했다. 따라서 Version 0.1의 `MSB8041` 차단 문제는 해결됐으며, 현재 PC에서 소스를 컴파일하고 실행할 수 있다.

Sprint 0 전체는 아직 끝나지 않았다. 형 변환과 엔트리 포인트 경고, 실행 안정성 수정, 저장소 정리, 전체 플레이 검증이 남아 있으므로 Sprint 1로 전환하지 않는다.

## 설치 확인

| 항목 | 확인 결과 |
| --- | --- |
| Visual Studio | Community 2022, 설치 버전 `17.14.37614.0` |
| MSBuild | `17.14.51` |
| Platform Toolset | `v143` |
| MFC 구성요소 | `Microsoft.VisualStudio.Component.VC.14.44.17.14.MFC` 등록 확인 |
| MFC 도구 버전 | `14.44.35207` |
| MFC x64 라이브러리 | `mfc140.lib`, `mfc140d.lib`, `mfc140u.lib`, `mfc140ud.lib` 확인 |
| 설치 후 재부팅 | 필요 없음 (`isRebootRequired=0`) |
| Windows SDK | `10.0.19041.0`, `10.0.22621.0`, `10.0.26100.0` 설치됨 |

## 프로젝트 설정 변경

`FinalProject_Peglin.vcxproj`에서 Debug Win32, Release Win32, Release x64에도 `<LanguageStandard>stdcpp20</LanguageStandard>`를 추가했다. 기존 Debug x64와 합쳐 네 구성 모두 `/std:c++20`을 사용한다.

이 변경이 필요한 이유는 `Parent_ball.cpp`의 공통 코드가 `std::clamp`를 사용하기 때문이다. 설정 전 Release x64에서는 `C2039`와 `C3861`로 빌드가 실패했으며, C++20 통일 후 같은 소스가 정상 컴파일됐다.

## 빌드 검증

| 솔루션 구성 | 프로젝트 매핑 | 결과 | 생성 파일 |
| --- | --- | --- | --- |
| Debug x64 | Debug x64 | 성공 | `x64/Debug/FinalProject_Peglin.exe` |
| Release x64 | Release x64 | 성공 | `x64/Release/FinalProject_Peglin.exe` |
| Debug x86 | Debug Win32 | 성공 | `Debug/FinalProject_Peglin.exe` |
| Release x86 | Release Win32 | 성공 | `Release/FinalProject_Peglin.exe` |

모든 빌드는 오류 0개로 완료됐다. 첫 병렬 재빌드에서는 구성 간 중간 산출물 사용이 겹쳐 Debug x64가 한 번 실패했으나, 단독 재빌드는 성공했다. 현재 프로젝트는 구성별 중간 출력 경로가 완전히 격리되지 않았을 가능성이 있으므로 서로 다른 구성을 동시에 재빌드하지 않는다.

## 실행 스모크 테스트

- 대상: `x64/Debug/FinalProject_Peglin.exe`
- 결과: 실행 후 3초 동안 프로세스가 종료되지 않았고 `Responding=True`를 확인했다.
- 정리: 확인 후 정상 종료 요청을 보내고, 남아 있는 경우 테스트 프로세스만 종료했다.
- 한계: 자동 확인은 프로세스 기동 단계까지 수행했다. 화면 요소, 입력, 전투, 재시작, 장시간 GDI 안정성은 이후 Sprint 0 수동·자동 스모크 테스트에서 검증해야 한다.

## 런타임 정책

프로젝트는 동적 MFC 연결(`UseOfMfc=Dynamic`)을 사용한다. 이 개발 PC에서는 설치된 Visual Studio MFC 런타임으로 실행한다. 다른 PC 배포 시 대상 아키텍처용 Visual C++ 재배포 가능 패키지를 설치하는 방식을 기본 정책으로 하며, 배포 패키징 확정 전에는 EXE 단독 배포를 지원 대상으로 보지 않는다.

## 남은 문제와 이유

| 우선순위 | 항목 | 남겨 둔 이유 |
| --- | --- | --- |
| P0 | `LNK4258` 엔트리 포인트 충돌 경고 | Debug 콘솔 pragma가 Unicode MFC 엔트리 포인트와 충돌하며 실제 설정이 무시된다. 실행 기준선을 명확히 해야 한다. |
| P0 | `C4244`, `C4305` 형 변환 경고 | 게임 좌표의 실수값이 GDI 정수 좌표로 암묵 변환되어 정밀도 손실 의도를 판단하기 어렵다. |
| P0 | Windows SDK 버전 미고정 | `10.0` 선택 방식은 PC마다 다른 SDK를 선택할 수 있어 완전한 재현성을 보장하지 못한다. |
| P0 | 전체 플레이·안정성 테스트 미수행 | 기동 성공만으로 입력, 충돌, 턴, 승패, GDI 자원 수명까지 정상이라고 판단할 수 없다. |
| P1 | 구성 병렬 빌드 격리 | 여러 구성을 동시에 재빌드할 때 중간 산출물 충돌 가능성이 관찰됐다. 출력 경로를 분리해야 자동화가 안전하다. |

## 변경 파일

- `FinalProject_Peglin/FinalProject_Peglin.vcxproj`
- `BUILDING.md`
- `DEV_LOG/README.md`
- `DEV_LOG/Version_0.2.md`
- `DEV_LOG/Next_Version_Plan_0.md`

## 결론

Version 0.2에서 MFC 설치와 구성별 컴파일 차단 문제를 해결해 **현재 PC의 빌드·기동 환경은 사용 가능한 상태**가 됐다. 다음 단계는 환경 문서의 SDK 요구 버전을 확정하고, 경고와 실행 안정성 항목을 처리해 Sprint 0의 전체 검증 게이트를 통과하는 것이다.
