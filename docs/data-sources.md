# External Data Sources

## 1. Campus Layout — OpenStreetMap Building Footprints

**What it gives you:** Building outlines, positions, and sometimes floor counts for all KU campus buildings.

**How to get it:**
1. Go to https://overpass-turbo.eu
2. Paste this query and click Run:
   ```
   [out:json];
   (
     way["building"](around:1000, 41.2052, 29.0771);
     relation["building"](around:1000, 41.2052, 29.0771);
   );
   out body;
   >;
   out skel qt;
   ```
3. Click Export → GeoJSON
4. Save as `assets/campus_footprints.geojson`

**License:** CC BY-SA — attribution required. Fine for a course project.

**How to use in code:** Building polygon coordinates give you the world-space position and footprint of each building. Use these to place and scale your box primitives accurately.

---

## 1.5 Blender Mesh Generation (blosm) + Runtime Import

**What it gives you:** A single terrain + building mesh scene with UVs/baked materials, exported from Blender and loaded directly by the OpenGL app.

**Workflow used:**
1. Import OSM footprints/terrain into Blender using **blosm**
2. Build/clean meshes and UVs in Blender
3. Export scene as OBJ or glTF/GLB
4. Load at runtime with **Assimp** in `src/mesh.cpp`

**Attribution / license notes:**
- OpenStreetMap data: **ODbL**
- blosm addon is used as a data-prep tool in Blender

---

## 2. Building Reference Photos & Facade Textures

**What it gives you:** Visual reference for modeling + actual textures for building facades.

**How to get it:** Walk the campus and photograph each building with your phone.

**Split of work:**
- Alperen: SCI, ENG, CASE buildings
- Uğur: Library (Suna Kıraç), Sports Center, Student Center

**Tips:**
- Shoot facades straight-on (not at an angle) for clean texture extraction
- Photograph in overcast light to avoid harsh shadows baked into the texture
- Save photos to `assets/textures/buildings/`

---

## 3. Generic Material Textures (concrete, glass, asphalt, grass)

**Source:** https://polyhaven.com/textures

**License:** CC0 — no attribution required, free for any use.

**How to get it:**
1. Search for the material you need (e.g. "concrete", "asphalt", "grass")
2. Download at 1K or 2K resolution (sufficient for this project)
3. Grab the Diffuse/Albedo map at minimum; Normal map if you want extra detail
4. Save to `assets/textures/materials/`

---

## 4. Skybox

**Source:** https://polyhaven.com/hdris

**License:** CC0

**How to get it:**
1. Download any outdoor HDRI (e.g. "rural landscape", "blue sky")
2. Convert to a cube map using https://matheowis.github.io/HDRI-to-CubeMap/
3. Save the 6 face images to `assets/textures/skybox/`

---

## Summary

| Data | Source | Needed By |
|------|--------|-----------|
| Building footprints (GeoJSON) | overpass-turbo.eu | Before coding starts |
| Blender terrain/building scene | Blender + blosm (from OSM) | Mesh import phase |
| Building facade photos | Take yourselves on campus | Before texture phase |
| Generic material textures | polyhaven.com | During texture phase |
| Skybox cube map | polyhaven.com + HDRI converter | During final phase |
