"""Build Discord Rich Presence assets for the F4 Fallout 4 app.

Sources (official / public domain):
  - Steam CDN (appid 377160): library_hero, library capsule, header, logo
  - Wikimedia Commons: Fallout 4 logo (PD by threshold of originality)
  - Fallout wiki / Fandom (static.wikia.nocookie.net): Pip-Boy, Vault Boy

Outputs (Discord requires 1024x1024 square assets, 512x512 minimum, png/jpg):
  dist/discord-assets/
    fo4-big.png           1024x1024  large image (Power Armor cover)
    icon_combat.png       1024x1024  small image - in combat
    icon_explore.png      1024x1024  small image - exploring
    icon_menu.png         1024x1024  small image - generic menu
    icon_pipboy.png       1024x1024  small image - Pip-Boy open
    icon_mainmenu.png     1024x1024  small image - main menu
    icon_loading.png      1024x1024  small image - loading
    cover.png             1024x576   rich-presence invite cover (16:9)
"""

from __future__ import annotations

from pathlib import Path

import requests
from PIL import Image, ImageFilter, ImageEnhance

ROOT = Path(r"D:\X\2-Dev\f4drp-presence-overhaul\dist\discord-assets")
SRC = ROOT / "source"
ROOT.mkdir(parents=True, exist_ok=True)
SRC.mkdir(parents=True, exist_ok=True)

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/146.0.0.0 Safari/537.36"
    ),
    "Accept": "image/webp,image/png,image/jpeg,*/*;q=0.8",
}

SOURCES: dict[str, str] = {
    "steam_library_hero.jpg": "https://cdn.akamai.steamstatic.com/steam/apps/377160/library_hero.jpg",
    "steam_capsule_2x.jpg": "https://cdn.akamai.steamstatic.com/steam/apps/377160/library_600x900_2x.jpg",
    "steam_capsule.jpg": "https://cdn.akamai.steamstatic.com/steam/apps/377160/library_600x900.jpg",
    "steam_logo.png": "https://cdn.akamai.steamstatic.com/steam/apps/377160/logo.png",
    "steam_page_bg.jpg": "https://cdn.akamai.steamstatic.com/steam/apps/377160/page_bg_generated_v6b.jpg",
    "wm_fo4_logo.png": "https://upload.wikimedia.org/wikipedia/commons/6/68/Fallout_4_logo.png",
    "fandom_pipboy_3000.webp": "https://static.wikia.nocookie.net/fallout/images/3/39/Fo4_Pip-Boy_3000_Mark_IV.png/revision/latest",
    "fandom_pipboy_hires.webp": "https://static.wikia.nocookie.net/fallout/images/9/9c/Pipboy300markiv.png/revision/latest",
    "fandom_pipboy_loading.webp": "https://static.wikia.nocookie.net/fallout/images/f/fd/FO4_Pip-Boy_3000_Mark_IV_Loading_Screen.jpg/revision/latest",
    "fandom_vaultboy_fo3.webp": "https://static.wikia.nocookie.net/fallout/images/c/c0/VaultBoyFO3.png/revision/latest",
    "fandom_vaultboy_statue.webp": "https://static.wikia.nocookie.net/fallout/images/0/09/Fo4VW-Vault-boy-statue.png/revision/latest",
    "fandom_side_quest.webp": "https://static.wikia.nocookie.net/fallout/images/1/18/Icon_Fo4_side_quest.png/revision/latest",
    "fandom_vaultboy_fo3_clean.webp": "https://static.wikia.nocookie.net/fallout/images/1/1e/Fallout3e_transparent.png/revision/latest",
}


def fetch_all() -> None:
    print("=== FETCH ===")
    for name, url in SOURCES.items():
        path = SRC / name
        if path.exists() and path.stat().st_size > 1024:
            try:
                with Image.open(path) as test:
                    test.verify()
                print(f"  cached  {name:34s} {path.stat().st_size:>9d} B")
                continue
            except Exception:
                path.unlink(missing_ok=True)
        try:
            r = requests.get(url, headers=HEADERS, timeout=30)
            r.raise_for_status()
            path.write_bytes(r.content)
            print(f"  fetched {name:34s} {len(r.content):>9d} B")
        except Exception as exc:
            print(f"  FAIL    {name:34s} {exc}")


def open_any(path: Path) -> Image.Image:
    img = Image.open(path)
    img.load()
    return img.convert("RGBA")


def report_sources() -> None:
    print("\n=== SOURCE INVENTORY ===")
    for p in sorted(SRC.iterdir()):
        if p.is_dir() or p.suffix == ".txt":
            continue
        try:
            with Image.open(p) as img:
                print(
                    f"  {p.name:38s} {img.size[0]:5d}x{img.size[1]:<5d} {img.mode:5s} {img.format}"
                )
        except Exception as exc:
            print(f"  {p.name:38s} UNREADABLE: {exc}")


