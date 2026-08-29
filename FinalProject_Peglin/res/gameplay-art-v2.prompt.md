# Gameplay Art v2 — ImageGen 기록

- 생성 방식: Codex 내장 ImageGen
- 생성일: 2026-08-30
- 스타일 기준 이미지: `ui-adventure-frame-v1.bmp`
- 런타임 변환: 배경은 1000×800 24-bit BMP, 스프라이트는 투명 PNG 원본을 보존한 뒤 순수 마젠타 색상 키의 24-bit BMP로 축소했다.

## 플레이 배경

결과 파일:

- 원본: `gameplay-cave-v2-source.png`
- 런타임: `gameplay-cave-v2.bmp`

프롬프트:

> Create a production-ready 2D gameplay background for a cute dark-fantasy peg-bouncing game, matching a hand-painted premium indie game UI. Exact landscape composition for a 1000x800 Windows game client. A deep navy underground cave with twisted tree roots and dark stone, restrained warm amber crystal lanterns, painterly texture, subtle depth and atmospheric fog. Reserve the upper 250 pixels as a character encounter header: a small mossy stone platform on the upper left for the player, a larger platform on the upper right for one monster, and an ornate glowing launch portal centered at x=500 around y=245. Reserve the lower playfield from y=260 to y=780 as a clean, mostly uniform dark blue-black stone chamber with very subtle radial lighting and faint rock texture, high contrast for colorful pegs and a glowing ball. Frame the left and right board boundaries around x=10 and x=990 with thin carved stone roots; no white or pure black empty blocks. Do not include any character, monster, ball, peg, projectile, UI text, labels, numbers, logos, watermark, or embedded controls. Keep all important decorative details away from the central lower playfield. Crisp game-art finish, readable at native resolution, no blur, no photographic realism.

## 플레이어

결과 파일:

- 원본: `player-hero-v2-source.png`
- 런타임: `player-hero-v2.bmp`

프롬프트:

> Create one production-ready 2D game character sprite on a genuinely transparent background with clean alpha edges. A charming small Peglin-like forest goblin hero, but an original design: bright leaf-green skin, large curious amber eyes, short pointed ears, a tiny dark-blue adventurer hood and mossy leather tunic, holding a small wooden sling at the ready. Three-quarter view facing right, heroic but cute, centered full body, strong compact silhouette, slightly oversized head and hands. Hand-painted dark-fantasy indie game style matching a navy cave with warm amber crystal lights. Warm rim light from the right, subtle cool shadow, rich color, no outline-only look. Must remain clearly readable when reduced to 72x72 pixels. No platform, no environment, no shadow rectangle, no text, no UI, no logo, no watermark, no border, exactly one character.

## 몬스터

결과 파일:

- 원본: `enemy-crystal-toad-v2-source.png`
- 런타임: `enemy-crystal-toad-v2.bmp`

프롬프트:

> Create one production-ready 2D enemy sprite on a genuinely transparent background with clean alpha edges. An original cute-but-dangerous cave monster for a peg-bouncing RPG: a squat midnight-blue crystal toad-slime guardian with a rounded body, two sturdy little feet, glowing amber eyes, small jagged stone horns, and several warm amber crystal shards growing from its back. Three-quarter view facing left toward the hero, alert battle pose, compact silhouette with a mischievous expression. Hand-painted dark-fantasy indie game style matching a navy underground cave with twisted roots and amber lanterns. Warm rim light from the left and cool cave shadow. Must remain clearly readable when reduced to 88x72 pixels. No platform, no environment, no health bar, no text, no UI, no logo, no watermark, no border, exactly one monster.

## 마법 공

결과 파일:

- 원본: `orb-amber-teal-v2-source.png`
- 런타임: `orb-amber-teal-v2.bmp`

프롬프트:

> Create one production-ready magical projectile orb sprite on a genuinely transparent background with clean alpha edges. Exactly one perfectly circular enchanted amber-and-teal glass orb for a cute dark-fantasy peg-bouncing game. Strong bright golden core, teal-blue outer glass, a tiny leaf-shaped rune inside, crisp circular rim, compact luminous halo that does not extend too far, subtle directional highlight at upper left, readable motion-ready silhouette. Hand-painted premium indie game style matching a navy cave with amber crystals. Designed to remain sharp and recognizable when reduced to 24x24 pixels. No trail, no extra particles beyond a very tight glow, no platform, no environment, no text, no UI, no logo, no watermark, no border.
