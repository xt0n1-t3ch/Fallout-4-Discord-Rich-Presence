from pathlib import Path
from PIL import Image, ImageFilter, ImageEnhance
import shutil

DST = Path(r"D:\X\2-Dev\f4drp-presence-overhaul\dist\discord-assets")
PV = DST / "preview"

shutil.copy2(PV / "PB-LOGO_pipboy3000_transparent_boosted.png", DST / "icon_pipboy.png")
print("icon_pipboy.png written", (DST / "icon_pipboy.png").stat().st_size // 1024, "KB")

im = Image.open(PV / "MM-B_vault_111_door.png").convert("RGBA")
bbox = im.getbbox()
if bbox and (bbox[2] - bbox[0] < im.size[0] or bbox[3] - bbox[1] < im.size[1]):
    im = im.crop(bbox)
side = max(im.size)
canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
canvas.paste(im, ((side - im.size[0]) // 2, (side - im.size[1]) // 2), im)
canvas = canvas.resize((1024, 1024), Image.LANCZOS)
rgb = canvas.convert("RGB")
alpha = canvas.split()[-1]
rgb = ImageEnhance.Contrast(rgb).enhance(1.12)
rgb = ImageEnhance.Color(rgb).enhance(1.10)
final = Image.merge("RGBA", (*rgb.split(), alpha))
final = final.filter(ImageFilter.UnsharpMask(radius=1.4, percent=90, threshold=2))
final.save(DST / "icon_mainmenu.png", "PNG", optimize=True)
print(
    "icon_mainmenu.png written",
    (DST / "icon_mainmenu.png").stat().st_size // 1024,
    "KB",
)
