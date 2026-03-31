# KU Campus Tour — CLAUDE.md

## Project
Virtual Koç University Campus Tour — COMP410 Computer Graphics, Spring 2026
Team: Alperen Kars & Uğur Öner
Instructor: Yücel Yemez

## Tech Stack
- C++17, OpenGL 4.1 core profile (macOS max)
- GLSL 410 shaders (vertex + fragment)
- GLM for math (glm::vec3, glm::mat4, etc.)
- GLFW for windowing and input
- GLEW for OpenGL extension loading

## Build
```bash
make        # builds ./campus
make clean  # removes binary
```

## Run
```bash
./campus
```

## Project Structure
```
src/            # C++ source files
shaders/        # GLSL vertex and fragment shaders
assets/
  textures/     # PNG/JPG texture files
  models/       # OBJ model files (if used)
docs/           # Proposal, report, notes
```

## Conventions
- Use GLM types: `glm::vec3`, `glm::vec4`, `glm::mat4` for all math
- Shader files: `shaders/vshader.glsl`, `shaders/fshader.glsl`
- All OpenGL objects (VAO, VBO, shaders) initialized in `init()`, drawn in `display()`
- Camera uses first-person view matrix: `glm::lookAt(eye, center, up)`
- Coordinate system: Y-up, 1 unit ≈ 1 meter

## Milestones
- March 9: Proposal submitted
- Late April: Progress presentation (basic scene + navigation + lighting)
- End of semester: Final (full campus, textures, Phong shading)

## External Data
See `docs/data-sources.md` for full instructions on acquiring:
- Campus building footprints (OpenStreetMap via overpass-turbo.eu)
- Building facade photos (take on campus)
- Material textures (polyhaven.com, CC0)
- Skybox (polyhaven.com HDRIs)

## Key Constraints (from syllabus)
- Must use C/C++ and OpenGL only — no Unity, no other rendering APIs
- Software implementation is mandatory
