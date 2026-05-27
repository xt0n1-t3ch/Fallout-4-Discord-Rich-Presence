from pathlib import Path

HERE = Path(__file__).parent
PATH_DATA = (HERE / "_fallout_path.txt").read_text(encoding="utf-8").strip()

SVG = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1920 500" preserveAspectRatio="xMidYMid slice" role="img" aria-label="Discord Rich Presence for Fallout 4">
  <title>Discord Rich Presence for Fallout 4</title>
  <defs>
    <radialGradient id="bg" cx="50%" cy="55%" r="80%">
      <stop offset="0%" stop-color="#0a1812"/>
      <stop offset="100%" stop-color="#000503"/>
    </radialGradient>
    <linearGradient id="green" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#7CFFB0"/>
      <stop offset="100%" stop-color="#19E37D"/>
    </linearGradient>
    <pattern id="scanlines" width="3" height="3" patternUnits="userSpaceOnUse">
      <rect width="3" height="1" fill="#19E37D" opacity="0.06"/>
    </pattern>
    <filter id="glow" x="-30%" y="-30%" width="160%" height="160%">
      <feGaussianBlur stdDeviation="3.5" result="b"/>
      <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
    </filter>
    <radialGradient id="vignette" cx="50%" cy="50%" r="75%">
      <stop offset="55%" stop-color="#000" stop-opacity="0"/>
      <stop offset="100%" stop-color="#000" stop-opacity="0.7"/>
    </radialGradient>
  </defs>

  <rect width="1920" height="500" fill="url(#bg)"/>
  <rect width="1920" height="500" fill="url(#scanlines)"/>

  <rect width="1920" height="4" fill="#7CFFB0" opacity="0.35">
    <animate attributeName="y" from="-10" to="510" dur="6s" repeatCount="indefinite"/>
    <animate attributeName="opacity" values="0;0.4;0.4;0" keyTimes="0;0.2;0.8;1" dur="6s" repeatCount="indefinite"/>
  </rect>

  <g transform="translate(280 250)" filter="url(#glow)">
    <g>
      <animateTransform attributeName="transform" type="rotate" from="0" to="360" dur="42s" repeatCount="indefinite"/>
      <circle r="140" fill="none" stroke="#19E37D" stroke-width="4" opacity="0.85"/>
      <circle r="105" fill="none" stroke="#19E37D" stroke-width="3" stroke-dasharray="10 14" opacity="0.7"/>
    </g>
    <g stroke="url(#green)" stroke-width="16" stroke-linecap="round">
      <line x1="0" y1="-95" x2="0" y2="-30"/>
      <line x1="82" y1="48" x2="22" y2="14"/>
      <line x1="-82" y1="48" x2="-22" y2="14"/>
    </g>
    <circle r="16" fill="#7CFFB0">
      <animate attributeName="r" values="14;19;14" dur="2.6s" repeatCount="indefinite"/>
      <animate attributeName="opacity" values="0.85;1;0.85" dur="2.6s" repeatCount="indefinite"/>
    </circle>
  </g>

  <g transform="translate(540 140)" filter="url(#glow)">
    <g transform="scale(0.075)" fill="url(#green)">
      <path d="{PATH_DATA}"/>
    </g>
    <text x="660" y="200" font-family="Impact, 'Arial Black', sans-serif" font-size="290" font-weight="900" fill="url(#green)" letter-spacing="-8">4</text>
  </g>

  <g transform="translate(540 360)" font-family="'Consolas', 'Courier New', monospace">
    <text fill="#7CFFB0" font-size="42" font-weight="bold" filter="url(#glow)">DISCORD RICH PRESENCE</text>
    <text fill="#19E37D" opacity="0.78" font-size="20" y="40" letter-spacing="3">ONE F4SE DLL  ·  OLD-GEN + NEXT-GEN  ·  INI-CONFIGURABLE</text>
    <g y="84" transform="translate(0 84)">
      <text fill="#FFC927" font-size="22" font-weight="bold">&gt; SYSTEM READY</text>
      <text x="180" fill="#FFC927" font-size="26" font-weight="bold">
        <animate attributeName="opacity" values="1;1;0;0" keyTimes="0;0.5;0.5;1" dur="1.1s" repeatCount="indefinite"/>▮
      </text>
    </g>
  </g>

  <rect width="1920" height="500" fill="url(#vignette)" pointer-events="none"/>
</svg>
"""

out = HERE / "banner.svg"
out.write_text(SVG, encoding="utf-8")
print(f"banner.svg: {out.stat().st_size} bytes")
