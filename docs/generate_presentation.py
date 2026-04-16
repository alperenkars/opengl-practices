#!/usr/bin/env python3
"""Generate mid-project progress presentation for KU Campus Tour."""

from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE

# ── Palette ──────────────────────────────────────────────────────────────
WHITE      = RGBColor(0xFF, 0xFF, 0xFF)
OFF_WHITE  = RGBColor(0xF5, 0xF5, 0xF5)
DARK       = RGBColor(0x1A, 0x1A, 0x2E)
ACCENT     = RGBColor(0x16, 0x54, 0x7E)  # KU-ish blue
LIGHT_GRAY = RGBColor(0xBB, 0xBB, 0xBB)
MID_GRAY   = RGBColor(0x66, 0x66, 0x66)
GREEN_OK   = RGBColor(0x27, 0xAE, 0x60)
ORANGE_WIP = RGBColor(0xE6, 0x7E, 0x22)
LIGHT_BG   = RGBColor(0xF0, 0xF4, 0xF8)
CARD_BG    = RGBColor(0xE8, 0xEE, 0xF4)
HIGHLIGHT  = RGBColor(0x1E, 0x90, 0xFF)

prs = Presentation()
prs.slide_width  = Inches(13.333)
prs.slide_height = Inches(7.5)

W = prs.slide_width
H = prs.slide_height


# ── Helpers ──────────────────────────────────────────────────────────────

def solid_bg(slide, color):
    bg = slide.background
    fill = bg.fill
    fill.solid()
    fill.fore_color.rgb = color


def add_text(slide, left, top, width, height, text, size=18, bold=False,
             color=DARK, align=PP_ALIGN.LEFT, font_name="Calibri"):
    txBox = slide.shapes.add_textbox(left, top, width, height)
    txBox.word_wrap = True
    tf = txBox.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = text
    p.font.size = Pt(size)
    p.font.bold = bold
    p.font.color.rgb = color
    p.font.name = font_name
    p.alignment = align
    return txBox


def add_multiline(slide, left, top, width, height, lines, size=16,
                  color=DARK, line_spacing=1.5, bullet=False, font_name="Calibri"):
    """lines: list of str or list of (str, color)."""
    txBox = slide.shapes.add_textbox(left, top, width, height)
    txBox.word_wrap = True
    tf = txBox.text_frame
    tf.word_wrap = True
    for i, item in enumerate(lines):
        if isinstance(item, tuple):
            txt, clr = item
        else:
            txt, clr = item, color
        if i == 0:
            p = tf.paragraphs[0]
        else:
            p = tf.add_paragraph()
        if bullet:
            p.text = txt
        else:
            p.text = txt
        p.font.size = Pt(size)
        p.font.color.rgb = clr
        p.font.name = font_name
        p.space_after = Pt(size * (line_spacing - 1))
    return txBox


def add_rect(slide, left, top, width, height, fill_color, border_color=None):
    shape = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, left, top, width, height)
    shape.fill.solid()
    shape.fill.fore_color.rgb = fill_color
    if border_color:
        shape.line.color.rgb = border_color
        shape.line.width = Pt(1)
    else:
        shape.line.fill.background()
    # Smaller corner radius
    shape.adjustments[0] = 0.05
    return shape


def add_circle(slide, left, top, size, fill_color):
    shape = slide.shapes.add_shape(MSO_SHAPE.OVAL, left, top, size, size)
    shape.fill.solid()
    shape.fill.fore_color.rgb = fill_color
    shape.line.fill.background()
    return shape


def add_line(slide, x1, y1, x2, y2, color=LIGHT_GRAY, width=1):
    connector = slide.shapes.add_connector(1, x1, y1, x2, y2)  # 1 = straight
    connector.line.color.rgb = color
    connector.line.width = Pt(width)
    return connector


def placeholder_img(slide, left, top, width, height, label="Screenshot"):
    """Draw a placeholder rectangle with dashed-look border and label."""
    shape = add_rect(slide, left, top, width, height, OFF_WHITE, LIGHT_GRAY)
    # Add label in center
    txBox = slide.shapes.add_textbox(left, top + height // 2 - Inches(0.3),
                                     width, Inches(0.6))
    tf = txBox.text_frame
    p = tf.paragraphs[0]
    p.text = label
    p.font.size = Pt(14)
    p.font.color.rgb = LIGHT_GRAY
    p.font.italic = True
    p.alignment = PP_ALIGN.CENTER
    return shape


# =====================================================================
# SLIDE 1 — Title
# =====================================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])  # blank
solid_bg(slide, DARK)

