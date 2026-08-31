# Peglin MFC 외부 콘텐츠 작성 가이드

실행 파일 옆 `content` 폴더에는 서로 독립적으로 검증되는 UTF-8 카탈로그 두 개가 있다.

| 파일 | 역할 |
| --- | --- |
| `stages.v1.ini` | 스테이지, 페그 배치, 적 로스터와 보스 행동 |
| `gameplay.v1.ini` | 재사용 효과, 오브, 유물과 적별 효과 연결 |

두 파일 중 하나가 없거나 손상되면 해당 카탈로그 전체를 폐기하고 검증된 내장 정의를 사용한다. 일부만 적용하지 않으므로 잘못된 교차 참조가 런타임 상태에 섞이지 않는다.

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
