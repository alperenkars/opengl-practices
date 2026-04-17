from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_ALIGN_VERTICAL
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

doc = Document()

# --- Page margins ---
for section in doc.sections:
    section.top_margin    = Cm(2.5)
    section.bottom_margin = Cm(2.5)
    section.left_margin   = Cm(3.0)
    section.right_margin  = Cm(2.5)

# ============================================================
# Helpers
# ============================================================
def add_paragraph(text="", bold=False, italic=False, size=12,
                  align=WD_ALIGN_PARAGRAPH.LEFT, space_before=0, space_after=6):
    p = doc.add_paragraph()
    p.alignment = align
    p.paragraph_format.space_before = Pt(space_before)
    p.paragraph_format.space_after  = Pt(space_after)
    if text:
        run = p.add_run(text)
        run.font.name  = "Times New Roman"
        run.font.size  = Pt(size)
        run.font.bold  = bold
        run.font.italic = italic
        run.font.color.rgb = RGBColor(0, 0, 0)
    return p

def add_heading(text, level=1):
    sizes   = {1: 14, 2: 13, 3: 12}
    befores = {1: 14, 2: 10, 3: 8}
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.LEFT
    p.paragraph_format.space_before = Pt(befores.get(level, 10))
    p.paragraph_format.space_after  = Pt(4)
    run = p.add_run(text)
    run.font.name  = "Times New Roman"
    run.font.size  = Pt(sizes.get(level, 12))
    run.font.bold  = True
    run.font.color.rgb = RGBColor(0, 0, 0)
    return p

def add_separator():
    p = doc.add_paragraph()
    p.paragraph_format.space_before = Pt(2)
    p.paragraph_format.space_after  = Pt(2)
    pPr = p._p.get_or_add_pPr()
    pBdr = OxmlElement('w:pBdr')
    bottom = OxmlElement('w:bottom')
    bottom.set(qn('w:val'), 'single')
    bottom.set(qn('w:sz'), '4')
    bottom.set(qn('w:space'), '1')
    bottom.set(qn('w:color'), '000000')
    pBdr.append(bottom)
    pPr.append(pBdr)

def add_bullet(text, bold_prefix=None):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.space_before = Pt(1)
    p.paragraph_format.space_after  = Pt(1)
    p.paragraph_format.left_indent  = Cm(0.75)
    if bold_prefix:
        r = p.add_run(bold_prefix + " ")
        r.font.name = "Times New Roman"; r.font.size = Pt(12)
        r.font.bold = True; r.font.color.rgb = RGBColor(0, 0, 0)
    r2 = p.add_run(text)
    r2.font.name = "Times New Roman"; r2.font.size = Pt(12)
    r2.font.color.rgb = RGBColor(0, 0, 0)

def add_code(code_text):
    """Insert a monospaced code block with a light-grey shaded paragraph."""
    p = doc.add_paragraph()
    p.paragraph_format.space_before = Pt(4)
    p.paragraph_format.space_after  = Pt(6)
    p.paragraph_format.left_indent  = Cm(0.4)
    # shade the paragraph
    pPr = p._p.get_or_add_pPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), 'F2F2F2')
    pPr.append(shd)
    # add a thin border
    pBdr = OxmlElement('w:pBdr')
    for side in ('top', 'left', 'bottom', 'right'):
        b = OxmlElement(f'w:{side}')
        b.set(qn('w:val'), 'single')
        b.set(qn('w:sz'), '4')
        b.set(qn('w:space'), '4')
        b.set(qn('w:color'), 'BFBFBF')
        pBdr.append(b)
    pPr.append(pBdr)

    run = p.add_run(code_text)
    run.font.name = "Consolas"
    # set East Asian font as well to keep monospace consistently
    rPr = run._r.get_or_add_rPr()
    rFonts = OxmlElement('w:rFonts')
    rFonts.set(qn('w:ascii'), 'Consolas')
    rFonts.set(qn('w:hAnsi'), 'Consolas')
    rFonts.set(qn('w:cs'), 'Consolas')
    rPr.append(rFonts)
    run.font.size = Pt(9.5)
    run.font.color.rgb = RGBColor(0, 0, 0)

def add_caption(text):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(2)
    p.paragraph_format.space_after  = Pt(8)
    r = p.add_run(text)
    r.font.name = "Times New Roman"; r.font.size = Pt(10); r.font.italic = True
    r.font.color.rgb = RGBColor(80, 80, 80)

