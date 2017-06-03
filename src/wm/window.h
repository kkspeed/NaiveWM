#ifndef WM_WINDOW_H_
#define WM_WINDOW_H_

namespace naive {
namespace wm {

class Window {
 public:
  bool IsManaged() const { return managed_; }
  void SetParent(Window* parent);
  Window* parent() { return parent_; }
 private:
  bool managed_;
  Window* parent_;
};

}  // namespace wm
}  // namespace naive

#endif //NAIVEWM_WINDOW_H