# Accent bar at top
add_rect(slide, Inches(0), Inches(0), W, Inches(0.08), ACCENT)

# Title
add_text(slide, Inches(1.5), Inches(1.8), Inches(10), Inches(1.2),
         "Virtual Ko\u00e7 University Campus Tour",
         size=42, bold=True, color=WHITE, align=PP_ALIGN.CENTER)

# Subtitle
add_text(slide, Inches(1.5), Inches(3.1), Inches(10), Inches(0.6),
         "Mid-Project Progress Presentation",
         size=22, color=LIGHT_GRAY, align=PP_ALIGN.CENTER)

# Divider line
add_line(slide, Inches(5), Inches(4.0), Inches(8.3), Inches(4.0), ACCENT, 2)

# Team & course
add_text(slide, Inches(1.5), Inches(4.4), Inches(10), Inches(0.5),
         "Alperen Kars  &  U\u011fur \u00d6ner",
         size=20, color=WHITE, align=PP_ALIGN.CENTER)

add_text(slide, Inches(1.5), Inches(5.0), Inches(10), Inches(0.5),
         "COMP 410 \u2014 Computer Graphics  |  Spring 2026  |  Y\u00fccel Yemez",
         size=16, color=LIGHT_GRAY, align=PP_ALIGN.CENTER)

# Bottom accent bar
add_rect(slide, Inches(0), H - Inches(0.08), W, Inches(0.08), ACCENT)


# =====================================================================
# SLIDE 2 — Motivation (visual: campus map placeholder + 3 icon cards)
# =====================================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
solid_bg(slide, WHITE)
add_rect(slide, Inches(0), Inches(0), W, Inches(0.06), ACCENT)

add_text(slide, Inches(0.8), Inches(0.4), Inches(6), Inches(0.7),
         "Motivation", size=32, bold=True, color=DARK)

# Thin underline
add_line(slide, Inches(0.8), Inches(1.05), Inches(2.8), Inches(1.05), ACCENT, 2)

# Left side: campus placeholder image
placeholder_img(slide, Inches(0.8), Inches(1.5), Inches(5.5), Inches(5.2),
                "[ Ko\u00e7 University aerial photo ]")

# Right side: 3 motivation cards
card_x = Inches(7.0)
card_w = Inches(5.5)
card_h = Inches(1.4)
cards = [
    ("\U0001F3D7  Interactive 3D Exploration",
     "Navigate a real-scale campus model\nin first-person view"),
    ("\U0001F4CD  Real Geographic Data",
     "Building footprints sourced from\nOpenStreetMap"),
    ("\U0001F4A1  Learn Core Graphics",
     "Shaders, lighting, textures, camera\u2014\nbuilt from scratch in OpenGL"),
]

for i, (title, desc) in enumerate(cards):
    y = Inches(1.5) + i * Inches(1.7)
    add_rect(slide, card_x, y, card_w, card_h, CARD_BG)
    add_text(slide, card_x + Inches(0.3), y + Inches(0.15), card_w - Inches(0.5), Inches(0.4),
             title, size=16, bold=True, color=ACCENT)
    add_text(slide, card_x + Inches(0.3), y + Inches(0.6), card_w - Inches(0.5), Inches(0.7),
             desc, size=14, color=MID_GRAY)

add_rect(slide, Inches(0), H - Inches(0.06), W, Inches(0.06), ACCENT)


# =====================================================================
# SLIDE 3 — Technical Approach (pipeline diagram style)
# =====================================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
solid_bg(slide, WHITE)
add_rect(slide, Inches(0), Inches(0), W, Inches(0.06), ACCENT)

add_text(slide, Inches(0.8), Inches(0.4), Inches(6), Inches(0.7),
         "Technical Approach", size=32, bold=True, color=DARK)
add_line(slide, Inches(0.8), Inches(1.05), Inches(3.8), Inches(1.05), ACCENT, 2)

# Pipeline boxes
pipeline_items = [
    ("OSM Data", "Building footprints\n& positions"),
    ("C++ / OpenGL 4.1", "Core rendering\nengine"),
    ("GLSL 410", "Vertex & fragment\nshaders"),
    ("Phong Shading", "Ambient + diffuse\n+ specular"),
    ("First-Person\nCamera", "WASD + mouse\nnavigation"),
]

box_w = Inches(2.0)
box_h = Inches(1.6)
start_x = Inches(0.8)
y_top = Inches(1.8)
gap = Inches(0.55)

