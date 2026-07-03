# NinoPad App Designs

Full specs for 9 apps on ESP32 (no PSRAM) + ST7796 480×320 + XPT2046 + LVGL v9.

## Hardware Constraints

| Resource | Available | Notes |
|----------|-----------|-------|
| RAM | ~120KB heap | After 153KB display buffer + ~47KB static |
| Flash | ~60KB free | 95.4% full; prune widgets to reclaim ~30KB |
| Audio | None | MAX98357A pins TBD, not wired |
| Haptics | None | Motor not wired |
| Images | LVGL primitives only | No PNG decoder; draw with lines/arcs/fills |
| Fonts | Montserrat 12/14/16/24/28 | No room for 120px bitmap fonts |
| Screen mgmt | `lv_scr_act()` + `lv_obj_clean()` | No `lv_scr_load_anim` |

## Phase Plan

### Phase 0 — Shared Infrastructure
- Prune `lv_conf.h`: disable unused widgets to free flash
- `storage.cpp/h`: generic SD JSON read/write (ArduinoJson)
- `nino_drawing.h`: lightweight trace buffer (LVGL lines/points, not full `lv_canvas` — <5KB RAM)

### Phase 1 — Text/Grid/Button Apps (no canvas)
1. **Settings** — profile name entry, word list manager, progress dashboard, brightness/volume sliders
2. **Word Spy** — car mode (word auto-advance, "¡La vi!" button), picture match (2-choice word suns via LVGL primitives)
3. **10-Frame Sun** — drag suns into 2×5 grid, number decomposition, "Make 10"
4. **Story Time** — book parts quiz, word match quiz (drawing + audio deferred)

### Phase 2 — Tracing Apps (lightweight line-buffer trace)
5. **Luz Letters** — single letter trace, upper/lowercase, multi-stroke guidance, letter sounds (voice lines deferred)
6. **Snip Snip** — path tracing with accuracy scoring
7. **My Name** — name display + dotted-line guide for handwriting practice

### Phase 3 — Deferred (needs PSRAM or hardware)
8. **Shape Paint** — color-by-shape + free paint (canvas RAM too high)
9. **Counting Jar** — firefly animations (CPU heavy on no-PSRAM ESP32)
- All I2S audio (playback, recording)
- All haptic feedback
- PNG/SD card images (use LVGL-drawn primitives instead)

## App 1: My Name

- "My name is" + dashed input box (name from SD `settings.json`)
- Large drawing canvas with "DRAW HERE" placeholder
- Tracing mode: dotted guide of child's name, touch validation
  - Green/red dot feedback, haptic buzz on error (deferred), chime on completion (deferred)
- Free draw mode: 8-color palette, brush size, clear, undo (last 3 strokes)
- Name sentence tracing: "My name is [NAME]" dotted below canvas
- Success: star burst (LVGL animation), "¡Excelente!" label
- Save traced image to SD as raw bitmap

**LVGL elements:** `lv_label`, `lv_line` (guide paths), `lv_btn` (color picker, tool bar), `lv_canvas` (drawing — downsized to save RAM)

## App 2: Luz Letters

- Full-screen dotted letter (A-Z), centered, uppercase + lowercase toggle
- Navigation: arrow buttons to cycle letters
- Trace mode: dotted outline, start/end markers
  - Stroke-by-stroke for multi-stroke letters (A, B, E, F, H, I, K, M, N, R, T, X, Y)
  - Next stroke highlights after completion
  - Wrong start: green dot pulse "Start here!"
- Sound mode: tap letter to hear name (defer until I2S wired)
- Letter explorer: swipe through alphabet, fun facts
- Progress tracking: JSON — letters mastered, strokes completed

**LVGL elements:** `lv_canvas` (trace buffer), `lv_line` (dotted paths, start/end dots), `lv_btn` (arrows, toggle), `lv_label` (fun facts)

## App 3: Word Spy

**Car Mode:**
- Full-screen word, large font, auto-advance 15s
- "¡La vi!" button — child taps when they spot word
  - "Where?" prompt: 4 picture choices (store/car/sign/house)
  - Correct → star + next word
  - Words from SD `word_lists/pre_primer.json`
  - Shuffle + smart repeat (missed words come back sooner)

