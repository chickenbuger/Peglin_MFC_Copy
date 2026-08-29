# Enemy Roster v1 — ImageGen 기록

## 생성 정보

| 항목 | 값 |
| --- | --- |
| 생성일 | 2026-08-30 |
| 생성 방식 | Codex 내장 ImageGen |
| 용도 | Sprint 6 Version 6.2 멀티 몬스터 전투용 적 스프라이트 |
| 공통 참조 | `gameplay-cave-v1-source.png`, `enemy-crystal-toad-v1-source.png` |
| 런타임 처리 | 투명 PNG 원본을 마젠타 키 기반 24-bit BMP로 변환해 MFC 리소스에 포함 |

## Ember Bat

- 원본: `enemy-ember-bat-v1-source.png`
- 런타임: `enemy-ember-bat-v1.bmp` — 100×84, 24-bit

```text
Use case: stylized-concept. Asset type: production-ready 2D game enemy sprite. Create exactly one original ember cave bat scout on a genuinely transparent background with clean alpha edges. The creature has a compact dark indigo body, broad angular wings, oversized pointed ears, bright amber eyes, and small glowing orange crystal veins along the wing joints. Three-quarter view facing left toward the hero, wings lifted in an alert hovering pose, cute but dangerous expression, strong readable silhouette. Hand-painted premium indie dark-fantasy style matching a deep navy cave with twisted roots and warm amber crystals. Warm rim light from the left, cool blue shadow, rich texture without visual noise. Must remain clearly readable when reduced to about 88x72 pixels. No platform, no environment, no health bar, no text, no UI, no logo, no watermark, no border, exactly one monster.
```

## Moss Shaman

- 원본: `enemy-moss-shaman-v1-source.png`
- 런타임: `enemy-moss-shaman-v1.bmp` — 96×88, 24-bit

```text
Use case: stylized-concept. Asset type: production-ready 2D game enemy sprite. Create exactly one original moss mushroom shaman on a genuinely transparent background with clean alpha edges. A small squat cave creature with a wide layered mushroom cap, deep teal and moss-green body, glowing amber spore spots, two bright curious eyes, tiny root feet, and a crooked twig staff tipped with a warm crystal. Three-quarter view facing left toward the hero, casting-ready pose, magical and mischievous rather than frightening, strong compact silhouette. Hand-painted premium indie dark-fantasy style matching a deep navy cave with twisted roots and warm amber crystals. Warm rim light from the left, cool blue shadow, rich texture without visual noise. Must remain clearly readable when reduced to about 82x76 pixels. No platform, no environment, no health bar, no text, no UI, no logo, no watermark, no border, exactly one monster.
```

## 재현·검수 기준

- 기존 Crystal Toad와 같은 동굴 팔레트와 좌측 시선을 유지한다.
- 축소 시에도 눈, 실루엣, 주황색 결정 포인트가 구분돼야 한다.
- 배경·바닥·UI·문자·테두리·워터마크를 포함하지 않는다.
- 런타임 BMP의 마젠타 키가 실제 창에서 노출되지 않는지 확인한다.