def add_screenshot_placeholder(label):
    """A bordered, empty box where the user can later paste a screenshot."""
    table = doc.add_table(rows=1, cols=1)
    table.alignment = WD_ALIGN_PARAGRAPH.CENTER
    cell = table.cell(0, 0)
    cell.width = Cm(14)
    cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
    # Add some empty lines so the box is visibly tall
    for _ in range(7):
        p = cell.add_paragraph()
        p.paragraph_format.space_before = Pt(0)
        p.paragraph_format.space_after  = Pt(0)
    centre = cell.paragraphs[3]
    centre.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = centre.add_run("[ Screenshot placeholder — paste image here ]")
    r.font.name = "Times New Roman"; r.font.size = Pt(11); r.font.italic = True
    r.font.color.rgb = RGBColor(120, 120, 120)
    # light grey shading
    tc = cell._tc
    tcPr = tc.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), 'FAFAFA')
    tcPr.append(shd)
    add_caption(label)

def style_table(table, headers):
    for i, cell in enumerate(table.rows[0].cells):
        cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        p = cell.paragraphs[0]
        p.clear()
        run = p.add_run(headers[i])
        run.font.name  = "Times New Roman"
        run.font.size  = Pt(11)
        run.font.bold  = True
        run.font.color.rgb = RGBColor(0, 0, 0)
        p.paragraph_format.space_before = Pt(2)
        p.paragraph_format.space_after  = Pt(2)
        tc = cell._tc
        tcPr = tc.get_or_add_tcPr()
        shd = OxmlElement('w:shd')
        shd.set(qn('w:val'), 'clear')
        shd.set(qn('w:color'), 'auto')
        shd.set(qn('w:fill'), 'D9D9D9')
        tcPr.append(shd)

def fill_table_row(row, values):
    for i, cell in enumerate(row.cells):
        cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
        p = cell.paragraphs[0]
        p.clear()
        run = p.add_run(values[i])
        run.font.name  = "Times New Roman"
        run.font.size  = Pt(11)
        run.font.color.rgb = RGBColor(0, 0, 0)
        p.paragraph_format.space_before = Pt(2)
        p.paragraph_format.space_after  = Pt(2)

# ============================================================
# TITLE BLOCK
# ============================================================
p = doc.add_paragraph()
p.alignment = WD_ALIGN_PARAGRAPH.CENTER
p.paragraph_format.space_before = Pt(0)
p.paragraph_format.space_after  = Pt(4)
r = p.add_run("Progress Report")
r.font.name = "Times New Roman"; r.font.size = Pt(16); r.font.bold = True
r.font.color.rgb = RGBColor(0, 0, 0)

p2 = doc.add_paragraph()
p2.alignment = WD_ALIGN_PARAGRAPH.CENTER
p2.paragraph_format.space_after = Pt(2)
r2 = p2.add_run("COMP410 Computer Graphics — Spring 2026")
r2.font.name = "Times New Roman"; r2.font.size = Pt(12); r2.font.italic = True
r2.font.color.rgb = RGBColor(0, 0, 0)

for line in [
    ("Project Title:", "Virtual Koç University Campus Tour"),
    ("Team Members:", "Alperen Kars, Uğur Öner"),
    ("Instructor:", "Yücel Yemez"),
]:
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(1)
    p.paragraph_format.space_after  = Pt(1)
    r_label = p.add_run(line[0] + " ")
    r_label.font.name = "Times New Roman"; r_label.font.size = Pt(12)
    r_label.font.bold = True; r_label.font.color.rgb = RGBColor(0, 0, 0)
    r_val = p.add_run(line[1])
    r_val.font.name = "Times New Roman"; r_val.font.size = Pt(12)
    r_val.font.color.rgb = RGBColor(0, 0, 0)

add_separator()

# ============================================================
# 1. INTRODUCTION
# ============================================================
add_heading("1. Introduction")
add_paragraph(
    "This report describes the current state of our COMP410 term project, the Virtual Koç "
    "University Campus Tour. The goal of the project is to let a user walk through a faithful "
    "3D reconstruction of the KU campus from a first-person point of view, using only C++ and "
    "shader-based OpenGL. The application opens in a window, the mouse controls where the "
    "camera is looking, and the WASD keys move the player around the campus. The user can also "
    "switch into a free-fly camera by pressing F to inspect the campus from the air.",
    space_before=4, space_after=6
)
add_paragraph(
    "At this stage of the semester we have a working build that loads the full campus mesh, "
    "renders it with per-fragment Phong lighting, supports textured surfaces from the model "
    "file, follows the terrain when the user walks around, and prevents the camera from "
    "passing through buildings. The rest of this report explains how each part is built, with "
    "small code excerpts so the implementation choices are easy to follow.",
    space_after=6
)

