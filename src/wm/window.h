#ifndef WM_WINDOW_H_
#define WM_WINDOW_H_

#include <string>
#include <vector>

namespace naive {

class Surface;

namespace wm {

class Window {
 public:
  bool IsManaged() const { return managed_; }
  void SetParent(Window* parent);
  void SetTransient(bool transient);
  void SetPopup(bool popup);
  void SetFullscreen(bool fullscreen);
  void SetMaximized(bool maximized);
  void SetTitle(std::string title);
  void SetClass(std::string clazz);
  void SetAppId(std::string app_id);

  void SetSurface(Surface* surface) { surface_ = surface; }
  void SetPosition(int32_t x, int32_t y) {
    x_ = x;
    y_ = y;
  }

  void AddChild(Window* child);
  void RemoveChild(Window* child);
  bool HasChild(const Window* child) const;
  void PlaceAbove(Window* window, Window* target);
  void PlaceBelow(Window* window, Window* target);

  Window* parent() { return parent_; }

 private:
  bool managed_;

  int32_t x_, y_;
  std::vector<Window*> children_;
  Window* parent_;
  Surface* surface_;
};

}  // namespace wm
}  // namespace naive

#endif  // NAIVEWM_WINDOW_H
