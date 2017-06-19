#ifndef WM_WINDOW_H_
#define WM_WINDOW_H_

#include <cstdint>
#include <string>
#include <vector>
#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/shell_surface.h"
#include "compositor/surface.h"

namespace naive {

class ShellSurface;

namespace wm {

class Window : public SurfaceObserver {
 public:
  Window();
  ~Window();

  bool IsManaged() const { return managed_; }
  void set_parent(Window* parent) { parent_ = parent; }
  void set_transient(bool transient) { is_transient_ = transient; }
  void set_fullscreen(bool fullscreen);
  void set_maximized(bool maximized);
  void set_title(std::string title) { title_ = title; }
  void set_class(std::string clazz) { clazz_ = clazz; }
  void set_appid(std::string app_id) { app_id_ = app_id; }
  void set_popup(bool popup) { is_popup_ = popup; }

  // SurfaceObserver overrides
  void OnCommit() override;

  Surface* surface() { return surface_; }
  void set_surface(Surface* surface) {
    LOG_ERROR << "Set Surface " << surface << std::endl;
    surface_ = surface;
    surface_->AddSurfaceObserver(this);
  }

  void SetShellSurface(ShellSurface* shell_surface) {
    shell_surface_ = shell_surface;
  }

  void SetPosition(int32_t x, int32_t y) {
    TRACE("%d, %d", x, y);
    pending_state_.geometry.x_ = x;
    pending_state_.geometry.y_ = y;
    state_.geometry.x_ = x;
    state_.geometry.y_ = y;
  }

  void SetGeometry(const base::geometry::Rect& rect) {
    TRACE("geometry: %p: %d %d %d %d", this, rect.x(), rect.y(), rect.width(), rect.height());
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

  bool focused() { return focused_; }
  // Sets the window size via window manager.
  // TODO: This needs to commit as well.
  void WmSetSize(int32_t width, int32_t height);
  void WmSetPosition(int32_t x, int32_t y) {
    wm_x_ = x;
    wm_y_ = y;
    surface_->force_commit();
  }
  void LoseFocus();
  void TakeFocus();
  void Close();

  void set_managed(bool managed) { managed_ = managed; }
  std::vector<Window*>& children() { return children_; }
  Window* parent() { return parent_; }
  base::geometry::Rect geometry() { return state_.geometry; }
  int32_t wm_x() { return wm_x_; }
  int32_t wm_y() { return wm_y_; }
 private:
  bool managed_;
  bool focused_ = false;

  struct WindowState {
    base::geometry::Rect geometry;
    WindowState(): geometry({0, 0, 0, 0}) {}
  };

  bool is_popup_, is_transient_;
  WindowState pending_state_, state_;
  std::string title_, clazz_, app_id_;
  std::vector<Window*> children_;
  Window* parent_;
  Surface* surface_;
  ShellSurface* shell_surface_;
  int32_t wm_x_, wm_y_;
};

}  // namespace wm
}  // namespace naive

#endif  // NAIVEWM_WINDOW_H
