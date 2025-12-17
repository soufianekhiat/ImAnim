# Drag Feedback

Animated feedback for drag operations with snapping support.

## Overview

Drag feedback provides smooth animated responses when users drag UI elements. Features include:
- Velocity tracking for momentum effects
- Grid and point snapping with animated transitions
- Overshoot/bounce effects on release

## Basic Usage

```cpp
static ImVec2 widget_pos(100, 100);
static bool dragging = false;

ImGui::SetCursorPos(widget_pos);
ImGui::InvisibleButton("drag_widget", ImVec2(80, 80));

ImGuiID drag_id = ImGui::GetItemID();
float dt = ImGui::GetIO().DeltaTime;

if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
    if (!dragging) {
        // Start drag
        iam_drag_begin(drag_id, widget_pos);
        dragging = true;
    }
    // Update during drag
    ImVec2 mouse_pos = ImGui::GetMousePos();
    iam_drag_feedback fb = iam_drag_update(drag_id, mouse_pos, dt);
    widget_pos = fb.position;
}
else if (dragging) {
    // Release with snap animation
    iam_drag_opts opts;
    opts.snap_grid = ImVec2(50, 50);  // Snap to 50px grid
    opts.snap_duration = 0.2f;
    opts.ease_type = iam_ease_out_back;

    iam_drag_feedback fb = iam_drag_release(drag_id, widget_pos, opts, dt);
    widget_pos = fb.position;

    if (!fb.is_snapping) {
        dragging = false;
    }
}
```

## Drag Options

```cpp
struct iam_drag_opts {
    ImVec2 snap_grid;           // Grid size for snapping (0,0 = no grid)
    ImVec2* snap_points;        // Array of custom snap points
    int snap_points_count;      // Number of snap points
    float snap_duration;        // Duration of snap animation (default: 0.2)
    float overshoot;            // Overshoot amount (0 = none, 1 = normal)
    int ease_type;              // Easing type for snap (default: out_cubic)
};
```

## Drag Feedback State

```cpp
struct iam_drag_feedback {
    ImVec2 position;            // Current animated position
    ImVec2 offset;              // Offset from drag start
    ImVec2 velocity;            // Current velocity estimate
    bool is_dragging;           // Currently being dragged
    bool is_snapping;           // Currently snapping to target
    float snap_progress;        // Snap animation progress (0-1)
};
```

## Grid Snapping

```cpp
iam_drag_opts opts;
opts.snap_grid = ImVec2(32, 32);      // 32px grid
opts.snap_duration = 0.15f;
opts.overshoot = 0.3f;                 // Slight bounce
opts.ease_type = iam_ease_out_back;
```

## Custom Snap Points

```cpp
ImVec2 snap_targets[] = {
    ImVec2(100, 100),
    ImVec2(200, 100),
    ImVec2(150, 200),
    ImVec2(100, 300),
};

iam_drag_opts opts;
opts.snap_points = snap_targets;
opts.snap_points_count = 4;
opts.snap_duration = 0.25f;
```

## Cancel Drag

```cpp
// Cancel drag tracking (e.g., on Escape key)
if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    iam_drag_cancel(drag_id);
    dragging = false;
    widget_pos = original_pos;  // Restore original position
}
```

## Momentum-Based Release

Use velocity for throw/fling effects:

```cpp
if (dragging && !ImGui::IsMouseDown(0)) {
    iam_drag_feedback fb = iam_drag_release(drag_id, widget_pos, opts, dt);

    // Apply momentum decay
    if (fb.velocity.x != 0 || fb.velocity.y != 0) {
        widget_pos.x += fb.velocity.x * dt * 0.5f;
        widget_pos.y += fb.velocity.y * dt * 0.5f;
    }
}
```

## Example: Draggable Card

```cpp
void DraggableCard(int card_index, ImVec2* pos) {
    ImGui::PushID(card_index);

    static bool dragging = false;
    ImGuiID drag_id = ImGui::GetID("card");
    float dt = ImGui::GetIO().DeltaTime;

    // Draw card at position
    ImGui::SetCursorScreenPos(*pos);
    ImGui::BeginGroup();
    ImGui::Button("Card", ImVec2(120, 80));
    ImGui::EndGroup();

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    if (active && ImGui::IsMouseDragging(0)) {
        if (!dragging) {
            iam_drag_begin(drag_id, *pos);
            dragging = true;
        }
        iam_drag_feedback fb = iam_drag_update(drag_id, ImGui::GetMousePos(), dt);
        *pos = fb.position;
    }
    else if (dragging) {
        iam_drag_opts opts;
        opts.snap_grid = ImVec2(130, 90);  // Card size + margin
        opts.snap_duration = 0.2f;
        opts.overshoot = 0.2f;
        opts.ease_type = iam_ease_out_back;

        iam_drag_feedback fb = iam_drag_release(drag_id, *pos, opts, dt);
        *pos = fb.position;

        if (!fb.is_snapping) {
            dragging = false;
        }
    }

    ImGui::PopID();
}
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_drag_begin(id, pos)` | Start tracking drag at position |
| `iam_drag_update(id, pos, dt)` | Update drag position during drag |
| `iam_drag_release(id, pos, opts, dt)` | Release drag with animated feedback |
| `iam_drag_cancel(id)` | Cancel drag tracking |

## See Also

- [Tweens](tweens.md) - Basic value animation
- [Transforms](transforms.md) - 2D transform animation
