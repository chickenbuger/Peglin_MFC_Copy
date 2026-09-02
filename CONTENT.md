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

`stages.v1.ini`는 일반 스테이지를 먼저, 보스 스테이지를 마지막에 둔다. Version 9.6 기본 카탈로그는 일반 9개와 보스 1개, 총 10개다.

- 첫 일반 스테이지는 고정 시작 노드다.
- 남은 일반 스테이지는 파일 순서대로 최대 두 개씩 묶여 다음 경로 선택 카드가 된다.
- 보스는 정확히 하나여야 하며 마지막 단일 노드가 된다.
- 현재 기본 런은 `시작 1회 + 선택 전투 4회 + 보스 1회`, 총 6회의 전투와 중간 상점 1회로 구성된다.
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

기본 카탈로그에서는 Dense Cavern, Fungal Hollow, Ember Roost, Obsidian Burrow, Rootspire Crossing과 Rootbound Citadel 보스전이 서로 다른 수평·수직 이동 패턴을 사용한다.

## Refresh Peg 재배치 규칙

- 공격 턴이 끝날 때 활성 Refresh Peg의 속성을 다른 활성 페그와 교환해 위치를 변경한다.
- Refresh Peg가 직접 충돌로 발동하면 제거된 보드를 복원한 직후 이전과 다른 원본 페그 인덱스로 Refresh 속성을 옮긴다.
- 폭발 범위로 Refresh Peg가 제거된 경우에도 공격 종료를 기다리지 않고 같은 충돌 처리에서 다른 위치에 다시 만든다.
- 스테이지가 원래 가진 Refresh Peg 개수는 유지하며, Refresh가 없는 사용자 정의 배치는 최소 한 개를 보장한다.
- 위치 선택은 스테이지 ID·완료 턴·재배치 순서를 이용한 결정적 순서이므로 같은 입력을 재현할 수 있다.
- Refresh 타입만 교환하므로 페그 좌표, 이동 궤도, 원본 인덱스와 전체 활성 페그 수는 바뀌지 않는다.
- 활성 후보가 하나뿐이라 다른 위치가 물리적으로 존재하지 않는 사용자 정의 배치에서는 현재 위치를 유지한다.

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

스테이지의 `enemy` 세 번째 값은 시각 타입이며 Version 9.6에서 다음 일곱 값을 지원한다.

- `CrystalToad`
- `EmberBat`
- `MossShaman`
- `ThornbackWolf`
- `AzureWisp`
- `CinderBeetle`
- `RootLancer`

## 검증과 복구

로더는 적용 전에 다음을 모두 확인한다.

1. 파일 크기와 UTF-8 유효성
2. 버전, 섹션, 필수 키와 값 범위
3. 종류별 ID 중복과 시작 덱 필수 ID
4. 모든 효과·적 교차 참조
5. 효과 그래프의 순환과 합성 결과
6. 적용 후 스테이지 정의의 전체 유효성

검증에 실패하면 오류 원인과 줄 번호를 보존하고 내장 오브·유물·스테이지 규칙으로 안전하게 복구한다.

## 제작용 검증 리포트

Release x64 빌드 후 다음 명령을 실행하면 게임과 동일한 C++ 로더로 두 INI를 검증하고 `artifacts/content-report.md`를 만든다.

```powershell
& '.\tools\Write-ContentReport.ps1' -Configuration Release -Platform x64
```

리포트는 전체 콘텐츠 수, 스테이지별 페그·이동·Refresh·적·체력·공격력과 난이도 점수, 적 안정 ID·피해 배율·사거리, 오브·유물의 최종 합성 효과를 제공한다. 난이도 점수는 인접 스테이지 급등·급락, 같은 층 분기 편차, 층 평균 역행과 보스 강도를 비교하는 제작 지표이며 실제 턴 피해 예측값은 아니다. 출력은 저장소 내부 경로만 허용한다.

## 인게임 미리보기와 핫 리로드

Version 9.7부터 스테이지 선택 화면에서 `F8`을 누르면 콘텐츠 제작용 미리보기를 연다. 실행할 때 `--content-preview`를 전달하면 미리보기를 즉시 열고 외부 콘텐츠를 한 번 다시 읽는다.

```powershell
& '.\x64\Debug\FinalProject_Peglin.exe' --content-preview
```

- `←`/`→`, 화면의 Previous/Next 버튼 또는 게임패드 방향 입력으로 10개 스테이지를 순환한다.
- `R`, Hot Reload 버튼 또는 게임패드 `A`로 외부 스테이지·게임플레이 카탈로그를 다시 읽는다.
- `F8`/`Esc`, Close 버튼 또는 게임패드 `B`로 미리보기를 닫는다.
- 미리보기는 실제 배경, 적 이미지, 체력·사거리, 전체 페그, Refresh·Bomb 페그와 이동 페그 궤적을 사용한다.
- 로드한 콘텐츠가 형식·교차 참조·난이도 곡선을 모두 통과한 뒤에만 현재 카탈로그와 교환한다.
- 진행 중인 런의 모든 전투 스테이지와 현재 보유 오브·유물의 안정 ID가 새 카탈로그에도 있어야 한다. 하나라도 빠지면 전체 변경을 거부하고 기존 상태를 유지한다.
- 핫 리로드 후 새 경로 구조는 다음 새 게임부터 적용하며, 진행 중인 런의 체크포인트와 선택 경로는 바꾸지 않는다.

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
