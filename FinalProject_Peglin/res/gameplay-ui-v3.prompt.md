# Gameplay UI v3 — ImageGen 기록

- 생성 방식: Codex 내장 ImageGen
- 생성일: 2026-08-30
- 구도 참고: 사용자가 제공한 Peglin풍 전투 UI 스크린샷
- 색감·화풍 참고: `gameplay-cave-v2-source.png`
- 원본: `gameplay-cave-v3-source.png` (1402×1122 PNG)
- 런타임: `gameplay-cave-v3.bmp` (1000×800, 24-bit BMP)
- 런타임 변환: 기존 12×4 페그 배열과 보드 프레임을 맞추기 위해 좌우를 약 5.8%씩 중앙 크롭한 뒤 1000×800으로 고품질 축소했다.

## 최종 프롬프트

> Create a new production-ready 2D gameplay BACKGROUND ONLY for an original dark-fantasy peg-bouncing RPG, landscape 5:4 composition intended for a 1000x800 Windows game client.
>
> Reference roles:
> - Use the first reference image only for broad UI composition and hierarchy: a character-and-monster encounter area across the top and one large, clearly framed rectangular peg board below. Do not copy its characters, monsters, text, icons, exact ornaments, or layout details.
> - Use the second reference image for palette and painterly continuity: deep navy cave stone, twisted roots, restrained amber crystals, premium hand-painted indie game finish.
>
> Required layout:
> - Outer cave frame made of dark carved stone and roots, with restrained warm amber crystal lamps near the side edges.
> - Upper encounter zone from approximately y=70 to y=275: one continuous mossy stone ledge/platform that supports a hero on the left and a group of up to three monsters on the right. Leave the middle open and visually calm for HUD overlays.
> - Lower playfield from approximately x=25 to x=975 and y=300 to y=685: one large recessed RECTANGULAR peg-board chamber, enclosed by a clearly readable carved-stone border on all four sides. The interior must be spacious, flat, dark blue-black stone with subtle texture and low contrast so code-drawn pegs, an orb, trajectory lines, and labels remain readable.
> - Keep the top-center of the lower playfield open and unadorned because the orb will begin INSIDE the rectangular peg field near x=500, y=330.
> - The bottom boundary must be visible above y=700 and leave a modest dark strip below it for runtime status text.
> - Lighting should guide the eye from the upper encounter ledge into the rectangular playfield, with cool cave depth and small amber accents.
>
> Critical exclusions:
> - Absolutely NO circular portal, circular doorway, ring, launch hole, cannon, pedestal, target, round socket, or circular focal structure anywhere near the center or orb spawn.
> - No character, monster, creature, ball, orb, peg, projectile, trajectory, health bar, progress bar, UI panel, button, icon, text, letter, number, logo, signature, or watermark.
> - Do not create separate side inventory panels; maximize the central rectangular peg field.
> - Do not make the playfield border so ornate that it competes with gameplay.
>
> Style: original premium hand-painted 2D dark fantasy game art, readable silhouettes, crisp native-resolution finish, painterly rather than photorealistic, cohesive navy/charcoal/amber palette, no blur.