for i, (title, desc) in enumerate(pipeline_items):
    x = start_x + i * (box_w + gap)
    # Box
    add_rect(slide, x, y_top, box_w, box_h, CARD_BG)
    add_text(slide, x, y_top + Inches(0.2), box_w, Inches(0.5),
             title, size=15, bold=True, color=ACCENT, align=PP_ALIGN.CENTER)
    add_text(slide, x + Inches(0.15), y_top + Inches(0.7), box_w - Inches(0.3), Inches(0.8),
             desc, size=12, color=MID_GRAY, align=PP_ALIGN.CENTER)
    # Arrow between boxes
    if i < len(pipeline_items) - 1:
        ax = x + box_w
        ay = y_top + box_h / 2
        add_text(slide, ax, ay - Inches(0.15), Inches(gap), Inches(0.3),
                 "\u25B6", size=16, color=ACCENT, align=PP_ALIGN.CENTER)

# Bottom section: key numbers
y_nums = Inches(4.2)
add_line(slide, Inches(0.8), y_nums, Inches(12.5), y_nums, LIGHT_GRAY, 1)

nums = [
    ("12", "Buildings"),
    ("410", "GLSL Version"),
    ("~430", "Lines of C++"),
    ("Blinn-Phong", "Shading Model"),
    ("60 FPS", "Target"),
]

num_w = Inches(2.2)
for i, (big, label) in enumerate(nums):
    x = Inches(0.8) + i * num_w + Inches(0.2)
    add_text(slide, x, y_nums + Inches(0.3), Inches(1.8), Inches(0.7),
             big, size=28, bold=True, color=ACCENT, align=PP_ALIGN.CENTER)
    add_text(slide, x, y_nums + Inches(1.0), Inches(1.8), Inches(0.4),
             label, size=13, color=MID_GRAY, align=PP_ALIGN.CENTER)

# Libraries row
y_libs = Inches(5.8)
add_text(slide, Inches(0.8), y_libs, Inches(1.5), Inches(0.4),
         "Libraries:", size=14, bold=True, color=DARK)

libs = ["GLFW", "GLEW", "GLM", "stb_image"]
for i, lib in enumerate(libs):
    x = Inches(2.5) + i * Inches(1.8)
    add_rect(slide, x, y_libs - Inches(0.05), Inches(1.4), Inches(0.45), ACCENT)
    add_text(slide, x, y_libs, Inches(1.4), Inches(0.35),
             lib, size=13, bold=True, color=WHITE, align=PP_ALIGN.CENTER)

add_rect(slide, Inches(0), H - Inches(0.06), W, Inches(0.06), ACCENT)


# =====================================================================
# SLIDE 4 — Current Progress (screenshot placeholders + check marks)
# =====================================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
solid_bg(slide, WHITE)
add_rect(slide, Inches(0), Inches(0), W, Inches(0.06), ACCENT)

add_text(slide, Inches(0.8), Inches(0.4), Inches(6), Inches(0.7),
         "Current Progress", size=32, bold=True, color=DARK)
add_line(slide, Inches(0.8), Inches(1.05), Inches(3.5), Inches(1.05), ACCENT, 2)

# Two screenshot placeholders side by side
placeholder_img(slide, Inches(0.8), Inches(1.5), Inches(5.5), Inches(3.5),
                "[ Screenshot: campus overview ]")
placeholder_img(slide, Inches(6.8), Inches(1.5), Inches(5.5), Inches(3.5),
                "[ Screenshot: close-up view ]")

# Feature checklist at bottom
y_feat = Inches(5.4)
features = [
    ("\u2713  12 buildings from real OSM data", GREEN_OK),
    ("\u2713  First-person WASD + mouse navigation", GREEN_OK),
    ("\u2713  Phong shading with directional sunlight", GREEN_OK),
    ("\u2713  Ground plane & sky background", GREEN_OK),
]

for i, (txt, clr) in enumerate(features):
    col = i % 2
    row = i // 2
    x = Inches(0.8) + col * Inches(6.0)
    y = y_feat + row * Inches(0.55)
    add_text(slide, x, y, Inches(5.5), Inches(0.45),
             txt, size=16, bold=False, color=clr)

add_rect(slide, Inches(0), H - Inches(0.06), W, Inches(0.06), ACCENT)


# =====================================================================
# SLIDE 5 — Roadmap / Remaining Work (timeline style)
# =====================================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
solid_bg(slide, WHITE)
add_rect(slide, Inches(0), Inches(0), W, Inches(0.06), ACCENT)

add_text(slide, Inches(0.8), Inches(0.4), Inches(6), Inches(0.7),
         "Roadmap", size=32, bold=True, color=DARK)
