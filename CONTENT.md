# Peglin MFC 외부 콘텐츠 작성 가이드

실행 파일 옆 `content` 폴더에는 서로 독립적으로 검증되는 UTF-8 카탈로그 네 개가 있다.

| 파일 | 역할 |
| --- | --- |
| `stages.v1.ini` | 스테이지, 페그 배치, 적 로스터와 보스 행동 |
| `gameplay.v1.ini` | 재사용 효과, 오브, 유물과 적별 효과 연결 |
| `strings.ko-KR.v1.ini` | 한국어 UI 문자열 |
| `strings.en-US.v1.ini` | 영어 UI 문자열 |

각 파일이 없거나 손상되면 해당 카탈로그 전체를 폐기하고 검증된 내장 정의를 사용한다. 일부만 적용하지 않으므로 잘못된 교차 참조가 런타임 상태에 섞이지 않는다.

## 분기형 스테이지 런

`stages.v1.ini`는 일반 스테이지를 먼저, 보스 스테이지를 마지막에 둔다. Version 7.1 기본 카탈로그는 일반 7개와 보스 1개, 총 8개다.

- 첫 일반 스테이지는 고정 시작 노드다.
- 남은 일반 스테이지는 파일 순서대로 최대 두 개씩 묶여 다음 경로 선택 카드가 된다.
- 보스는 정확히 하나여야 하며 마지막 단일 노드가 된다.
- 현재 기본 런은 `시작 1회 + 선택 전투 3회 + 보스 1회`, 총 5회의 전투로 구성된다.
- 빈 ID, 중복 ID, 보스 누락·중복과 한 계층의 세 번째 선택지는 런 생성 단계에서 거부한다.
- 플레이어가 선택하지 않은 경로는 클리어 기록에 포함되지 않는다.

## 안정 ID

- ID는 영문 소문자, 숫자와 하이픈만 사용하며 최대 48자다.
- `basic-orb`, `iron-orb`, `echo-orb`는 시작 덱 호환성을 위한 필수 오브 ID다.
- `stages.v1.ini`의 `enemy` 첫 번째 값은 `gameplay.v1.ini`의 `[enemy]` ID와 일치해야 한다.
- 같은 종류 안의 중복 ID, 알 수 없는 효과 ID와 적 ID는 전체 외부 카탈로그 복구를 발생시킨다.

## 몬스터 거리와 공격 사거리

스테이지 로스터의 `enemy`는 다음 순서로 정의한다.

```ini
enemy=ember-bat,Ember Bat,EmberBat,10,3
```

값은 `안정 ID, 표시 이름, 시각 타입, 최대 체력, 공격 사거리(칸)`이다. 마지막 사거리를 생략한 기존 4개 값 형식은 1칸으로 읽어 호환하지만, 새 콘텐츠는 몬스터별 사거리를 명시해야 한다.

- 공격 사거리는 `1~12칸`이다. 기본 로스터는 두꺼비·늑대 1칸, 주술사 2칸, 박쥐 3칸, 위습 4칸을 사용한다.
- `enemy_steps`는 가장 앞 몬스터와 플레이어 사이의 시작 거리다. 두 번째·세 번째 몬스터는 각각 한 칸, 두 칸 뒤에서 시작한다.
- `enemy_step`은 한 칸 전진을 화면에 표시할 픽셀 폭이다. 기본 다중 몬스터 스테이지는 실루엣 겹침을 줄이기 위해 32px를 사용한다.
- 플레이어 공격이 정산되면 살아 있는 모든 몬스터가 독립적으로 행동한다. 사거리 밖이면 정확히 한 칸 전진하고, 이미 사거리 안이면 이동을 멈추고 공격한다.
- 이번 이동으로 사거리에 들어온 몬스터는 같은 턴에 추가 공격하지 않고 다음 플레이어 공격 정산부터 공격한다.
- 같은 턴에 여러 몬스터가 사거리 안이면 각각 공격하며 피해는 합산된다. 플레이어 공격으로 먼저 쓰러진 몬스터는 행동하지 않는다.
- `player_damage`는 일반 몬스터 한 마리의 공격 피해다. 보스의 `action=Strike,...`는 사거리 안에서 사용할 개별 피해를 정의한다.
- 보스 행동 패턴은 사거리 도달 뒤에만 진행되며 `Strike`와 `Fortify`를 사용한다. 사거리 안에서 다시 이동하는 `Advance` 패턴은 검증에서 거부한다.

## 이동 페그 정의

`stages.v1.ini`에서 레이아웃과 `peg_type`을 선언한 뒤 특정 페그에 왕복 이동을 추가할 수 있다.

```ini
peg_motion=12,Horizontal,22,1.20,0
peg_motion=20,Vertical,18,1.00,3.1415927
```

값은 순서대로 `페그 인덱스, 이동축, 진폭(px), 각속도(rad/s), 시작 위상(rad)`이다.

