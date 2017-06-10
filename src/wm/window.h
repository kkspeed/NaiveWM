#ifndef WM_WINDOW_H_
#define WM_WINDOW_H_

#include <string>
#include <vector>
#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/surface.h"

namespace naive {

class ShellSurface;

namespace wm {

class Window : public SurfaceObserver {
 public:
  Window();

  bool IsManaged() const { return managed_; }
  void SetParent(Window* parent) { parent_ = parent; }
  void SetTransient(bool transient) { is_transient_ = transient; }
  void SetPopup(bool popup) { is_popup_ = popup; }
  void SetFullscreen(bool fullscreen);
  void SetMaximized(bool maximized);
  void SetTitle(std::string title) { title_ = title; }
  void SetClass(std::string clazz) { clazz_ = clazz; }
  void SetAppId(std::string app_id) { app_id_ = app_id; }

  // SurfaceObserver overrides
  void OnCommit() override;

  void SetSurface(Surface* surface) {
    surface_ = surface;
    surface_->AddSurfaceObserver(this);
  }

  void SetShellSurface(ShellSurface* shell_surface) {
    shell_surface_ = shell_surface;
  }

  void SetPosition(int32_t x, int32_t y) {
    pending_state_.geometry.x_ = x;
    pending_state_.geometry.y_ = y;
  }

  void SetGeometry(const base::geometry::Rect& rect) {
    pending_state_.geometry = rect;
  }

  void Resize(int32_t width, int32_t height);

  void AddChild(Window* child);
  void RemoveChild(Window* child);
  bool HasChild(const Window* child) const;
  void PlaceAbove(Window* window, Window* target);
  void PlaceBelow(Window* window, Window* target);
  void BeginMove() { /* TODO: implement this */
  }

  Window* parent() { return parent_; }

 private:
  bool managed_;

  struct WindowState {
    base::geometry::Rect geometry;
    WindowState() : geometry({0, 0, 0, 0}) {}
  };

  bool is_popup_, is_transient_;
  WindowState pending_state_, state_;
  std::string title_, clazz_, app_id_;
  std::vector<Window*> children_;
  Window* parent_;
  Surface* surface_;
  ShellSurface* shell_surface_;
};

}  // namespace wm
}  // namespace naive

#endif  // NAIVEWM_WINDOW_H
