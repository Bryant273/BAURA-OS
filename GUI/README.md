# BAURA GUI and BGE status

## Software components

- `BGE/renderer.*` and `renderer_core.*` provide a bounded ARGB8888 software
  canvas with clipping, alpha composition, rounded rectangles, gradients,
  bitmap text, and primitive icons. The renderer does not own or discover its
  framebuffer.
- `BGE/effects_engine.*` provides a bounded two-pass box blur and glass panels.
  Its running-window blur cost is linear in the selected region; regions are
  capped at 4,194,304 pixels.
- `BGE/gpu_accel_3d.*` is a CPU triangle rasterizer with a software depth
  buffer, not GPU acceleration.
- `BGE/window_manager.*` and `BGE/ui_framework.*` provide a software desktop
  shell, window stacking/focus, minimize/maximize/close hit targets, wallpaper,
  and the taskbar.
- `DISPLAY_SERVER/` owns bounded ARGB surfaces and composes them to a
  platform-supplied framebuffer. Its optional `present` callback is the handoff
  to a platform display backend.
- `COMPOSITOR/` clips and orders visible surfaces by z order.
- `WINDOW_MANAGER/` provides a C++ window/surface lifecycle, focus, state,
  resize, move, and injected input callbacks. It is distinct from the
  framebuffer shell in `BGE/`.
- `DESKTOP/`, `LOGIN_SCREEN/`, `UI_FRAMEWORK/`, and
  `BOOT/INSTALLER/installer_gui.*` provide the desktop widgets, login and
  installer presentation, and callback-driven interaction.
- `BAURA_GUI_FFI.h/.cpp` exposes surface, framebuffer, compositor, and window
  operations to C callers. Compositor handles retain their display server, so
  destroying the caller's display handle cannot leave a dangling compositor.

`make test-gui` builds and runs host-side checks for these software paths,
including framebuffer clipping, surface lifetime/resizing, window state,
window dragging, taskbar app activation/search/menu actions, login credential
callbacks, installer callbacks, and rasterizer input validation. It also
checks the callback-driven PS/2 decoder, RGBX/BGRX output conversion, and
registration/start/stop of the GUI runtime through the Core service manager.
Run this target after C++ compiler and Make are available in the environment.

## Platform integration still required

`PLATFORM/baura_gui_platform.*` provides a software renderer runtime for an
already-mapped 32-bit RGBX/BGRX framebuffer, with pitch/capacity validation and
an optional flush callback. `INPUT/ps2_driver.*` decodes basic PS/2 Set 2
keyboard and three-byte mouse packets through caller-provided port callbacks.
`PLATFORM/baura_gui_core_service.*` registers and starts/stops this runtime
through the Core service manager; the initial-task platform ABI has an optional,
size-gated GUI service hook. These are platform-injected software interfaces,
not UEFI/GOP discovery, physical memory mapping, or a seL4 I/O-port/IRQ driver.
The chosen seL4 initial-task contract must still authorize and supply the
framebuffer mapping and PS/2 callbacks (or provide another input backend), then
bind a `BauraGuiServiceBinding` to the Core hook. No integrated boot path
currently installs those resources. The application launcher/search backend is
also not connected. UI action callbacks remain optional and must be wired to
BAURA services by the platform. The login screen delegates credential
verification; the installer delegates disk operations. Neither performs
authentication, partitioning, formatting, or installation itself.

The Papirus catalog records icon identifiers and tint colors; it does not
include Papirus artwork. Current icons are software-drawn primitives. The
desktop and installer therefore render as host-testable UI components, not yet
as a booted, hardware-integrated BAURA desktop.