- 이동축은 `Horizontal` 또는 `Vertical`만 지원한다.
- 진폭은 `0 초과~64px`, 각속도는 `0 초과~2π rad/s` 범위여야 한다.
- 기준 위치와 전체 이동 경로가 PEG FIELD 안에 있어야 한다.
- 같은 페그 인덱스에 `peg_motion`을 두 번 선언하면 전체 외부 스테이지 카탈로그를 거부한다.
- 이동은 조준 중과 공 비행 중에 계속되고 일시정지 중에는 멈춘다. 스테이지 재시작 시 시작 위상으로 돌아간다.
- Refresh 복원은 현재 좌표가 아닌 원본 페그 인덱스를 사용하므로 이동 중인 페그가 중복 생성되지 않는다.

기본 카탈로그에서는 Dense Cavern, Fungal Hollow, Ember Roost와 Rootbound Citadel 보스전이 서로 다른 수평·수직 이동 패턴을 사용한다.

## 효과 정의

```ini
[effect]
id=enemy-rootbound-hide
kind=EnemyDamageTakenMultiplier
value=0.9
include=enemy-stone-hide
[/effect]
```

지원 종류:

- `PegDamageMultiplier`: 오브·유물의 페그 피해 배율
- `ScoreMultiplier`: 오브·유물의 점수 배율
- `IncomingDamageMultiplier`: 유물의 플레이어 피격 배율
- `EnemyDamageTakenMultiplier`: 해당 적이 받는 피해 배율

`include`는 다른 효과를 합성하며 여러 번 사용할 수 있다. 알 수 없는 참조, 같은 효과의 중복 참조, 직접·간접 순환 참조와 최종 범위 `0.1~5.0`을 벗어난 합성 결과는 거부한다.

## 오브 정의

```ini
[orb]
id=echo-orb
name=Echo Orb
icon=orb-echo-v1
delivery=Projectile
target=All
effect=orb-echo-damage
effect=orb-echo-score
[/orb]
```

- `delivery`: `Projectile` 또는 `Melee`
- `target`: `Single` 또는 `All`
- `icon`: `assets.v1.json`과 런타임 리소스에 연결되는 이미지 키. 키가 없으면 오브 ID를 폴백 키로 사용한다.
- 오브에는 피해·점수 계열 효과만 연결할 수 있다.

## 유물 정의

```ini
[relic]
id=bark-guard
name=Bark Guard
icon=relic-bark-guard-v1
duplicate=Stackable
max_stacks=2
effect=relic-bark-guard
[/relic]
```

- `duplicate`: `Unique` 또는 `Stackable`
- `max_stacks`: `1~8`
- `icon`: `assets.v1.json`과 런타임 리소스에 연결되는 이미지 키. 키가 없으면 유물 ID를 폴백 키로 사용한다.
- 유물에는 피해·점수·플레이어 피격 계열 효과를 연결할 수 있다.

## 적 효과 연결

```ini
[enemy]
id=rootbound-titan
effect=enemy-rootbound-hide
[/enemy]
```

적에는 `EnemyDamageTakenMultiplier` 계열만 연결할 수 있다. 스테이지 로스터의 안정 ID와 연결된 뒤 실제 턴 피해 정산에 적용된다.

스테이지의 `enemy` 세 번째 값은 시각 타입이며 Version 7.2에서 다음 다섯 값을 지원한다.

- `CrystalToad`
- `EmberBat`
- `MossShaman`
- `ThornbackWolf`
- `AzureWisp`

## 검증과 복구

로더는 적용 전에 다음을 모두 확인한다.

1. 파일 크기와 UTF-8 유효성
2. 버전, 섹션, 필수 키와 값 범위
3. 종류별 ID 중복과 시작 덱 필수 ID
4. 모든 효과·적 교차 참조
5. 효과 그래프의 순환과 합성 결과
6. 적용 후 스테이지 정의의 전체 유효성

검증에 실패하면 오류 원인과 줄 번호를 보존하고 내장 오브·유물·스테이지 규칙으로 안전하게 복구한다.

## UI 문자열과 지역화

- 문자열 파일은 `version=1`, `locale=ko-KR` 또는 `locale=en-US`로 시작한다.
- 키는 소문자, 숫자, `.`, `_`만 사용하며 코드의 내장 키와 일치해야 한다.
- 누락된 키는 선택 언어의 내장 문구로 합병하고, 중복·알 수 없는 키·잘못된 UTF-8·로케일 불일치는 외부 파일 전체를 폐기한다.
- 옵션 화면의 `L` 키 또는 마우스 클릭으로 한국어·영어를 즉시 전환하며 선택은 설정 파일에 저장된다.

## 이미지 자산 파이프라인

`FinalProject_Peglin/res/assets.v1.json`이 PNG 원본, 런타임 BMP, 리소스 ID, 크기와 투명 처리 규칙을 정의한다. 저장소 루트에서 다음을 실행한다.

```powershell
& '.\tools\Build-Assets.ps1' -VerifyConversion
```

파이프라인은 PNG 원본을 임시 24-bit BMP로 변환해 변환 가능성을 검증하고, 커밋된 BMP의 크기·색심도와 `UiResources.rc`의 ID·파일 연결을 대조한다. `-Rebuild`를 주면 원본 PNG에서 런타임 BMP를 재생성한다.
