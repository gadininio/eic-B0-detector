# Sensitive area and angular coverage of the B0 tracker, old (epic 43f427e) vs new (ec774d8) geometry.
# Usage: python3 b0area.py <dir with {43f427e,ec774d8}_B0_tracker.xml> [out.png]
#        (see notes/claude_commands.md, 2026-11-18); with out.png also draws the maps.
# Rasterizes each station's sensors in the plane perpendicular to the ion axis (local x = beam-frame x,
# +x = electron side), then maps to theta = atan(r / z_station) from the IP along the ion axis.
import re, sys, math
import numpy as np
D = sys.argv[1]
G = 0.25                                   # mm grid
xs = np.arange(-200, 200, G) + G / 2
X, Y = np.meshgrid(xs, xs, indexing="ij")
R, PHI = np.hypot(X, Y), np.degrees(np.arctan2(Y, X))

# ---- old: 36 trapezoids of 10 deg per layer (30 big to r = 150 mm, 6 small on the +x side)
def old_layer(rout_small):
    rin, d = 35.0, math.radians(10)
    m = np.zeros(X.shape, bool)
    for big, n, phi0 in ((True, 30, 3.5 * 10 + 180), (False, 6, -(2.5 * 10 + 180))):
        rout = 150.0 if big else rout_small
        h = rout * math.cos(d / 2) - rin                      # radial height of the trd
        for i in range(n):
            phi = math.radians(phi0 + 10 * i)
            # module center at (-r cos phi, -r sin phi): radial unit vector u
            u = np.array([-math.cos(phi), -math.sin(phi)]); v = np.array([-u[1], u[0]])
            s = X * u[0] + Y * u[1] - rin                    # distance from the inner edge, radially
            t = X * v[0] + Y * v[1]
            hw = (2 * rin * math.sin(d / 2) + (2 * rout * math.sin(d / 2) - 2 * rin * math.sin(d / 2)) * s / h) / 2
            m |= (s >= 0) & (s <= h) & (np.abs(t) <= hw)
    return m
old = {z: old_layer(ro) for z, ro in ((5895, 85), (6165, 95), (6435, 100), (6705, 105))}

# ---- new: staves (3 sensors 16 x 16 mm at y = -15, 0, +15 mm around the stave center), front + back faces
txt = open(f"{D}/ec774d8_B0_tracker.xml").read()
zst = {1: 5850.0, 2: 6272.2, 3: 6601.0, 4: 6880.0}
new = {}
for st in (1, 2, 3, 4):
    block = re.search(rf'<module_layout name="B0ModuleLayout_St{st}">(.*?)</module_layout>', txt, re.S).group(1)
    m = np.zeros(X.shape, bool); n = 0
    for x0, y0 in re.findall(r'<module x="([-\d.]+)\*mm" y="([-\d.]+)\*mm"/>', block):
        x0, y0 = float(x0), float(y0); n += 1
        for dy in (-15, 0, 15):
            m |= (np.abs(X - x0) <= 8) & (np.abs(Y - (y0 + dy)) <= 8)
    new[zst[st]] = m
    print(f"new station {st}: {n} staves, {3 * n} sensors, sum of sensor areas {3 * n * 256 / 100:.0f} cm2")

A = G * G / 100                             # cm2 per cell
for name, g in (("old", old), ("new", new)):
    print(f"\n{name}: sensitive area per station (union of both faces) [cm2]:",
          ", ".join(f"z={z:.0f}: {m.sum() * A:.0f}" for z, m in g.items()))
    rin = min(R[m].min() for m in g.values())
    print(f"  smallest sensitive radius {rin:.1f} mm")

# coverage vs theta and phi: a straight line from the IP along (theta, phi) crosses station z at r = z tan(theta)
def nhit(g, tx, ty):                      # direction slopes tx = x/z, ty = y/z [mrad] -> number of stations hit
    n = np.zeros(np.shape(tx), int)
    for z, m in g.items():
        ix = np.clip(((z * np.asarray(tx) / 1000 + 200) / G).astype(int), 0, len(xs) - 1)
        iy = np.clip(((z * np.asarray(ty) / 1000 + 200) / G).astype(int), 0, len(xs) - 1)
        n += m[ix, iy]
    return n
def covered(g, th, ph):                   # th [mrad], ph [deg] arrays -> number of stations hit
    t = np.tan(th / 1000) * 1000
    return nhit(g, t * np.cos(np.radians(ph)), t * np.sin(np.radians(ph)))
th = np.linspace(4, 22, 1801); ph = np.linspace(-180, 180, 1441)[:-1]
TH, PH = np.meshgrid(th, ph, indexing="ij")
cov = {name: covered(g, TH, PH) >= 3 for name, g in (("old", old), ("new", new))}
frac = {}
print("\nfraction of directions hitting >= 3 stations (straight line from the IP):")
print(f"{'theta [mrad]':>14} {'old':>6} {'new':>6}")
for lo, hi in ((5.5, 8), (8, 11), (11, 14), (14, 17), (17, 20), (5.5, 20)):
    s = (TH[:, 0] >= lo) & (TH[:, 0] < hi)
    # weight each theta row by sin(theta) ~ theta (solid angle)
    w = th[s][:, None]
    f = {k: (cov[k][s] * w).sum() / (w.sum() * len(ph)) for k in ("old", "new")}
    frac[(lo, hi)] = f
    print(f"{lo:>6}-{hi:<6} " + " ".join(f"{f[k] * 100:5.0f}%" for k in ("old", "new")))