add_line(slide, Inches(0.8), Inches(1.05), Inches(2.5), Inches(1.05), ACCENT, 2)

# Timeline: horizontal line with milestones
timeline_y = Inches(2.5)
add_line(slide, Inches(1.5), timeline_y, Inches(11.8), timeline_y, ACCENT, 3)

milestones = [
    ("Mar 9", "Proposal\nSubmitted", GREEN_OK, True),
    ("Apr 13", "Mid Progress\n(today)", ACCENT, True),
    ("Late Apr", "Textures &\nSkybox", ORANGE_WIP, False),
    ("Mid May", "Collision &\nLabels", ORANGE_WIP, False),
    ("End May", "Final Demo\n& Report", ORANGE_WIP, False),
]

for i, (date, desc, color, done) in enumerate(milestones):
    x = Inches(1.5) + i * Inches(2.5)
    # Circle on timeline
    c_size = Inches(0.3)
    add_circle(slide, x - c_size // 2, timeline_y - c_size // 2, c_size, color)
    # Date above
    add_text(slide, x - Inches(0.7), timeline_y - Inches(0.8), Inches(1.4), Inches(0.4),
             date, size=13, bold=True, color=color, align=PP_ALIGN.CENTER)
    # Description below
    add_text(slide, x - Inches(0.8), timeline_y + Inches(0.35), Inches(1.6), Inches(0.7),
             desc, size=13, color=MID_GRAY if not done else color, align=PP_ALIGN.CENTER)

# Remaining work cards (2 rows x 3 cols)
remaining = [
    ("Texture Mapping", "Facade photos on\nbuilding walls"),
    ("Skybox", "HDR environment\ncubemap"),
    ("Collision Detection", "AABB bounds\nper building"),
    ("Building Labels", "On-screen text\nidentifiers"),
    ("Improved Geometry", "L-shapes, roofs,\ndetailed footprints"),
    ("Paths & Roads", "Walkways between\nbuildings"),
]

card_w = Inches(3.5)
card_h = Inches(1.2)
start_x = Inches(0.8)
start_y = Inches(4.0)

for i, (title, desc) in enumerate(remaining):
    col = i % 3
    row = i // 3
    x = start_x + col * (card_w + Inches(0.4))
    y = start_y + row * (card_h + Inches(0.3))
    add_rect(slide, x, y, card_w, card_h, CARD_BG)
    # Orange dot indicator
    add_circle(slide, x + Inches(0.2), y + Inches(0.25), Inches(0.15), ORANGE_WIP)
    add_text(slide, x + Inches(0.5), y + Inches(0.15), card_w - Inches(0.6), Inches(0.35),
             title, size=14, bold=True, color=DARK)
    add_text(slide, x + Inches(0.5), y + Inches(0.55), card_w - Inches(0.6), Inches(0.6),
             desc, size=12, color=MID_GRAY)

add_rect(slide, Inches(0), H - Inches(0.06), W, Inches(0.06), ACCENT)


# =====================================================================
# SLIDE 6 — Demo / Thank You
# =====================================================================
slide = prs.slides.add_slide(prs.slide_layouts[6])
solid_bg(slide, DARK)
add_rect(slide, Inches(0), Inches(0), W, Inches(0.08), ACCENT)

add_text(slide, Inches(1.5), Inches(2.0), Inches(10), Inches(1.0),
         "Live Demo", size=44, bold=True, color=WHITE, align=PP_ALIGN.CENTER)

add_text(slide, Inches(1.5), Inches(3.2), Inches(10), Inches(0.6),
         "First-person walkthrough of the campus scene",
         size=20, color=LIGHT_GRAY, align=PP_ALIGN.CENTER)

# Divider
add_line(slide, Inches(5), Inches(4.3), Inches(8.3), Inches(4.3), ACCENT, 2)

add_text(slide, Inches(1.5), Inches(4.8), Inches(10), Inches(0.5),
         "Thank you", size=24, color=LIGHT_GRAY, align=PP_ALIGN.CENTER)

add_text(slide, Inches(1.5), Inches(5.5), Inches(10), Inches(0.5),
         "Alperen Kars  &  U\u011fur \u00d6ner  |  COMP 410",
         size=16, color=LIGHT_GRAY, align=PP_ALIGN.CENTER)

add_rect(slide, Inches(0), H - Inches(0.08), W, Inches(0.08), ACCENT)


# ── Save ─────────────────────────────────────────────────────────────────
output = "docs/mid_progress_presentation.pptx"
prs.save(output)
print(f"Saved: {output}")
