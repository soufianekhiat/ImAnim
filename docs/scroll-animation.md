# Scroll Animation

Smooth animated scrolling for ImGui windows.

## Overview

ImAnim provides helpers for animating scroll position in ImGui windows. Instead of jumping instantly to a scroll target, the scroll animates smoothly with configurable easing.

## Basic Usage

```cpp
// Scroll current window to Y position
iam_scroll_to_y(500.0f, 0.3f);  // Scroll to Y=500 over 0.3 seconds

// Scroll to X position
iam_scroll_to_x(200.0f, 0.3f);

// Scroll to top
iam_scroll_to_top(0.3f);

// Scroll to bottom
iam_scroll_to_bottom(0.3f);
```

## With Custom Easing

```cpp
// Smooth scroll with custom easing
iam_scroll_to_y(
    target_y,
    0.5f,  // Duration
    iam_ease_preset(iam_ease_out_expo)
);

// Bouncy scroll
iam_scroll_to_y(
    target_y,
    0.6f,
    iam_ease_preset(iam_ease_out_back)
);

// Cubic bezier easing
iam_scroll_to_y(
    target_y,
    0.4f,
    iam_ease_bezier(0.25f, 0.1f, 0.25f, 1.0f)
);
```

## Example: Scroll to Item

```cpp
if (ImGui::Begin("List Window")) {
    for (int i = 0; i < 100; i++) {
        ImGui::PushID(i);

        if (ImGui::Selectable("Item", selected_item == i)) {
            selected_item = i;
        }

        // Scroll to newly selected item
        if (just_selected && i == selected_item) {
            float item_y = ImGui::GetCursorPosY();
            iam_scroll_to_y(item_y - 100.0f, 0.25f);  // Scroll with margin
        }

        ImGui::PopID();
    }
    ImGui::End();
}
```

## Example: Scroll on Button Click

```cpp
if (ImGui::Begin("Scrollable Content")) {
    // Navigation buttons
    if (ImGui::Button("Go to Top")) {
        iam_scroll_to_top(0.3f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to Bottom")) {
        iam_scroll_to_bottom(0.3f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to Section 3")) {
        iam_scroll_to_y(section3_y, 0.4f, iam_ease_preset(iam_ease_out_cubic));
    }

    ImGui::Separator();

    // Content
    for (int section = 0; section < 10; section++) {
        if (section == 3) {
            section3_y = ImGui::GetCursorPosY();
        }
        ImGui::Text("Section %d", section);
        ImGui::Dummy(ImVec2(0, 200));  // Section content
    }

    ImGui::End();
}
```

## Example: Back-to-Top Button

```cpp
if (ImGui::Begin("Long Document")) {
    // Track scroll position
    float scroll_y = ImGui::GetScrollY();
    float max_scroll = ImGui::GetScrollMaxY();

    // Content...
    for (int i = 0; i < 200; i++) {
        ImGui::Text("Line %d: Lorem ipsum dolor sit amet", i);
    }

    // Show back-to-top button when scrolled down
    if (scroll_y > 300.0f) {
        ImGui::SetCursorPos(ImVec2(
            ImGui::GetWindowWidth() - 50,
            ImGui::GetWindowHeight() - 50 + scroll_y
        ));

        if (ImGui::Button("^")) {
            iam_scroll_to_top(0.4f, iam_ease_preset(iam_ease_out_expo));
        }
    }

    ImGui::End();
}
```

## Example: Page-Based Navigation

```cpp
static int current_page = 0;
const int items_per_page = 20;
const float item_height = 25.0f;

if (ImGui::Begin("Paged List")) {
    // Page navigation
    if (ImGui::Button("< Prev") && current_page > 0) {
        current_page--;
        iam_scroll_to_y(
            current_page * items_per_page * item_height,
            0.3f,
            iam_ease_preset(iam_ease_out_cubic)
        );
    }
    ImGui::SameLine();
    if (ImGui::Button("Next >") && current_page < total_pages - 1) {
        current_page++;
        iam_scroll_to_y(
            current_page * items_per_page * item_height,
            0.3f,
            iam_ease_preset(iam_ease_out_cubic)
        );
    }

    ImGui::Separator();

    // List items
    for (int i = 0; i < total_items; i++) {
        ImGui::Text("Item %d", i);
    }

    ImGui::End();
}
```

## Example: Horizontal Scroll Gallery

```cpp
if (ImGui::Begin("Gallery")) {
    // Horizontal scroll container
    ImGui::BeginChild("gallery_scroll", ImVec2(0, 200), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    // Gallery items
    for (int i = 0; i < 20; i++) {
        if (i > 0) ImGui::SameLine();

        ImGui::PushID(i);
        if (ImGui::ImageButton("img", texture_ids[i], ImVec2(150, 150))) {
            // Center clicked item in view
            float item_x = i * 160.0f;
            float view_width = ImGui::GetWindowWidth();
            float center_x = item_x - (view_width - 150) / 2;
            iam_scroll_to_x(center_x, 0.3f);
        }
        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::End();
}
```

## Example: Anchor Links

```cpp
// Define anchors
std::map<std::string, float> anchors;

if (ImGui::Begin("Document")) {
    // Table of contents
    if (ImGui::CollapsingHeader("Contents", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Selectable("Introduction")) {
            iam_scroll_to_y(anchors["intro"], 0.4f);
        }
        if (ImGui::Selectable("Getting Started")) {
            iam_scroll_to_y(anchors["start"], 0.4f);
        }
        if (ImGui::Selectable("API Reference")) {
            iam_scroll_to_y(anchors["api"], 0.4f);
        }
    }

    ImGui::Separator();

    // Content with anchors
    anchors["intro"] = ImGui::GetCursorPosY();
    ImGui::TextWrapped("Introduction...");
    ImGui::Dummy(ImVec2(0, 300));

    anchors["start"] = ImGui::GetCursorPosY();
    ImGui::TextWrapped("Getting Started...");
    ImGui::Dummy(ImVec2(0, 400));

    anchors["api"] = ImGui::GetCursorPosY();
    ImGui::TextWrapped("API Reference...");
    ImGui::Dummy(ImVec2(0, 500));

    ImGui::End();
}
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_scroll_to_y(y, dur, ease)` | Scroll to Y position |
| `iam_scroll_to_x(x, dur, ease)` | Scroll to X position |
| `iam_scroll_to_top(dur, ease)` | Scroll to top |
| `iam_scroll_to_bottom(dur, ease)` | Scroll to bottom |

Default easing is `iam_ease_out_cubic` if not specified.

## See Also

- [Tweens](tweens.md) - Basic value animation
- [Easing](easing.md) - All easing functions