add_screenshot_placeholder("Figure 1 — Overall view of the campus from the spawn point.")

# ============================================================
# 2. TECH STACK
# ============================================================
add_heading("2. Tech Stack")
add_paragraph(
    "We deliberately kept the dependency list short. Everything below is either part of the "
    "standard course toolchain or a small, well-known helper library. The project builds with "
    "a single Makefile and runs from the command line.",
    space_before=4, space_after=6
)

ts_headers = ["Component", "Version / Notes", "Role in the project"]
ts_rows = [
    ["Language",          "C++17",                 "Core implementation language."],
    ["Graphics API",      "OpenGL 4.1 core (macOS max)", "All rendering goes through the programmable pipeline."],
    ["Shading language",  "GLSL 410",              "Vertex and fragment shaders (one program)."],
    ["Math",              "GLM",                   "Vectors, matrices, projections, LookAt."],
    ["Windowing & input", "GLFW",                  "Window, mouse, keyboard, framebuffer events."],
    ["GL extension loader","GLEW",                 "Loads OpenGL 4.x function pointers."],
    ["Model loading",     "Assimp",                "Imports the campus from .glb / .gltf / .obj."],
    ["Image loading",     "stb_image (single header)", "Decodes embedded and external textures."],
    ["Build system",      "GNU Make",              "make builds the binary named campus."],
    ["Asset pipeline",    "Blender + OpenStreetMap", "Footprints from OSM, exported as glTF/GLB."],
]
t1 = doc.add_table(rows=1 + len(ts_rows), cols=3)
t1.style = 'Table Grid'
style_table(t1, ts_headers)
for i, row_data in enumerate(ts_rows):
    fill_table_row(t1.rows[i + 1], row_data)

add_paragraph(
    "The build command is the standard one defined in the Makefile, and the binary is run "
    "directly from the project root:",
    space_before=8, space_after=4
)
add_code(
    "make            # builds ./campus\n"
    "./campus        # opens the window and starts the tour"
)

# ============================================================
# 3. PROJECT STRUCTURE
# ============================================================
add_heading("3. Project Structure")
add_paragraph(
    "The repository is intentionally flat so it stays readable. Source files live under src/, "
    "shaders are plain GLSL files under shaders/, and all geometry, textures and the campus "
    "model file live under assets/.",
    space_before=4, space_after=6
)
add_code(
    "graphcode/\n"
    "├── Makefile\n"
    "├── shaders/\n"
    "│   ├── vshader.glsl     # vertex shader (transforms + varyings)\n"
    "│   └── fshader.glsl     # fragment shader (Phong + texture + gamma)\n"
    "├── src/\n"
    "│   ├── main.cpp         # window, init, display, input loop\n"
    "│   ├── camera.h         # first-person + fly camera\n"
    "│   ├── mesh.h / mesh.cpp # Assimp-based model loader\n"
    "│   ├── shader.h         # tiny shader-program loader\n"
    "│   ├── texture.h / texture.cpp # stb_image-based texture loader\n"
    "│   ├── stb_image.h\n"
    "│   └── cgltf.h\n"
    "└── assets/\n"
    "    ├── models/Campus.glb   # the exported campus scene\n"
    "    └── textures/           # facade and ground textures"
)

# ============================================================
# 4. CURRENT PROGRESS
# ============================================================
add_heading("4. Current Progress")
add_paragraph(
    "The list below summarises everything that is already implemented and visible when running "
    "the application. The remaining items belong to the final-submission phase and are listed "
    "at the end of the report.",
    space_before=4, space_after=4
)
for item in [
    "Window and OpenGL 4.1 core context (GLFW + GLEW).",
    "Loading the full KU campus from a .glb file via Assimp, including embedded textures.",
    "Per-fragment Phong shading (ambient + diffuse + Blinn-Phong specular) in GLSL.",
    "Texture sampling with mipmaps and anisotropic filtering on textured meshes.",
    "Gamma-correct lighting (sRGB → linear on input, linear → sRGB on output).",
    "First-person walk camera with mouse look and WASD movement.",
    "Free-fly camera mode (toggle with F, ascend/descend with Space/C).",
    "Sprint and turbo speed modifiers (Shift / Alt).",
    "Terrain following: the camera’s Y position is sampled from the walkable mesh.",
    "Building collision: the camera cannot enter axis-aligned bounding boxes of buildings.",
    "Automatic spawn placement: the player always starts on a walkable tile near the centre.",
    "Per-mesh natural colour palette so untextured buildings still look believable.",
    "Sky-coloured background that matches the gamma-corrected scene output.",
]:
    add_bullet(item)

