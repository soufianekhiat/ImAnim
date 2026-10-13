# Clip Editor

Edit a clip's tracks and keyframes visually while it plays. Every edit applies
live to the running instances, and "Copy C++" turns the result back into an
`iam_clip::begin(...)...end()` chain you can paste into your code.

## Quick Start

```cpp
// Name ids so the editor (and exported code) shows names instead of hashes
static ImGuiID const CLIP  = iam_id("card_intro");
static ImGuiID const ALPHA = iam_id("alpha");
static ImGuiID const POS   = iam_id("pos");

iam_clip::begin(CLIP)
    .key_float(ALPHA, 0.0f, 0.0f, iam_ease_out_cubic)
    .key_float(ALPHA, 0.5f, 1.0f)
    .key_vec2(POS, 0.0f, iam_v2(0, 40), iam_ease_out_back)
    .key_vec2(POS, 0.6f, iam_v2(0, 0))
    .end();

ImGuiID inst = ImHashStr("card_intro_inst");
iam_play(CLIP, inst);

// Each frame: editor window driving your instance
static bool show_editor = true;
iam_show_clip_editor(CLIP, &show_editor, inst);

// ...or embedded in your own window
iam_clip_editor(CLIP, inst);
```

`preview_instance_id` is the instance the playhead scrubs and the toolbar plays.
Pass `0` and the editor plays its own instance.

Both functions return `true` on frames where the clip changed.

## Timeline

| Action | Effect |
|--------|--------|
| Click / drag the ruler | Scrub the preview instance (Shift: snap) |
| Click empty lane space | Move the playhead |
| Click a key | Select it (Ctrl/Shift: add/remove) |
| Drag on empty space | Box select (Ctrl/Shift: add) |
| Drag selected keys | Retime them (snaps to the step, hold Alt for free) |
| Double-click a lane | Add a key there, valued on the existing curve |
| Right-click a key | Set ease for the selection, delete |
| Right-click a track label | Add key at playhead, select all keys, delete track |
| Del / Backspace | Delete selected keys |
| Ctrl+A | Select all keys |
| Ctrl+Z, Ctrl+Y / Ctrl+Shift+Z | Undo, redo |
| Space | Play / pause (when the editor is hovered) |
| Ctrl+wheel | Zoom around the mouse |
| Shift+wheel, horizontal wheel, middle-drag | Pan |

Each segment shows its ease curve. Track labels show the current value of the
preview instance (a swatch for colors).

## Inspector

With one key selected: time, value (per type: float, vec2, vec4, int, color,
and percent/px bias for anchor-relative tracks), spring parameters (float
tracks), the color blend space, and the ease of the segment starting at the key,
edited on a graph with the [ease editor](easing.md#ease-editor) (presets, steps,
cubic-bezier handles, custom curves). Curve eases are exported by "Copy C++" and
saved in clip files. With several keys selected you can set the
ease for all of them or delete them.

## Toolbar

- **Play/Pause, Restart**: control the preview instance.
- **Loop** and direction, **Delay**.
- **Snap** and the snap step, **Fit** to show the whole clip.
- **Undo/Redo** (100 steps).
- **Add track**: name + type (float, vec2, vec4, int, color). The name is
  registered with `iam_id()`.
- **Copy C++**: puts the clip's code on the clipboard.
- **Save/Load**: `iam_clip_save()` / `iam_clip_load()` with the given path.

## Named IDs

```cpp
ImGuiID iam_id(char const* name);                    // ImHashStr(name), remembers the name
void iam_set_id_name(ImGuiID id, char const* name);  // name an existing id
char const* iam_get_id_name(ImGuiID id);             // nullptr when unnamed
```

The editor uses names for the window title, track labels, default save path,
and in the exported code (`iam_id("alpha")`). Unnamed ids are shown and
exported as hex (`0x1A2B3C4Du`).

## Notes

- Keys with per-loop variation are kept and retimed, but their variation is not
  editable in the editor nor exported.
- Callbacks and markers are not touched by the editor.
- Edits change the clip itself: if your code calls `iam_clip::begin()` on the
  same id again (e.g. every frame), it overwrites the edits. Build clips once.