**Picture Match Mode:**
- Top: picture (drawn with LVGL primitives), bottom: 2 word choices in "sun" buttons
- Tap match → color fill, label confirmation
- Wrong: gentle shake animation, "Try again!"

**LVGL elements:** `lv_label`, `lv_btn` ("¡La vi!", choices, suns), `lv_timer` (auto-advance), `lv_anim` (shake, star burst)

## App 4: Counting Jar

- Mason jar graphic (LVGL-drawn), fireflies (LVGL circles with glow)
- Count mode: 1-10 fireflies, tap number button or write number
  - Correct: lid pops, fireflies escape
  - Number word below in dotted font
- Trace mode: dotted numbers 1-10 with words
- "I Know My Numbers" challenge — engagement prompts

**LVGL elements:** `lv_obj` (jar, fireflies as circles), `lv_anim` (float, glow, pop), `lv_btnmatrix` (numbers 1-10), `lv_label`, `lv_canvas` (number tracing — deferred to Phase 2)

## App 5: 10-Frame Sun

- Two 10-frame grids (2×5), suns fill cells
- "Show me 7!": drag suns from pool into grid, snap to cell
  - Filled: yellow sun, empty: gray outline
  - Submit: "7 = 5 + 2" visual decomposition
- Number match: 10-frame → drag correct number card (pool: 18, 20, 4, 15, 11, 12, 16, 9)
- Make 10: two frames, fill first to 10, see remainder

**LVGL elements:** `lv_obj` grid (2×5, styled borders), draggable suns (`lv_obj` with `LV_OBJ_FLAG_CLICKABLE` + event-driven drag), `lv_anim` (snap, glow), `lv_label`, `lv_btn`

## App 6: Shape Paint

- Canvas with scattered shapes (oval, circle, triangle, square, rectangle)
- Color by Shape: "Color the triangles BLUE!" — child taps shapes
  - Wrong shape: name + correct color hint
  - Progress bar, celebration at 5
- Shape key reference bar (shape → color mapping)
- Free Paint: drag shapes from palette, resize, rotate, recolor
- "How many sides?" quiz: shape → tap number (3, 4, 0)

**LVGL elements:** `lv_canvas` (drawing, deferred to Phase 3), `lv_obj` (shapes with radius/colors), `lv_btn` (palette, color picker), `lv_anim` (fill, dance, morph)

## App 7: Snip Snip

- Two cartoon faces bottom, dotted "hair" lines above
- Haircut tracing: scissors icon follows finger on zigzag/wavy lines
  - Accuracy scoring: green trail on-line, red off-line
  - 3/2/1 stars based on <10%/<25%/try again
  - Hair "falls" on completion
- Progression: Straight → Zigzag → Wave → Spiral → Shape outline → Custom
- Physical bridge: "Now try with real scissors!" after 3 digital levels
- Scissor grip tutorial (animated hand)

**LVGL elements:** `lv_line` (hair paths), `lv_canvas` (trail), `lv_img` (scissor icon), `lv_anim` (hair fall, face cheer), `lv_btn` (level select)

## App 8: Story Time

- Drawing canvas ("My Favorite Part") + record button (deferred)
- Book Parts Quiz: tap front cover, start reading, space between words, letter/word/sentence/punctuation
- "I Can Read Words": picture + 2 word sun choices
- Summer reading tracker: log books, earn badges

**LVGL elements:** `lv_canvas` (drawing, deferred), `lv_label` (instructions), `lv_btn` (sun choices, log book), `lv_anim` (badge award)

## App 9: Settings

- **Child profile:** Name entry (keyboard → SD `settings.json`)
- **Word list manager:** View pre-primer words, add custom, toggle on/off
- **Progress dashboard:** Read-only JSON summaries, stars, time per app, export CSV
- **System:** Brightness slider, volume slider (deferred), language (en/es), reset all

**LVGL elements:** `lv_keyboard`, `lv_list` + `lv_switch` (word toggle), `lv_slider`, `lv_dropdown`, `lv_msgbox`, `lv_tabview`
