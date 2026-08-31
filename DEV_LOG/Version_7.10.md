# Version 7.10 — 골드 기반 Goblin Market 상점 스테이지

## 버전 정보

| 항목 | 값 |
| --- | --- |
| 스프린트 | Sprint 7 |
| 버전 | Version 7.10 |
| 작업일 | 2026-09-01 |
| 상태 | 완료 |
| Sprint 7 진행률 | P0 10/16 완료 |

## 개선 이유

분기형 런은 전투와 보상 선택만 반복되어 전투 사이에 자원을 소비하는 판단이 없었다. 오브와 유물을 획득해도 즉시 선택 보상에만 의존하므로, 골드를 모아 다음 전투를 준비하거나 아끼는 Peglin 스타일의 런 운영이 부족했다. 전투가 아닌 독립 상점 노드와 가격·구매 상태가 명확한 화면을 추가해 경로 선택과 장비 구성에 경제 판단을 연결했다.

## 구현 내용

- 세 번째 일반 전투 뒤에 고정 `Goblin Market` 상점 노드를 삽입해 전체 경로를 일반 전투 3회 → 상점 → 일반 전투 → 보스로 구성했다.
- 새 런은 50G로 시작하고 일반 전투를 이길 때마다 25G를 획득한다. 상점 방문은 전투 클리어 횟수와 전투 보상 순서에 영향을 주지 않는다.
- 상점 상품은 `Iron Orb` 45G, `Bark Guard` 70G, `Heal 30 HP` 30G이며 한 방문에서 상품별 한 번만 구매할 수 있다.
- 현재 골드, 가격, 구매 가능·완료·골드 부족 상태와 실제 오브·유물 효과를 카드에 표시한다.
- 오브 보유 한도, 중복 불가 유물, 최대 체력, 잔액 부족을 구매 전에 검사하고 실패한 구매에는 골드를 차감하지 않는다.
- 상점 이탈 시 별도 전투 보상을 만들지 않고 즉시 다음 분기 스테이지 선택 화면으로 이동한다.
- 마우스 카드 클릭과 숫자 키 `1`~`3`으로 구매하고, Enter·Esc·`B` 또는 하단 버튼으로 상점을 나갈 수 있다.
- 상점 전용 `Shop` 화면·UI 명령·클릭 영역을 추가하고, 경로 카드와 런 상태 패널에 `SHOP`과 현재 골드를 표시한다.

## ImageGen 자산

- 사용 모드: `stylized-concept`
- 생성 프롬프트: `Transparent 2D Goblin Market sprite for a dark cave fantasy game UI. A friendly goblin merchant stands behind a compact wooden counter and holds a softly glowing gold coin. Painterly hand-drawn adventure game style, readable silhouette, deep brown, moss green, amber, teal and gold palette, transparent background, no text, no logo, no watermark.`
- 원본: `FinalProject_Peglin/res/shop-merchant-v1-source.png`
- 게임 리소스: `FinalProject_Peglin/res/shop-merchant-v1.bmp` — 260×300, 24-bit, 마젠타 투명색
- 재현 기록: `FinalProject_Peglin/res/shop-merchant-v1.prompt.md`

## 검증

| 검증 | 결과 |
| --- | --- |
| Debug/Release × x64/x86 | 모두 재빌드 성공 · 오류 0개, 자체 코드 경고 0개 |
| 자동화 테스트 | 구성별 850개 · 총 3,400개 통과 |
| 런 경로 | 일반 전투 3회 뒤 상점, 이탈 뒤 다음 전투 분기, 이후 보스 순서 검증 |
| 골드 | 시작 50G, 전투 승리당 +25G, 유효·무효·잔액 부족 결제 검증 |
| 상점 구매 | 오브·유물·회복 적용, 상품별 1회 제한, 상점에서 전투 보상 생성 금지 검증 |
| 실제 실행 | 3회 전투 후 125G로 Goblin Market 입장, Iron Orb 45G와 Bark Guard 70G 구매, 잔액 10G와 회복 골드 부족 표시 확인 |
| 화면 전환 | 상점 이탈 후 노드 5/6 전투 분기와 구매한 Bark Guard 표시 확인 |
| 종료 | 실제 앱 종료 후 게임 프로세스가 남지 않음 |
| 자산 파이프라인 | 17개 리소스 검증 · 16개 PNG 변환 검사 통과 |
| Release 패키지 | 7,263,894 bytes · SHA-256 `5613357A126FA72ED18EBF404783A871BC2A0B3052F252BD111503358E7D8D80` |
| 패키지 검사 | 경로 안전성·무결성·변조·런타임·외부 카탈로그 누락 탐지 통과 |

## 다음 버전

Version 7.11에서는 파일 기반 효과음·배경음 카탈로그와 개별 볼륨을 구현한다.