add_screenshot_placeholder("Figure 2 — First-person walk mode, terrain following + building collision.")
add_screenshot_placeholder("Figure 3 — Fly mode (press F), used to inspect the campus from above.")

# ============================================================
# 5. RENDERING PIPELINE — HIGH LEVEL
# ============================================================
add_heading("5. Rendering Pipeline")
add_paragraph(
    "The renderer is intentionally a single forward pass: one shader program, one draw call per "
    "mesh. We picked this design because the campus model is a static scene, and the bottleneck "
    "is geometry/texture upload rather than draw-call count. The high-level flow per frame is:",
    space_before=4, space_after=4
)
for item in [
    "Compute deltaTime, then poll keyboard/mouse input and update the camera.",
    "Clear the color and depth buffers using a sky-blue clear color.",
    "Bind the shader program and upload per-frame uniforms (view, projection, light, viewPos).",
    "If the loaded scene has no terrain, draw a fallback green ground quad so we never see void.",
    "Iterate over every mesh of the campus model; bind its texture (or a 1×1 white fallback), "
    "set its base colour, bind its VAO, and issue a single glDrawElements call.",
    "Swap the back buffer to the screen.",
]:
    add_bullet(item)

add_paragraph("The actual render loop body lives in display() in src/main.cpp:",
              space_before=6, space_after=4)
add_code(
    "void display()\n"
    "{\n"
    "    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);\n"
    "    glUseProgram(shaderProgram);\n"
    "\n"
    "    const glm::mat4 view = camera.viewMatrix();\n"
    "    const glm::mat4 projection = glm::perspective(\n"
    "        glm::radians(60.0f),\n"
    "        (float)WIDTH / (float)HEIGHT,\n"
    "        0.1f, farPlane);\n"
    "\n"
    "    setUniform(shaderProgram, \"view\", view);\n"
    "    setUniform(shaderProgram, \"projection\", projection);\n"
    "    setUniform(shaderProgram, \"viewPos\", camera.position);\n"
    "    setUniform(shaderProgram, \"lightDir\",\n"
    "               glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f)));\n"
    "    setUniform(shaderProgram, \"lightColor\", glm::vec3(1.0f, 0.98f, 0.95f));\n"
    "\n"
    "    setUniform(shaderProgram, \"model\", sceneRootTransform);\n"
    "    for (const auto& mesh : campusModel.meshes) {\n"
    "        glBindTexture(GL_TEXTURE_2D,\n"
    "                      mesh.diffuseTex ? mesh.diffuseTex : fallbackWhiteTex);\n"
    "        setUniform(shaderProgram, \"hasTexture\", mesh.diffuseTex ? 1 : 0);\n"
    "        setUniform(shaderProgram, \"objectColor\", mesh.baseColor);\n"
    "        glBindVertexArray(mesh.vao);\n"
    "        glDrawElements(GL_TRIANGLES, mesh.indexCount,\n"
    "                       GL_UNSIGNED_INT, 0);\n"
    "    }\n"
    "}"
)

# ============================================================
# 6. HOW WE RENDER OBJECTS
# ============================================================
add_heading("6. How We Render Objects")

