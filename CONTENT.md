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
delivery=Projectile
target=All
effect=orb-echo-damage
effect=orb-echo-score
[/orb]
```

- `delivery`: `Projectile` 또는 `Melee`
- `target`: `Single` 또는 `All`
- 오브에는 피해·점수 계열 효과만 연결할 수 있다.

## 유물 정의

```ini
[relic]
id=bark-guard
name=Bark Guard
duplicate=Stackable
max_stacks=2
effect=relic-bark-guard
[/relic]
```

- `duplicate`: `Unique` 또는 `Stackable`
- `max_stacks`: `1~8`
- 유물에는 피해·점수·플레이어 피격 계열 효과를 연결할 수 있다.

## 적 효과 연결

```ini
[enemy]
id=rootbound-titan
effect=enemy-rootbound-hide
[/enemy]
```

적에는 `EnemyDamageTakenMultiplier` 계열만 연결할 수 있다. 스테이지 로스터의 안정 ID와 연결된 뒤 실제 턴 피해 정산에 적용된다.

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
