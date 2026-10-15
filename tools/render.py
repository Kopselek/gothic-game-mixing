"""Tiny software rasterizer for previewing meshes as PNG (orthographic, textured, z-buffered)."""
import math

from png import write_png


def render(path, triangles, view="front", size=(360, 640), up=1, right=0, depth=2, flip_right=False, cull=False):
    """triangles: [((p0, p1, p2), (uv0, uv1, uv2), texture)] where texture has width/height/rgba.

    `up`, `right` and `depth` pick which coordinate axes map to the image; the camera
    looks along -depth. view="side" swaps right for depth. cull=True drops the triangles that
    face away from it, as a game does: counter-clockwise is the front (3ds Max's, an ASC's).
    """
    if view == "side":
        right, depth = depth, right
    w, h = size
    pts = [p for tri, _, _ in triangles for p in tri]
    lo = [min(p[i] for p in pts) for i in range(3)]
    hi = [max(p[i] for p in pts) for i in range(3)]
    scale = 0.92 * min(w / max(hi[right] - lo[right], 1e-6), h / max(hi[up] - lo[up], 1e-6))
    cx, cy = (lo[right] + hi[right]) / 2, (lo[up] + hi[up]) / 2
    sign = -1 if flip_right else 1

    def project(p):
        return (w / 2 + sign * (p[right] - cx) * scale, h / 2 - (p[up] - cy) * scale, p[depth])

    color = bytearray(b"\x30\x30\x38\xff" * (w * h))
    zbuf = [-math.inf] * (w * h)
    light = (0.3, 0.5, 0.8)
    for tri, uvs, tex in triangles:
        a, b, c = (project(p) for p in tri)
        if cull:
            e, f = [tri[1][i] - tri[0][i] for i in range(3)], [tri[2][i] - tri[0][i] for i in range(3)]
            k, l = (depth + 1) % 3, (depth + 2) % 3
            if e[k] * f[l] - e[l] * f[k] <= 0:  # the face normal's component towards the camera
                continue
        # flat shading from the 3D face normal, in image-axis order (right, up, depth)
        p0, p1, p2 = ([q[right], q[up], q[depth]] for q in tri)
        u = [p1[i] - p0[i] for i in range(3)]
        v = [p2[i] - p0[i] for i in range(3)]
        n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
        ln = math.sqrt(sum(x * x for x in n)) or 1
        shade = 0.35 + 0.65 * abs(sum(n[i] / ln * light[i] for i in range(3)))
        area = (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1])
        if abs(area) < 1e-9:
            continue
        x0, x1 = max(int(min(a[0], b[0], c[0])), 0), min(int(max(a[0], b[0], c[0])) + 1, w - 1)
        y0, y1 = max(int(min(a[1], b[1], c[1])), 0), min(int(max(a[1], b[1], c[1])) + 1, h - 1)
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                px, py = x + 0.5, y + 0.5
                w0 = ((b[0] - px) * (c[1] - py) - (c[0] - px) * (b[1] - py)) / area
                w1 = ((c[0] - px) * (a[1] - py) - (a[0] - px) * (c[1] - py)) / area
                w2 = 1 - w0 - w1
                if w0 < 0 or w1 < 0 or w2 < 0:
                    continue
                z = w0 * a[2] + w1 * b[2] + w2 * c[2]
                i = y * w + x
                if z <= zbuf[i]:
                    continue
                if tex is None:
                    r = g = bl = 200
                else:
                    tu = w0 * uvs[0][0] + w1 * uvs[1][0] + w2 * uvs[2][0]
                    tv = w0 * uvs[0][1] + w1 * uvs[1][1] + w2 * uvs[2][1]
                    tx = int((tu % 1.0) * tex.width) % tex.width
                    ty = int((tv % 1.0) * tex.height) % tex.height
                    o = (ty * tex.width + tx) * 4
                    if tex.rgba[o + 3] < 128:  # alpha-tested, as a game draws hair cards
                        continue
                    r, g, bl = tex.rgba[o], tex.rgba[o + 1], tex.rgba[o + 2]
                zbuf[i] = z
                color[i * 4:i * 4 + 3] = bytes((int(r * shade), int(g * shade), int(bl * shade)))
    write_png(path, w, h, bytes(color))
