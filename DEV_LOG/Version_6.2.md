# Version 6.2 — 멀티 몬스터 전투와 적 종류 확장

## 버전 정보

| 항목 | 값 |
| --- | --- |
| 스프린트 | Sprint 6 |
| 버전 | Version 6.2 |
| 작업일 | 2026-08-30 |
| 상태 | 완료 |
| Sprint 6 진행률 | P0 2/7 완료 |

## 개선 이유

Version 6.1까지 전투에는 Crystal Toad 한 마리만 존재했다. 스테이지마다 적의 수와 외형이 같아 전투 장면의 변화가 적었고, 콘텐츠 파일을 수정해도 여러 적으로 구성된 전투를 만들 수 없었다. 여러 종류의 적을 한 화면에 배치하고 순서대로 상대하도록 확장해 스테이지 구성의 다양성과 이후 적별 행동·속성 확장의 기반을 마련했다.

## 구현 결과

### 1. 최대 3마리의 전투 로스터

- `StageDefinition`에 안정적인 ID, 표시 이름, 외형, 체력을 가진 `EnemyDefinition`을 추가했다.
- 한 스테이지에 최대 3마리까지 구성하며 모든 적을 전투 상단에 동시에 표시한다.
- 현재 대상만 금색 타원으로 강조하고 HUD에 대상 이름·체력·남은 적 수를 표시한다.
- 현재 적이 쓰러지면 `EnemyDefeated` 이벤트를 한 번 발생시키고 다음 생존 적으로 전환한다.
- 마지막 적이 쓰러졌을 때만 Victory로 전환해 기존의 단일 결과 보장 규칙을 유지한다.

### 2. 세 종류의 몬스터

| 외형 키 | 표시 예 | 역할 |
| --- | --- | --- |
| `CrystalToad` | Crystal Toad | 기존 결정 동굴 몬스터 |
| `EmberBat` | Ember Bat / Ember Bat Scout | 빠른 비행 정찰 몬스터 |
| `MossShaman` | Moss Shaman / Moss Shaman Elder | 포자 마법을 사용하는 동굴 주술사 |

- ImageGen으로 Ember Bat과 Moss Shaman의 투명 PNG 원본을 생성했다.
- 마젠타 키 기반 24-bit BMP로 변환하고 MFC 리소스에 내장했다.
- 최종 프롬프트와 변환 규칙은 `res/enemy-roster-v1.prompt.md`에 보존했다.

### 3. 외부 콘텐츠 문법

`stages.v1.ini`의 스테이지 섹션에 `enemy` 키를 반복해 로스터를 구성한다.

```ini
enemy=crystal-toad,Crystal Toad,CrystalToad,8
enemy=ember-bat,Ember Bat,EmberBat,5
enemy=moss-shaman,Moss Shaman,MossShaman,7
```

- 순서: `안정 ID, 표시 이름, 외형 키, 체력`
- 중복 ID, 잘못된 문자, 빈 이름, 알 수 없는 외형, 0 이하 체력, 4번째 적을 거부한다.
- 난이도 체력 배율을 로스터의 모든 적에게 동일하게 적용한다.
- `enemy` 항목이 없는 이전 스테이지 정의는 기존 규칙 체력의 Crystal Toad 한 마리로 복구해 하위 호환성을 유지한다.

### 4. 기본 스테이지 구성

- Forgotten Forest: Crystal Toad, Ember Bat, Moss Shaman — 3마리 / 총 HP 20
- Dense Cavern: Ember Bat Scout, Moss Shaman Elder, Crystal Toad Guard — 3마리 / 총 HP 30
- Rootbound Citadel: Rootbound Titan — 보스 1마리 / 총 HP 60

## 검증 결과

| 검증 | 결과 |
| --- | --- |
| Debug x64 재빌드 | 성공 — 오류 0개, 자체 코드 경고 0개 |
| Release x64 재빌드 | 성공 — 오류 0개, 자체 코드 경고 0개 |
| Debug x86 재빌드 | 성공 — 오류 0개, 자체 코드 경고 0개 |
| Release x86 재빌드 | 성공 — 오류 0개, 자체 코드 경고 0개 |
| 핵심 자동화 테스트 | 구성별 552개, 총 2,208개 통과 |
| 로스터 순차 처치 | 3마리의 대상 전환, 남은 적 수, 최종 Victory 검증 |
| 데이터 오류 경로 | 중복 ID, 알 수 없는 외형, 최대 수 초과 거부 검증 |
| 실제 Debug x64 실행 | 메뉴 적 수·총 HP, 세 적 동시 표시, 대상 강조, HUD·투명 합성 확인 |
| 발사 실행 | 공 이동·잔상 중에도 세 적과 HUD가 안정적으로 표시됨 |

## Sprint 6 상태

- 완료: 게임플레이 이미지와 공 이동 피드백 — Version 6.1
- 완료: 몬스터 종류와 멀티 몬스터 전투 — Version 6.2
- 다음 P0: 런 진행 상태와 스테이지 클리어 보상 선택
- Sprint 6는 아직 진행 중이며 Sprint 7로 전환하지 않는다.
