#include "wm/manage/workspace.h"

#include <algorithm>
#include <cassert>

namespace naive {
namespace wm {

ManageWindow::ManageWindow(Window* window, WMPrimitives* primitives)
  : window_(window), primitives_(primitives),
    is_floating_(window->is_transient() || window->is_popup()) {}

Workspace::Workspace(uint32_t tag): tag_(tag) {}

void Workspace::AddWindow(std::unique_ptr<ManageWindow> window) {
  windows_.push_back(std::move(window));
}

ManageWindow* Workspace::CurrentWindow() {
  assert(current_window_ <= windows_.size());
  if (windows_.size() == 0)
    return nullptr;
  return windows_[current_window_].get();
}

std::unique_ptr<ManageWindow> Workspace::PopWindow(Window* window) {
  auto it = std::find_if(windows_.begin(), windows_.end(),
                         [window](auto& mw) { return mw->window() == window; });
  if (it != windows_.end()) {
    while (it + 1 != windows_.end()) {
      std::iter_swap(it, it + 1);
      it++;
    }
    auto* mw = windows_.back().release();
    windows_.pop_back();
    return std::unique_ptr<ManageWindow>(mw);
  }
  if (current_window_ >= windows_.size())
    current_window_ = 0;
  return nullptr;
}

ManageWindow* Workspace::NextWindow() {
  if (windows_.size() == 0)
    return nullptr;
  size_t next_index = (current_window_ + 1) % windows_.size();
  current_window_ = next_index;
  return windows_[next_index].get();
}

ManageWindow* Workspace::PrevWindow() {
  if (windows_.size() == 0)
    return nullptr;
  size_t next_index = (current_window_ - 1 + windows_.size()) % windows_.size();
  current_window_ = next_index;
  return windows_[next_index].get();
}

void Workspace::Show(bool show) {
  for (auto& window : windows_)
    window->Show(show);
}

void Workspace::SetCurrentWindow(Window* window) {
  for (size_t i = 0; i < windows_.size(); i++) {
    if (windows_[i]->window() == window)
      current_window_ = i;
  }
}

void Workspace::ArrangeWindows() {
  // TODO: Tile
}

bool Workspace::HasWindow(Window* window) {
  return std::find_if(windows_.begin(), windows_.end(), [window](auto& mw) {
    return mw->window() == window;
  }) != windows_.end();
}

}  // namespace wm
}  // namespace naive