def fit_to_square(
    src: Image.Image,
    size: int = 1024,
    bg: tuple[int, int, int, int] = (10, 14, 8, 255),
    crop_box: tuple[float, float, float, float] | None = None,
    pad: float = 0.0,
    sharpen: bool = True,
) -> Image.Image:
    img = src.convert("RGBA")
    if crop_box is not None:
        l, t, r, b = crop_box
        w, h = img.size
        img = img.crop((int(l * w), int(t * h), int(r * w), int(b * h)))
    inset = int(size * pad)
    inner = size - 2 * inset
    img.thumbnail((inner, inner), Image.LANCZOS)
    if sharpen:
        img = img.filter(ImageFilter.UnsharpMask(radius=1.2, percent=80, threshold=2))
    canvas = Image.new("RGBA", (size, size), bg)
    x = (size - img.size[0]) // 2
    y = (size - img.size[1]) // 2
    if img.mode == "RGBA":
        canvas.paste(img, (x, y), img)
    else:
        canvas.paste(img, (x, y))
    return canvas


def crop_centered_square(
    src: Image.Image, focus_x: float = 0.5, focus_y: float = 0.5
) -> Image.Image:
    img = src.convert("RGBA")
    w, h = img.size
    side = min(w, h)
    cx = int(w * focus_x)
    cy = int(h * focus_y)
    left = max(0, min(w - side, cx - side // 2))
    top = max(0, min(h - side, cy - side // 2))
    return img.crop((left, top, left + side, top + side))


def fit_cover_169(src: Image.Image, target=(1024, 576)) -> Image.Image:
    img = src.convert("RGB")
    w, h = img.size
    tw, th = target
    src_ratio = w / h
    target_ratio = tw / th
    if src_ratio > target_ratio:
        new_w = int(h * target_ratio)
        x0 = (w - new_w) // 2
        img = img.crop((x0, 0, x0 + new_w, h))
    else:
        new_h = int(w / target_ratio)
        y0 = (h - new_h) // 2
        img = img.crop((0, y0, w, y0 + new_h))
    return img.resize(target, Image.LANCZOS)


def vignette(img: Image.Image, strength: float = 0.35) -> Image.Image:
    w, h = img.size
    mask = Image.new("L", (w, h), 0)
    from PIL import ImageDraw

    draw = ImageDraw.Draw(mask)
    draw.ellipse((-w // 4, -h // 4, w + w // 4, h + h // 4), fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(radius=int(min(w, h) * 0.20)))
    black = Image.new("RGB", (w, h), (0, 0, 0))
    base = img.convert("RGB")
    blended = Image.composite(base, black, mask)
    return Image.blend(base, blended, strength).convert("RGBA")


def add_logo_overlay(
    base: Image.Image,
    logo: Image.Image,
    width_ratio: float = 0.7,
    y_anchor: float = 0.78,
    shadow: bool = True,
) -> Image.Image:
    canvas = base.convert("RGBA").copy()
    bw, bh = canvas.size
    lw_target = int(bw * width_ratio)
    logo = logo.convert("RGBA")
    aspect = logo.size[1] / logo.size[0]
    logo = logo.resize((lw_target, int(lw_target * aspect)), Image.LANCZOS)
    x = (bw - logo.size[0]) // 2
    y = int(bh * y_anchor) - logo.size[1] // 2
    if shadow:
        shadow_img = Image.new("RGBA", logo.size, (0, 0, 0, 0))
        shadow_alpha = logo.split()[3].point(lambda a: int(a * 0.85))
        shadow_img.putalpha(shadow_alpha)
        shadow_img = shadow_img.filter(ImageFilter.GaussianBlur(radius=8))
        canvas.alpha_composite(shadow_img, (x + 4, y + 6))
    canvas.alpha_composite(logo, (x, y))
    return canvas


def boost(
    img: Image.Image, contrast: float = 1.08, saturation: float = 1.05
) -> Image.Image:
    out = ImageEnhance.Contrast(img.convert("RGB")).enhance(contrast)
    out = ImageEnhance.Color(out).enhance(saturation)
    return out.convert("RGBA")


def save(img: Image.Image, name: str) -> None:
    p = ROOT / name
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    img.save(p, "PNG", optimize=True)
    print(
        f"  wrote {name:24s} {img.size[0]}x{img.size[1]}  {p.stat().st_size // 1024} KB"
    )


def build() -> None:
    print("\n=== BUILD ===")

    hero = open_any(SRC / "steam_library_hero.jpg")
    capsule_2x = open_any(SRC / "steam_capsule_2x.jpg")
    open_any(SRC / "steam_logo.png")
    page_bg = open_any(SRC / "steam_page_bg.jpg")
    wm_logo = open_any(SRC / "wm_fo4_logo.png")

    pipboy = None
    for candidate in (
        "fandom_pipboy_3000.webp",
        "fandom_pipboy_hires.webp",
        "fandom_pipboy_loading.webp",
    ):
        p = SRC / candidate
        if p.exists():
            try:
                pipboy = open_any(p)
                print(
                    f"  pipboy source = {candidate} ({pipboy.size[0]}x{pipboy.size[1]})"
                )
                break
            except Exception:
                continue

    vaultboy = None
    for candidate in (
        "fandom_vaultboy_statue.webp",
        "fandom_vaultboy_fo3.webp",
        "fandom_vaultboy_fo3_clean.webp",
    ):
        p = SRC / candidate
        if p.exists():
            try:
                vaultboy = open_any(p)
                print(
                    f"  vaultboy source = {candidate} ({vaultboy.size[0]}x{vaultboy.size[1]})"
                )
                break
            except Exception:
                continue

    save(fit_cover_169(boost(vignette(hero, 0.30))), "cover.png")

    big_base = crop_centered_square(capsule_2x, focus_x=0.5, focus_y=0.40)
    big = big_base.resize((1024, 1024), Image.LANCZOS)
    big = boost(big, contrast=1.12, saturation=1.08)
    save(big, "fo4-big.png")

    hero_left = hero.crop((0, 0, int(hero.size[0] * 0.55), hero.size[1]))
    hero_left_sq = crop_centered_square(hero_left, focus_x=0.55, focus_y=0.55)
    combat = hero_left_sq.resize((1024, 1024), Image.LANCZOS)
    combat = boost(vignette(combat, 0.35), contrast=1.15, saturation=1.05)
    save(combat, "icon_combat.png")

    hero_right = hero.crop((int(hero.size[0] * 0.45), 0, hero.size[0], hero.size[1]))
    hero_right_sq = crop_centered_square(hero_right, focus_x=0.55, focus_y=0.50)
    explore = hero_right_sq.resize((1024, 1024), Image.LANCZOS)
    explore = boost(vignette(explore, 0.30), contrast=1.10, saturation=1.10)
    save(explore, "icon_explore.png")

    if pipboy is not None:
        pip = fit_to_square(pipboy, 1024, bg=(8, 18, 10, 255), pad=0.06)
        save(boost(pip, contrast=1.06, saturation=1.05), "icon_pipboy.png")
    else:
        print("  WARN pipboy unavailable, falling back to logo")

    if vaultboy is not None:
        menu = fit_to_square(vaultboy, 1024, bg=(12, 16, 12, 255), pad=0.08)
        save(boost(menu, contrast=1.05), "icon_menu.png")

        mainmenu_bg = page_bg.resize((1024, 1024), Image.LANCZOS)
        mainmenu_bg = vignette(mainmenu_bg, 0.40)
        vb_logo = wm_logo.copy()
        mainmenu = add_logo_overlay(
            mainmenu_bg, vb_logo, width_ratio=0.78, y_anchor=0.50, shadow=True
        )
        save(boost(mainmenu, contrast=1.05, saturation=1.0), "icon_mainmenu.png")
    else:
        print("  WARN vaultboy unavailable")

    loading_bg = page_bg.copy()
    loading_bg = crop_centered_square(loading_bg, 0.5, 0.45).resize(
        (1024, 1024), Image.LANCZOS
    )
    loading_bg = vignette(loading_bg, 0.55)
    if pipboy is not None:
        pip_small = pipboy.copy()
        ratio = 0.55
        pw = int(1024 * ratio)
        aspect = pip_small.size[1] / pip_small.size[0]
        pip_small = pip_small.resize((pw, int(pw * aspect)), Image.LANCZOS)
        x = (1024 - pip_small.size[0]) // 2
        y = (1024 - pip_small.size[1]) // 2
        loading_bg.alpha_composite(pip_small, (x, y))
    else:
        loading_bg = add_logo_overlay(
            loading_bg, wm_logo, width_ratio=0.70, y_anchor=0.50
        )
    save(boost(loading_bg, contrast=1.04), "icon_loading.png")


if __name__ == "__main__":
    fetch_all()
    report_sources()
    build()
    print("\n=== DONE ===")
    print(f"  output dir: {ROOT}")
    for p in sorted(ROOT.glob("*.png")):
        print(f"   - {p.name}  {p.stat().st_size // 1024} KB")
