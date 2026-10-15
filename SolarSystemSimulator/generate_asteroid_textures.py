#!/usr/bin/env python3
"""
generate_asteroid_textures.py
Generates scientifically accurate 2048x1024 equirectangular texture maps
for the 6 major recognized asteroids/minor planets:
  1. 1 Ceres (Dwarf Planet) - dark brownish-grey, Occator Crater bright salt spots (Cerealia Facula), Ahuna Mons
  2. 4 Vesta (Protoplanet) - bright buff/yellow basaltic rock, massive southern Rheasilvia crater, equatorial troughs
  3. 2 Pallas (B-type) - subtle slate-blue / bluish-grey hue, dense battered golf-ball cratering
  4. 10 Hygiea (C-type) - extremely dark charcoal / graphite soot-black (albedo ~0.07), primitive carbonaceous
  5. 433 Eros (S-type) - warm golden-tan / reddish-ochre silicate rock, regolith ponds, grooves
  6. 16 Psyche (M-type) - silvery-steel metallic sheen, nickel-iron crystalline texture, metallic glints
"""

import os
import math
import numpy as np
from PIL import Image

OUT_DIR = os.path.join(os.path.dirname(__file__), "assets", "textures", "planets")
W = 2048
H = 1024

def make_base_noise(seed=42):
    np.random.seed(seed)
    # Multi-octave smooth value noise
    grid = np.zeros((H, W), dtype=np.float32)
    for scale, weight in [(32, 0.4), (64, 0.25), (128, 0.15), (256, 0.1), (512, 0.07), (1024, 0.03)]:
        gh, gw = max(2, H // scale), max(4, W // scale)
        r = np.random.rand(gh, gw).astype(np.float32)
        im = Image.fromarray(r).resize((W, H), resample=Image.BILINEAR)
        grid += np.array(im, dtype=np.float32) * weight
    grid = (grid - grid.min()) / (grid.max() - grid.min() + 1e-6)
    return grid

def add_craters(heightmap, num_craters=400, seed=101):
    np.random.seed(seed)
    hm = heightmap.copy()
    y_coords, x_coords = np.mgrid[0:H, 0:W]

    for _ in range(num_craters):
        cx = np.random.randint(0, W)
        cy = np.random.randint(int(H * 0.05), int(H * 0.95))
        # Radius distributed with power law (many small, few large)
        r = int(np.random.exponential(scale=18.0) + 4)
        if r > 120: r = 120
        depth = np.random.uniform(0.12, 0.35)

        # Local bounding box
        x0, x1 = max(0, cx - r * 2), min(W, cx + r * 2 + 1)
        y0, y1 = max(0, cy - r * 2), min(H, cy + r * 2 + 1)

        dx = (x_coords[y0:y1, x0:x1] - cx)
        dy = (y_coords[y0:y1, x0:x1] - cy)
        d = np.sqrt(dx * dx + dy * dy) / float(r)

        # Crater profile: central bowl depression + raised rim + outer ejecta falloff
        bowl = np.clip(1.0 - d * d, 0.0, 1.0)
        rim = np.exp(-((d - 1.0) ** 2) / 0.08) * 0.5
        profile = -bowl * depth + rim * (depth * 0.6)

        hm[y0:y1, x0:x1] += profile

    return np.clip(hm, 0.0, 1.0)

# ==============================================================================
# 1. CERES
# ==============================================================================
def generate_ceres():
    print("Generating Ceres texture...")
    hm = make_base_noise(seed=1)
    hm = add_craters(hm, num_craters=500, seed=12)

    # Base dark brownish-charcoal clay/carbonate crust (albedo ~0.09)
    # RGB around (72, 68, 64)
    base_r = 72.0 + (hm - 0.5) * 35.0
    base_g = 68.0 + (hm - 0.5) * 32.0
    base_b = 64.0 + (hm - 0.5) * 28.0

    # Landmark: Occator Crater (lat ~20Â°N, lon ~240Â°)
    occ_y = int(H * (90 - 20) / 180.0) # ~398
    occ_x = int(W * 240.0 / 360.0)      # ~1365

    y_coords, x_coords = np.mgrid[0:H, 0:W]
    dx = (x_coords - occ_x).astype(np.float32)
    dy = (y_coords - occ_y).astype(np.float32)
    d = np.sqrt(dx * dx + dy * dy)

    # Occator dark rim / depression (radius ~55 px)
    crater_mask = np.clip((55.0 - d) / 15.0, 0.0, 1.0)
    base_r -= crater_mask * 18.0
    base_g -= crater_mask * 18.0
    base_b -= crater_mask * 18.0

    # Cerealia Facula: INTENSE bright white/cyan-tinted sodium carbonate salt dome in center (radius ~12 px)
    salt_glow = np.exp(-(d ** 2) / (2.0 * (9.0 ** 2)))
    salt_core = np.exp(-(d ** 2) / (2.0 * (4.5 ** 2)))

    # Secondary Vinalia Faculae (scattered salt patches 25 px to the east)
    dx2 = dx - 20.0
    dy2 = dy - 5.0
    d2 = np.sqrt(dx2 * dx2 + dy2 * dy2)
    vinalia = np.exp(-(d2 ** 2) / (2.0 * (6.0 ** 2))) * 0.75

    faculae = np.clip(salt_glow * 0.8 + salt_core * 0.7 + vinalia, 0.0, 1.0)

    # Salt is brilliant white with faint icy cyan tint: RGB (245, 250, 255)
    base_r = base_r * (1.0 - faculae) + 242.0 * faculae
    base_g = base_g * (1.0 - faculae) + 248.0 * faculae
    base_b = base_b * (1.0 - faculae) + 255.0 * faculae

    # Ahuna Mons (cryovolcanic bright dome at lat -10Â°, lon 316Â°)
    am_y = int(H * (90 - (-10)) / 180.0)
    am_x = int(W * 316.0 / 360.0)
    dam = np.sqrt((x_coords - am_x)**2 + (y_coords - am_y)**2)
    ahuna = np.exp(-(dam ** 2) / 45.0) * 0.6
    base_r += ahuna * 90.0
    base_g += ahuna * 95.0
    base_b += ahuna * 110.0

    img = np.stack([np.clip(base_r, 0, 255), np.clip(base_g, 0, 255), np.clip(base_b, 0, 255)], axis=-1).astype(np.uint8)
    Image.fromarray(img).save(os.path.join(OUT_DIR, "ceres.jpg"), quality=92)
    print("Ceres texture saved.")

# ==============================================================================
# 2. VESTA
# ==============================================================================
def generate_vesta():
    print("Generating Vesta texture...")
    hm = make_base_noise(seed=77)
    hm = add_craters(hm, num_craters=450, seed=88)

    # Bright yellowish-white to buff basaltic pyroxene crust (high albedo ~0.42)
    # Lighter eucrite highlands: (210, 198, 170), darker diogenite plains: (155, 145, 125)
    base_r = 175.0 + (hm - 0.5) * 60.0
    base_g = 165.0 + (hm - 0.5) * 55.0
    base_b = 140.0 + (hm - 0.5) * 45.0

    y_coords, x_coords = np.mgrid[0:H, 0:W]

    # Landmark: Rheasilvia Impact Basin centered at South Pole (lat -72Â°)
    rhea_y = int(H * 0.88) # ~900
    rhea_x = int(W * 0.50) # 1024
    dx = (x_coords - rhea_x).astype(np.float32)
    dy = (y_coords - rhea_y).astype(np.float32)
    d = np.sqrt(dx * dx + dy * dy)

    # Colossal basin depression (radius ~320 px)
    basin = np.exp(-((d - 160.0) ** 2) / (2.0 * (80.0 ** 2))) * 0.4
    # Central peak (22 km tall mountain!) in the center
    central_peak = np.exp(-(d ** 2) / (2.0 * (35.0 ** 2))) * 0.6

    base_r += (central_peak - basin) * 45.0
    base_g += (central_peak - basin) * 40.0
    base_b += (central_peak - basin) * 32.0

    # Divalia Fossae: Equatorial graben / parallel grooved troughs around lat -10Â° to +10Â°
    eq_dist = np.abs(y_coords - H * 0.5).astype(np.float32)
    trough_mask = np.exp(-(eq_dist ** 2) / (2.0 * (50.0 ** 2)))
    troughs = np.sin(y_coords * 0.35 + np.sin(x_coords * 0.03) * 3.0)
    trough_lines = np.clip(troughs * trough_mask * 0.4, -0.3, 0.3)

    base_r += trough_lines * 35.0
    base_g += trough_lines * 32.0
    base_b += trough_lines * 25.0

    img = np.stack([np.clip(base_r, 0, 255), np.clip(base_g, 0, 255), np.clip(base_b, 0, 255)], axis=-1).astype(np.uint8)
    Image.fromarray(img).save(os.path.join(OUT_DIR, "vesta.jpg"), quality=92)
    print("Vesta texture saved.")

# ==============================================================================
# 3. PALLAS
# ==============================================================================
def generate_pallas():
    print("Generating Pallas texture...")
    # B-type asteroid with low albedo (~0.12) and distinctive SLATE-BLUE / BLUISH-GREY hue
    hm = make_base_noise(seed=303)
    # Extra dense battered "golf-ball" cratering!
    hm = add_craters(hm, num_craters=750, seed=404)

    # Base bluish-grey: R ~ 85, G ~ 98, B ~ 116 (notice B > R by 30 units!)
    base_r = 85.0 + (hm - 0.5) * 40.0
    base_g = 96.0 + (hm - 0.5) * 42.0
    base_b = 114.0 + (hm - 0.5) * 48.0

    # Mottled high-frequency crater rims (golf-ball dimples)
    dimples = (np.sin(hm * 35.0) * 0.5 + 0.5) * 18.0
    base_r += dimples * 0.7
    base_g += dimples * 0.85
    base_b += dimples * 1.15

    img = np.stack([np.clip(base_r, 0, 255), np.clip(base_g, 0, 255), np.clip(base_b, 0, 255)], axis=-1).astype(np.uint8)
    Image.fromarray(img).save(os.path.join(OUT_DIR, "pallas.jpg"), quality=92)
    print("Pallas texture saved.")

# ==============================================================================
# 4. HYGIEA
# ==============================================================================
def generate_hygiea():
    print("Generating Hygiea texture...")
    # C-type asteroid, extremely dark primitive carbonaceous surface (albedo ~0.07)
    # The darkest large asteroid in the solar system!
    hm = make_base_noise(seed=505)
    hm = add_craters(hm, num_craters=350, seed=606)

    # Deep charcoal / graphite soot-black: RGB around (44, 42, 40)
    base_r = 46.0 + (hm - 0.5) * 22.0
    base_g = 43.0 + (hm - 0.5) * 20.0
    base_b = 41.0 + (hm - 0.5) * 19.0

    # Two subtle ancient impact basins (Serpens Basin & Calore Basin)
    y_coords, x_coords = np.mgrid[0:H, 0:W]
    b1 = np.exp(-(((x_coords - W * 0.35)**2 + (y_coords - H * 0.4)**2)) / (2.0 * (130.0**2))) * 0.25
    b2 = np.exp(-(((x_coords - W * 0.72)**2 + (y_coords - H * 0.6)**2)) / (2.0 * (110.0**2))) * 0.25

    base_r -= (b1 + b2) * 12.0
    base_g -= (b1 + b2) * 12.0
    base_b -= (b1 + b2) * 12.0

    img = np.stack([np.clip(base_r, 0, 255), np.clip(base_g, 0, 255), np.clip(base_b, 0, 255)], axis=-1).astype(np.uint8)
    Image.fromarray(img).save(os.path.join(OUT_DIR, "hygiea.jpg"), quality=92)
    print("Hygiea texture saved.")

# ==============================================================================
# 5. EROS
# ==============================================================================
def generate_eros():
    print("Generating Eros texture...")
    # S-type asteroid: warm golden-tan / reddish-ochre silicate (albedo ~0.25)
    hm = make_base_noise(seed=707)
    hm = add_craters(hm, num_craters=550, seed=808)

    # Warm golden-tan / silicate rust: R ~ 185, G ~ 148, B ~ 108
    base_r = 185.0 + (hm - 0.5) * 55.0
    base_g = 148.0 + (hm - 0.5) * 45.0
    base_b = 108.0 + (hm - 0.5) * 35.0

    y_coords, x_coords = np.mgrid[0:H, 0:W]

    # Landmark: Regolith ponds (flat, smooth dust-filled depressions found by NEAR Shoemaker)
    # Characteristic smooth, slightly lighter sediment basins
    for px, py, pr in [(int(W * 0.3), int(H * 0.45), 45), (int(W * 0.65), int(H * 0.55), 55), (int(W * 0.8), int(H * 0.35), 35)]:
        d = np.sqrt((x_coords - px)**2 + (y_coords - py)**2)
        pond = np.clip((pr - d) / 10.0, 0.0, 1.0)
        # Ponds are smoother and slightly brighter golden-buff
        base_r += pond * 22.0
        base_g += pond * 18.0
        base_b += pond * 12.0

    # Saddle depression (Himeros)
    saddle = np.exp(-((x_coords - W * 0.5)**2 + (y_coords - H * 0.5)**2) / (2.0 * (180.0**2))) * 0.3
    base_r -= saddle * 25.0
    base_g -= saddle * 22.0
    base_b -= saddle * 18.0

    img = np.stack([np.clip(base_r, 0, 255), np.clip(base_g, 0, 255), np.clip(base_b, 0, 255)], axis=-1).astype(np.uint8)
    Image.fromarray(img).save(os.path.join(OUT_DIR, "eros.jpg"), quality=92)
    print("Eros texture saved.")

# ==============================================================================
# 6. PSYCHE
# ==============================================================================
def generate_psyche():
    print("Generating Psyche texture...")
    # M-type metallic asteroid: Silvery-steel metallic grey with crystalline Widmanstatten-like glints
    hm = make_base_noise(seed=909)
    hm = add_craters(hm, num_craters=400, seed=999)

    # Silvery metallic steel-grey: R ~ 168, G ~ 170, B ~ 176 (cool silvery tone)
    base_r = 168.0 + (hm - 0.5) * 45.0
    base_g = 170.0 + (hm - 0.5) * 45.0
    base_b = 176.0 + (hm - 0.5) * 45.0

    # Crystalline metallic WidmanstÃ¤tten cross-hatch pattern (nickel-iron metal intergrowth)
    y_coords, x_coords = np.mgrid[0:H, 0:W]
    stripes1 = np.sin((x_coords + y_coords) * 0.25)
    stripes2 = np.sin((x_coords - y_coords) * 0.25)
    metal_crystal = np.clip(stripes1 * stripes2 * 0.5 + 0.5, 0.0, 1.0) * 22.0

    base_r += metal_crystal
    base_g += metal_crystal * 1.02
    base_b += metal_crystal * 1.08

    # Bright polished metallic nodules / glints
    np.random.seed(42)
    nodules = (np.random.rand(H, W) > 0.994).astype(np.float32)
    im_nod = Image.fromarray(nodules).resize((W, H), resample=Image.BOX)
    nodule_glow = np.array(im_nod) * 55.0

    base_r += nodule_glow
    base_g += nodule_glow * 1.02
    base_b += nodule_glow * 1.05

    img = np.stack([np.clip(base_r, 0, 255), np.clip(base_g, 0, 255), np.clip(base_b, 0, 255)], axis=-1).astype(np.uint8)
    Image.fromarray(img).save(os.path.join(OUT_DIR, "psyche.jpg"), quality=92)
    print("Psyche texture saved.")

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    generate_ceres()
    generate_vesta()
    generate_pallas()
    generate_hygiea()
    generate_eros()
    generate_psyche()
    print("All 6 asteroid textures generated successfully!")

if __name__ == "__main__":
    main()