add_heading("6.1 Vertex format and GPU buffers", level=2)
add_paragraph(
    "Every triangle that ends up on screen comes from a Vertex struct that holds a position, a "
    "normal, and a 2D texture coordinate. Each mesh owns three OpenGL objects: a Vertex Array "
    "Object (VAO) that remembers the attribute layout, a Vertex Buffer Object (VBO) for the "
    "vertex data, and an Element Buffer Object (EBO) for the index list.",
    space_before=4, space_after=4
)
add_code(
    "struct Vertex {\n"
    "    glm::vec3 pos;\n"
    "    glm::vec3 normal;\n"
    "    glm::vec2 uv;\n"
    "};\n\n"
    "struct Mesh {\n"
    "    GLuint vao = 0, vbo = 0, ebo = 0;\n"
    "    GLsizei indexCount = 0;\n"
    "    GLuint diffuseTex = 0;        // 0 = no texture, fall back to colour\n"
    "    glm::vec3 baseColor = glm::vec3(1.0f);\n"
    "};"
)
add_paragraph(
    "When a mesh is built we upload the vertex/index buffers once and describe the layout with "
    "glVertexAttribPointer. The location indices match the layout(location = …) declarations in "
    "the vertex shader, so the GPU knows that location 0 is position, 1 is normal, and 2 is UV.",
    space_before=4, space_after=4
)
add_code(
    "glBindVertexArray(mesh.vao);\n"
    "glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);\n"
    "glBufferData(GL_ARRAY_BUFFER,\n"
    "             vertices.size() * sizeof(Vertex),\n"
    "             vertices.data(), GL_STATIC_DRAW);\n\n"
    "glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);\n"
    "glBufferData(GL_ELEMENT_ARRAY_BUFFER,\n"
    "             indices.size() * sizeof(unsigned int),\n"
    "             indices.data(), GL_STATIC_DRAW);\n\n"
    "glEnableVertexAttribArray(0);  // pos\n"
    "glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,\n"
    "                      sizeof(Vertex), (void*)offsetof(Vertex, pos));\n"
    "glEnableVertexAttribArray(1);  // normal\n"
    "glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,\n"
    "                      sizeof(Vertex), (void*)offsetof(Vertex, normal));\n"
    "glEnableVertexAttribArray(2);  // uv\n"
    "glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,\n"
    "                      sizeof(Vertex), (void*)offsetof(Vertex, uv));"
)

add_heading("6.2 Loading the campus model with Assimp", level=2)
add_paragraph(
    "The campus geometry is too large to write by hand. We model it in Blender (using building "
    "footprints from OpenStreetMap as a starting point) and export it as a single Campus.glb "
    "file. Inside the program we load that file using Assimp and walk its node hierarchy. For "
    "every node we accumulate the world transform, then bake it into the vertex positions so "
    "that the renderer itself only has to deal with one global model matrix.",
    space_before=4, space_after=4
)
add_code(
    "const aiScene* scene = importer.ReadFile(path,\n"
    "    aiProcess_Triangulate |\n"
    "    aiProcess_GenSmoothNormals |\n"
    "    aiProcess_JoinIdenticalVertices);\n\n"
    "// recursive node walk: world = parent * node->mTransformation\n"
    "for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {\n"
    "    const aiVector3D p = worldTransform * ai_mesh->mVertices[i];\n"
    "    v.pos    = glm::vec3(p.x, p.y, p.z);\n"
    "    v.normal = ...; // transformed by inverse-transpose\n"
    "    v.uv     = (ai_mesh->HasTextureCoords(0))\n"
    "                 ? glm::vec2(ai_mesh->mTextureCoords[0][i].x,\n"
    "                             ai_mesh->mTextureCoords[0][i].y)\n"
    "                 : glm::vec2(0.0f);\n"
    "}"
)
add_paragraph(
    "While iterating, we also classify each mesh by its name. Meshes whose names contain "
    "“terrain”, “road”, “path”, or “pedestrian” are added to a walkable surface list. Meshes "
    "containing “building” become axis-aligned obstacles for collision. Helper meshes such as "
    "envelopes and bounding boxes (used in the OSM export) are skipped entirely:",
    space_before=4, space_after=4
)
add_code(
    "if (isWalkSurfaceName(name))   model.walkSurface.push_back({a.pos, b.pos, c.pos});\n"
    "if (isBuildingName(name))      model.obstacles.push_back({minXZ, maxXZ, minY, maxY});\n"
    "if (shouldSkipRenderableMesh(name)) continue;   // never drawn"
)

add_heading("6.3 Materials and textures", level=2)
add_paragraph(
    "Each mesh receives its material from Assimp. We first pick the diffuse / base-colour, then "
    "look for a base-colour or diffuse texture. If the texture is embedded inside the .glb (the "
    "key starts with ‘*’) we decode it from memory; otherwise we resolve a path next to the "
    "model file. All textures are uploaded with mipmaps and the highest anisotropy the driver "
    "supports, which matters a lot for the ground when viewed at grazing angles.",
    space_before=4, space_after=4
)
add_code(
    "glGenerateMipmap(GL_TEXTURE_2D);\n"
    "glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);\n"
    "glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);\n\n"
    "GLfloat maxAniso = 0.0f;\n"
    "glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);\n"
    "if (maxAniso > 1.0f)\n"
    "    glTexParameterf(GL_TEXTURE_2D,\n"
    "                    GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);"
)
add_paragraph(
    "If a mesh has no texture at all, we still want it to look reasonable. A small palette of "
    "natural facade and roof colours is hashed by the building index from the mesh name, so "
    "neighbouring buildings get different but believable colours. This avoids the "
    "“everything is the same grey” look you get with a single fallback colour.",
    space_before=4, space_after=4
)
add_screenshot_placeholder(
    "Figure 4 — Close-up of a building showing the natural facade palette and Phong shading."
)

