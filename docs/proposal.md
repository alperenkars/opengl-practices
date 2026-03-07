# Project Proposal
## COMP410 Computer Graphics — Spring 2026

**Project Title:** Virtual Koç University Campus Tour

**Team Members:** Alperen Kars, Uğur Öner

**Instructor:** Yücel Yemez

---

## 1. Motivation

Koç University has a large and architecturally rich campus that can be difficult to navigate for new students and visitors. This project aims to create an interactive 3D virtual tour of the KU campus exterior, allowing users to freely walk through a faithful representation of the campus from a first-person perspective. Beyond its practical utility, this application serves as a comprehensive exercise in real-time 3D graphics — integrating nearly every major topic covered in COMP410.

---

## 2. Project Description

We will implement an interactive first-person 3D campus tour using shader-based OpenGL in C++. The user will be able to navigate through the campus using keyboard and mouse input, observe the buildings and environment under a realistic lighting model, and experience the campus layout as if walking through it.

**Scope — Phase 1 (Progress Presentation):**
- 3D models of all major academic and social buildings (SCI, ENG, CASE, Law, Library, Sports Center, Student Center)
- Ground plane with basic terrain layout
- First-person camera navigation (WASD + mouse look)
- Perspective projection
- Basic Phong illumination (ambient + diffuse + specular) with a directional sun light
- Flat-color or basic textured surfaces

**Scope — Phase 2 (Final Submission):**
- Texture mapping on all building facades, ground, and paths
- Skybox (environment cube map)
- Multiple light sources (sun + ambient fill)
- Smooth collision detection (camera stays on ground, cannot walk through buildings)
- Interactive elements: building labels / info overlays
- Bonus (if time permits): shadow mapping, day/night cycle toggle

---

## 3. Computer Graphics Techniques

This project directly applies the following course topics:

| Technique | Application in Project |
|---|---|
| Shader-based OpenGL (GLSL) | All rendering via vertex + fragment shaders |
| Geometric transformations | Placing and scaling each building in world space |
| Viewing & Perspective projection | First-person camera using LookAt + Perspective |
| Phong illumination model | Per-fragment shading on all surfaces |
| Texture mapping | Building facades, ground, skybox |
| Hierarchical / scene graph modeling | Campus scene organized as a tree of objects |
| Input and interaction | WASD movement, mouse-look, keyboard shortcuts |
| Buffers (VAO/VBO) | Geometry data on the GPU for all objects |

---

## 4. Technical Approach

**Language & API:** C++17 with shader-based OpenGL (core profile). GLFW for window/input management, GLEW for extensions, Angel.h utilities from the course textbook.

**Scene representation:** A scene graph where the root node is the campus, with child nodes for each building. Each building node holds its transform (position, rotation, scale) and a set of mesh components.

**Camera system:** A first-person camera defined by position, yaw, and pitch angles. The view matrix is computed each frame from these values using `LookAt()`. Camera is constrained to ground level with basic AABB collision against buildings.

**Building modeling:** Buildings will be constructed from geometric primitives (boxes, cylinders for columns, flat quads for roofs). Each building is modeled in local space and placed in the world via a model matrix.

**Lighting:** A single directional light (sun) with Phong shading computed per-fragment in the GLSL fragment shader.

---

## 5. Milestones

| Date | Deliverable |
|---|---|
| March 9, 2026 | Proposal submission |
| Late April 2026 | Progress presentation: basic scene, navigation, lighting, ≥3 buildings |
| End of semester | Final: full campus, textures, skybox, collision, polish |

---

## 6. Division of Work

| Area | Primary |
|---|---|
| Scene graph & building models | Alperen Kars |
| Camera system & input | Uğur Öner |
| Shader development (lighting) | Both |
| Texture mapping | Both |
| Collision detection | Alperen Kars |
| Skybox & environment | Uğur Öner |
| Report & documentation | Both |

*Division may shift as the project progresses.*

---

## 7. References

- Angel, E. *Interactive Computer Graphics: A Top-Down Approach with OpenGL*, 6th ed.
- OpenGL Programming Guide, 8th ed., Khronos Group
- KU Campus map (official)
