# Particles

`iam_particles()` is a small immediate-mode emitter: call it every frame with
the emitter position; it simulates once per frame and draws into a draw list.

```cpp
iam_particle_opts o;
o.rate = 90.0f;                         // particles per second (0 = bursts only)
o.lifetime = 1.4f;
o.velocity = iam_v2(0, -250);           // base direction and speed (px/s)
o.spread = 0.5f;                        // random cone (radians)
o.gravity = iam_v2(0, 350);
o.size_start = 3.0f; o.size_end = 0.5f;
o.color_start = IM_COL32(120, 210, 240, 255);
o.color_end   = IM_COL32(91, 120, 231, 0);   // fade out
o.shape = iam_particle_spark;           // iam_particle_shape: circle, square or spark (oriented along velocity)
iam_particles(ImGui::GetID("fountain"), dl, emitter_pos, dt, o);

if (ImGui::IsItemClicked()) iam_particles_burst(ImGui::GetID("confetti"), 80);   // one-off burst
int live = iam_particles_count(ImGui::GetID("fountain"));
```

| Option | Default | |
|--------|---------|-|
| `rate`, `lifetime`, `lifetime_var` | 30, 1.0s, 0.2 | Emission and life (random +/- fraction) |
| `velocity`, `spread`, `speed_var` | (0,-80), 0.6, 0.3 | Launch direction/speed and randomness |
| `gravity`, `drag` | (0,0), 0 | Forces |
| `size_start`, `size_end`, `size_var` | 4, 1, 0.2 | Radius over life |
| `color_start`, `color_end` | white, transparent white | Blended in OKLAB over life |
| `shape`, `max_particles`, `seed` | `iam_particle_circle`, 512, 0 | `iam_particle_circle`, `_square`, `_spark` |

Pass a null draw list to simulate without drawing. Reduced motion
(`iam_set_reduced_motion`) scales emission by its procedural scale. Emitters not
used for a while are collected by `iam_gc()`.