add_heading("6.4 Vertex shader: model → view → clip space", level=2)
add_paragraph(
    "The vertex shader is short on purpose: it transforms the position into world space (so we "
    "can compute lighting in world space), transforms the normal with the inverse-transpose of "
    "the model matrix (so non-uniform scales still produce correct normals), forwards the UV, "
    "and emits the final clip-space position.",
    space_before=4, space_after=4
)
add_code(
    "#version 410 core\n"
    "layout(location = 0) in vec3 vPosition;\n"
    "layout(location = 1) in vec3 vNormal;\n"
    "layout(location = 2) in vec2 vUV;\n\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n\n"
    "out vec3 fragPos;\n"
    "out vec3 fragNormal;\n"
    "out vec2 fragUV;\n\n"
    "void main()\n"
    "{\n"
    "    vec4 worldPos = model * vec4(vPosition, 1.0);\n"
    "    fragPos    = worldPos.xyz;\n"
    "    fragNormal = mat3(transpose(inverse(model))) * vNormal;\n"
    "    fragUV     = vUV;\n"
    "    gl_Position = projection * view * worldPos;\n"
    "}"
)

add_heading("6.5 Fragment shader: Phong + texture + gamma", level=2)
add_paragraph(
    "The fragment shader implements per-fragment Phong lighting with a Blinn-Phong specular "
    "term. There is one directional sun light. The base colour is either the per-mesh colour "
    "or that colour multiplied by the texture sample. Texture samples are linearised (raised to "
    "the 2.2 power) before lighting because PNG/JPEG textures are stored in sRGB, and mixing "
    "sRGB values directly produces incorrect highlights. The final colour is gamma-corrected on "
    "the way out so the monitor displays it correctly.",
    space_before=4, space_after=4
)
add_code(
    "#version 410 core\n"
    "in vec3 fragPos;\n"
    "in vec3 fragNormal;\n"
    "in vec2 fragUV;\n\n"
    "uniform vec3      objectColor;\n"
    "uniform vec3      lightDir;     // direction TO the light\n"
    "uniform vec3      lightColor;\n"
    "uniform vec3      viewPos;\n"
    "uniform sampler2D diffuseMap;\n"
    "uniform bool      hasTexture;\n\n"
    "out vec4 fColor;\n\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(fragNormal);\n\n"
    "    vec3 ambient  = 0.20 * lightColor;\n"
    "    vec3 diffuse  = max(dot(N, lightDir), 0.0) * lightColor;\n\n"
    "    vec3 V = normalize(viewPos - fragPos);\n"
    "    vec3 H = normalize(lightDir + V);\n"
    "    float specStrength = hasTexture ? 0.08 : 0.30;\n"
    "    float shininess    = hasTexture ? 16.0 : 64.0;\n"
    "    vec3 specular = specStrength *\n"
    "                    pow(max(dot(N, H), 0.0), shininess) * lightColor;\n\n"
    "    vec3 baseColor = objectColor;\n"
    "    if (hasTexture) {\n"
    "        vec3 texel = texture(diffuseMap, fragUV).rgb;\n"
    "        baseColor *= pow(texel, vec3(2.2));   // sRGB -> linear\n"
    "    }\n\n"
    "    vec3 result = (ambient + diffuse + specular) * baseColor;\n"
    "    result = pow(result, vec3(1.0 / 2.2));    // linear -> sRGB\n"
    "    fColor = vec4(result, 1.0);\n"
    "}"
)
add_screenshot_placeholder("Figure 5 — Same building before / after enabling gamma-correct lighting.")

# ============================================================
# 7. CAMERA AND INPUT
# ============================================================
add_heading("7. Camera and Input")
add_paragraph(
    "The camera is a small struct that stores a position, a yaw/pitch pair, and a movement "
    "speed. The mouse callback updates yaw and pitch (clamped to ±89°), and front()/right() "
    "are derived from yaw/pitch. The view matrix is just glm::lookAt(position, position + "
    "front(), up).",
    space_before=4, space_after=4
)
add_code(
    "glm::vec3 front() const {\n"
    "    glm::vec3 f;\n"
    "    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));\n"
    "    f.y = sin(glm::radians(pitch));\n"
    "    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));\n"
    "    return glm::normalize(f);\n"
    "}\n\n"
    "glm::mat4 viewMatrix() const {\n"
    "    return glm::lookAt(position, position + front(),\n"
    "                       glm::vec3(0.0f, 1.0f, 0.0f));\n"
    "}"
)
add_paragraph(
    "Movement is split into two modes that share the same input handler. In walk mode, the "
    "movement vector is flattened to the XZ plane so the player cannot fly by looking up; in "
    "fly mode (toggled by F) the movement uses the full 3D camera direction and ignores both "
    "terrain and collision. Holding Shift gives a sprint speed; Alt gives a turbo speed used "
    "mostly for testing the far side of the campus.",
    space_before=4, space_after=4
)

