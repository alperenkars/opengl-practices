#!/usr/bin/env python3
"""Parse OSM building data and convert to local meter coordinates for OpenGL."""

import json
import math

# Reference point: approximate campus center
REF_LAT = 41.2058
REF_LON = 29.0740

# Buildings we want (OSM name -> our code name)
TARGET_BUILDINGS = {
    "Rahmi M. Koç Mühendislik Fakültesi": "ENG",
    "Fen Fakültesi": "SCI",
    "İktisadi ve İdari Bilimler Fakültesi": "CASE",
    "Suna Kıraç Kütüphanesi": "Library",
    "Öğrenci Merkezi": "StudentCenter",
    "Semahat Arsel Spor Salonu": "Sports",
    "İnsani Bilimler ve Edebiyat Fakültesi": "CASS",
    "Sevgi Gönül Oditoryumu": "Auditorium",
    "Tıp Fakültesi": "MED",
    "ELC Binası": "ELC",
    "Rektörlük": "Rectorate",
    "Semahat & Nusret Arsel (SNA) Building": "SNA",
}

# Estimated building heights (meters) — floors * ~4m
HEIGHTS = {
    "ENG": 16.0,
    "SCI": 16.0,
    "CASE": 16.0,
    "Library": 12.0,
    "StudentCenter": 10.0,
    "Sports": 14.0,
    "CASS": 16.0,
    "Auditorium": 10.0,
    "MED": 16.0,
    "ELC": 12.0,
    "Rectorate": 10.0,
    "SNA": 16.0,
}

# Default colors (muted building tones)
COLORS = {
    "ENG":           (0.75, 0.70, 0.65),
    "SCI":           (0.80, 0.75, 0.70),
    "CASE":          (0.70, 0.72, 0.68),
    "Library":       (0.85, 0.80, 0.72),
    "StudentCenter": (0.78, 0.74, 0.70),
    "Sports":        (0.65, 0.70, 0.75),
    "CASS":          (0.73, 0.71, 0.68),
    "Auditorium":    (0.72, 0.68, 0.65),
    "MED":           (0.77, 0.73, 0.70),
    "ELC":           (0.74, 0.72, 0.69),
    "Rectorate":     (0.80, 0.78, 0.72),
    "SNA":           (0.76, 0.74, 0.71),
}


def latlon_to_meters(lat, lon):
    """Convert lat/lon to local meters relative to REF_LAT/REF_LON.
    X = East, Z = North (but we'll negate Z so North = -Z in OpenGL).
    """
    dx = (lon - REF_LON) * math.cos(math.radians(REF_LAT)) * 111320.0
    dz = (lat - REF_LAT) * 110540.0
    return dx, -dz  # negate so walking "north" on map = -Z in OpenGL


def main():
    with open("assets/campus_raw.json") as f:
        data = json.load(f)

    # Build node lookup: id -> (lat, lon)
    nodes = {}
    for el in data["elements"]:
        if el["type"] == "node":
            nodes[el["id"]] = (el["lat"], el["lon"])

    # Process buildings
    results = []
    for el in data["elements"]:
        if el["type"] != "way":
            continue
        name = el.get("tags", {}).get("name", "")
        if name not in TARGET_BUILDINGS:
            continue

        code = TARGET_BUILDINGS[name]

        # Get polygon coordinates
        coords = []
        for nid in el.get("nodes", []):
            if nid in nodes:
                lat, lon = nodes[nid]
                x, z = latlon_to_meters(lat, lon)
                coords.append((x, z))

        if not coords:
            continue

        # Compute bounding box
        xs = [c[0] for c in coords]
        zs = [c[1] for c in coords]
        min_x, max_x = min(xs), max(xs)
        min_z, max_z = min(zs), max(zs)

        center_x = (min_x + max_x) / 2.0
        center_z = (min_z + max_z) / 2.0
        width = max_x - min_x    # X dimension
        depth = max_z - min_z    # Z dimension
        height = HEIGHTS.get(code, 12.0)
        color = COLORS.get(code, (0.75, 0.72, 0.70))

        results.append({
            "code": code,
            "name": name,
            "cx": center_x,
            "cz": center_z,
            "width": width,
            "depth": depth,
            "height": height,
            "color": color,
        })

    # Print as C++ code
    print("// Generated from OpenStreetMap data")
    print("// Reference: lat={}, lon={}".format(REF_LAT, REF_LON))
    print("// X = East, Z = South (OpenGL), Y = Up")
    print("std::vector<Building> buildings = {")
    for r in sorted(results, key=lambda x: x["code"]):
        c = r["color"]
        print('    {{"{code:<14s} {{{cx:>8.1f}f, 0.0f, {cz:>8.1f}f}}, '
              '{{{w:>5.1f}f, {h:>5.1f}f, {d:>5.1f}f}}, '
              '{{{r:.2f}f, {g:.2f}f, {b:.2f}f}}}},'.format(
                  code=r["code"] + '",',
                  cx=r["cx"], cz=r["cz"],
                  w=r["width"], h=r["height"], d=r["depth"],
                  r=c[0], g=c[1], b=c[2]))
    print("};")

    print("\n// Camera start: south of campus, looking north")
    print("// Camera(glm::vec3(0.0f, 1.7f, 60.0f))")


if __name__ == "__main__":
    main()