print(f"{'phi [deg]':>14} {'old':>6} {'new':>6}   (theta 5.5-20 mrad)")
s = (th >= 5.5) & (th < 20)
for lo in range(-180, 180, 45):
    p = (ph >= lo) & (ph < lo + 45)
    w = th[s][:, None] * np.ones(p.sum())
    print(f"{lo:>6}-{lo + 45:<6} " + " ".join(f"{(cov[k][s][:, p] * w).sum() / w.sum() * 100:5.0f}%" for k in ("old", "new")))

if len(sys.argv) > 2:
    # left: sensitive area of each station (sensors in the station plane); right: straight-line coverage,
    # number of stations a line from the IP crosses, vs its direction (slopes x/z, y/z)
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.colors import ListedColormap, BoundaryNorm
    ink, muted, grid = "#1f1f1e", "#6b6a66", "#d9d8d4"
    sens = "#256abf"                                          # sensor fill (one hue)
    seq = ListedColormap(["#f0efec", "#cde2fb", "#86b6ef", "#3987e5", "#0d366b"])   # 0..4 stations
    norm = BoundaryNorm(np.arange(-0.5, 5), seq.N)
    plt.rcParams.update({"font.size": 10, "axes.edgecolor": muted, "axes.labelcolor": ink,
                         "xtick.color": muted, "ytick.color": muted, "axes.titlecolor": ink})
    fig = plt.figure(figsize=(16, 8.6), facecolor="white")
    gs = fig.add_gridspec(2, 5, width_ratios=[1, 1, 1, 1, 1.9], wspace=0.28, hspace=0.32,
                          left=0.05, right=0.97, top=0.88, bottom=0.07)
    ext = [xs[0] - G / 2, xs[-1] + G / 2] * 2
    t = np.linspace(0, 2 * np.pi, 361)
    for row, (name, g, label) in enumerate((("old", old, "old B0 (epic 43f427e): 4 layers of silicon disks"),
                                            ("new", new, "new B0 (epic ec774d8): 4 stations of 16×16 mm AC-LGAD sensors"))):
        for col, (z, m) in enumerate(g.items()):
            ax = fig.add_subplot(gs[row, col])
            ax.imshow(np.where(m, 1.0, np.nan).T, origin="lower", extent=ext, cmap=ListedColormap([sens]),
                      interpolation="nearest")
            for tm in (5.5, 20):                              # B0-direction cone at this z
                ax.plot(z * tm / 1000 * np.cos(t), z * tm / 1000 * np.sin(t), color=ink, lw=0.8, ls="--")
            ax.set_xlim(-170, 170); ax.set_ylim(-170, 170); ax.set_aspect("equal")
            ax.set_title(f"station {col + 1}, z = {z:.0f} mm\n{m.sum() * A:.0f} cm²", fontsize=9.5)
            ax.set_xlabel("x [mm]  (+x: electron side)" if col == 0 else "x [mm]", fontsize=8.5)
            if col == 0: ax.set_ylabel("y [mm]", fontsize=8.5)
            ax.tick_params(labelsize=8)
            if col == 0:
                ax.text(-0.32, 1.32, label, transform=ax.transAxes, fontsize=12, fontweight="bold", color=ink)
        ax = fig.add_subplot(gs[row, 4])
        sl = np.linspace(-25, 25, 801)
        TX, TY = np.meshgrid(sl, sl, indexing="ij")
        im = ax.imshow(nhit(g, TX, TY).T, origin="lower", extent=[sl[0], sl[-1]] * 2, cmap=seq, norm=norm,
                       interpolation="nearest")
        for tm in (5.5, 20):
            ax.plot(tm * np.cos(t), tm * np.sin(t), color=ink, lw=1.2, ls="--")
        ax.set_aspect("equal"); ax.set_xlim(-25, 25); ax.set_ylim(-25, 25)
        ax.set_xlabel("θx = x/z [mrad]  (+: electron side)", fontsize=9); ax.set_ylabel("θy = y/z [mrad]", fontsize=9)
        f = frac[(5.5, 20)][name]
        ax.set_title(f"straight lines from the IP: stations crossed\n≥ 3 stations for {f * 100:.0f}% of directions "
                     f"at 5.5–20 mrad (dashed)", fontsize=9.5)
        cb = fig.colorbar(im, ax=ax, ticks=range(5), fraction=0.046, pad=0.03)
        cb.set_label("stations crossed", fontsize=9); cb.ax.tick_params(labelsize=8)
    fig.suptitle("B0 tracker coverage, old vs new geometry (ion-axis frame; dashed circles: θ = 5.5 and 20 mrad)",
                 fontsize=13, color=ink)
    fig.savefig(sys.argv[2], dpi=130)
    print(f"\nwrote {sys.argv[2]}")