# ============================================================
# 8. TERRAIN FOLLOWING & COLLISION
# ============================================================
add_heading("8. Terrain Following and Building Collision")
add_paragraph(
    "When the user is in walk mode, two things must hold every frame: (1) the camera’s feet "
    "stay on the ground, and (2) the player cannot push the camera through a building. We "
    "solve both in pure CPU code, without any physics library.",
    space_before=4, space_after=6
)

add_heading("8.1 Terrain height from a triangle list", level=2)
add_paragraph(
    "While loading the model, we collect every triangle that belongs to a walkable mesh into a "
    "flat list. To find the ground height under a position (x, z), we test the (x, z) point "
    "against each triangle’s 2D bounding box, then compute its barycentric coordinates inside "
    "the triangle and interpolate the Y of the three vertices. This gives us a smooth height "
    "value that follows hills and stairs. If multiple triangles overlap (for example, a path "
    "passing under a bridge), we either pick the highest one below the camera or the one "
    "closest to the camera’s previous Y, depending on context.",
    space_before=4, space_after=4
)
add_code(
    "const float det = v0.x * v1.y - v1.x * v0.y;\n"
    "const float u   = (v2.x * v1.y - v1.x * v2.y) / det;\n"
    "const float v   = (v0.x * v2.y - v2.x * v0.y) / det;\n"
    "const float w   = 1.0f - u - v;\n"
    "if (u >= 0 && v >= 0 && w >= 0)\n"
    "    outY = u * tri.b.y + v * tri.c.y + w * tri.a.y;"
)
add_paragraph(
    "After sampling the terrain we add 1.7 m for the player’s eye height. The result is the "
    "camera’s new Y. Because the test is per-triangle and the campus has thousands of "
    "triangles, sampling is cheap enough to do every frame; we never noticed it in the frame "
    "time.",
    space_before=4, space_after=4
)

add_heading("8.2 Building obstacles (axis-aligned boxes)", level=2)
add_paragraph(
    "For each building mesh we also store an axis-aligned bounding box (minXZ, maxXZ, minY, "
    "maxY). Before applying a movement, the candidate position is checked against every box "
    "with a small body radius (≈60 cm). If the full XZ move would land inside a building, we "
    "first try sliding along X only, then along Z only — this gives the natural “slide along "
    "the wall” feel instead of the player getting glued to the wall. If the player is somehow "
    "stuck inside an obstacle (rare, but happens after collision-edge cases), a search "
    "radiating outward from the previous position relocates them to the nearest walkable tile.",
    space_before=4, space_after=4
)
add_code(
    "if (projectToWalkable(prevPos + move, corrected)) {\n"
    "    resolvedPos = corrected;                       // full move ok\n"
    "} else {\n"
    "    if (projectToWalkable(prevPos + glm::vec3(move.x,0,0), corrected))\n"
    "        resolvedPos = corrected;                   // slide on X\n"
    "    if (projectToWalkable(prevPos + glm::vec3(0,0,move.z), corrected))\n"
    "        resolvedPos = corrected;                   // slide on Z\n"
    "}"
)

add_heading("8.3 Spawn placement", level=2)
add_paragraph(
    "On startup we try to place the camera at the centre of the scene’s bounding box, but only "
    "if that point is on a walkable triangle and not inside a building. If it isn’t, we sweep "
    "outward in concentric rings until we find a tile that satisfies both conditions. This "
    "guarantees the player never spawns mid-air or inside a wall, even when we re-export the "
    "campus and the centre of the bounding box shifts.",
    space_before=4, space_after=4
)

add_screenshot_placeholder("Figure 6 — Player walking along a path between two buildings; collision keeps them on the path.")

