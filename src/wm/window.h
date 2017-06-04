#ifndef WM_WINDOW_H_
#define WM_WINDOW_H_

#include <string>

namespace naive {
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
  Window* parent() { return parent_; }
 private:
  bool managed_;
  Window* parent_;
};

}  // namespace wm
}  // namespace naive

#endif //NAIVEWM_WINDOW_H