# ============================================================
# 9. ASSET PIPELINE
# ============================================================
add_heading("9. Asset Pipeline")
add_paragraph(
    "Modelling the entire KU campus by hand would be unrealistic, so we built a small pipeline "
    "instead. Building footprints come from OpenStreetMap, exported via overpass-turbo.eu as "
    "GeoJSON. Inside Blender we extrude these footprints to approximate building heights and "
    "place a few terrain and path meshes underneath them. Naming conventions matter here: any "
    "mesh whose name contains “terrain”, “road”, or “path” will be treated as walkable in the "
    "C++ side, and any mesh that contains “building” becomes a collision obstacle. The whole "
    "scene is exported as a single Campus.glb file with embedded textures so the runtime only "
    "has to load one file.",
    space_before=4, space_after=4
)
add_paragraph(
    "Material textures are CC0 assets from polyhaven.com (concrete, asphalt, grass). The "
    "skybox HDRI for Phase 2 will also come from there.",
    space_before=2, space_after=4
)

# ============================================================
# 10. CHALLENGES & DECISIONS
# ============================================================
add_heading("10. Notable Challenges and Design Decisions")

add_heading("10.1 Why bake transforms at load time", level=2)
add_paragraph(
    "Assimp gives us a node hierarchy with per-node transforms. We could keep that hierarchy "
    "and multiply the matrices each frame, but the campus is fully static — once it is loaded "
    "nothing moves except the camera. Multiplying parent transforms into the vertex positions "
    "during loading turns the runtime cost into a single uniform upload per frame and makes "
    "the per-mesh draw loop trivial.",
    space_before=4, space_after=4
)

add_heading("10.2 Why we treat the unit conversion explicitly", level=2)
add_paragraph(
    "Some Blender exports come out in centimetres, others in metres. After computing the "
    "scene’s bounding box, if the largest extent is bigger than 2 km we apply a 0.01 root "
    "scale, which is roughly the centimetres-to-metres assumption. Without this check, a "
    "freshly re-exported model would silently push the camera 100× away from the geometry and "
    "the screen would be empty.",
    space_before=4, space_after=4
)
add_code(
    "if (maxExtent > 2000.0f) {\n"
    "    sceneRootTransform = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));\n"
    "    sceneScale = 0.01f;\n"
    "}"
)

add_heading("10.3 Why hashing names for facade colours", level=2)
add_paragraph(
    "Many OSM-derived meshes have no UV map and no texture; with a single fallback colour, the "
    "campus looked completely flat. Hashing the OSM building index from the mesh name into a "
    "small palette of natural facade colours (creams, beiges, light greys) made neighbouring "
    "buildings distinguishable at almost zero cost, while still looking realistic.",
    space_before=4, space_after=4
)

add_heading("10.4 Why one shader program", level=2)
add_paragraph(
    "We considered separate shaders for textured vs untextured geometry, but a single shader "
    "with a hasTexture uniform turned out to be simpler and avoids a state change per material "
    "type. The branch inside the fragment shader is uniform across a draw call, so the GPU "
    "executes only one side and there is no measurable cost.",
    space_before=4, space_after=4
)

# ============================================================
# 11. NEXT STEPS
# ============================================================
add_heading("11. Next Steps Toward the Final Submission")
for item in [
    "Skybox: a cube-map HDRI sky drawn behind everything else, replacing the flat sky-blue clear color.",
    "Improved facade textures on key buildings (Library, Sci-Eng, CASE, Law, Sports Center, Student Center).",
    "Simple day/night light direction toggle, reusing the existing directional-light uniform.",
    "Optional shadow mapping for the sun light (stretch goal).",
    "On-screen HUD: building labels and a small minimap of the campus.",
    "Performance pass: frustum culling so we skip meshes outside the view.",
]:
    add_bullet(item)

add_screenshot_placeholder("Figure 7 — Screenshot reserved for the final submission demo.")

# ============================================================
# 12. CONCLUSION
# ============================================================
add_heading("12. Conclusion")
add_paragraph(
    "At the midpoint of the semester the project already delivers the core experience we set "
    "out to build: a user can launch a single binary, find themselves standing inside a "
    "recognisable 3D KU campus, look around with the mouse, walk between buildings, and fly "
    "above the scene to see the layout from the sky. Nearly every major COMP410 topic — "
    "shader programming, transformation pipeline, perspective viewing, Phong lighting, "
    "texture mapping, buffer management, and interaction — is already exercised by the code. "
    "The remaining work is mostly about polish (skybox, more textures) and a few quality-of-"
    "life features rather than new fundamentals.",
    space_before=4, space_after=6
)

# ============================================================
out = "/Users/alperen.kars/graphcode/docs/progress_report.docx"
doc.save(out)
print(f"Saved: {out}")
