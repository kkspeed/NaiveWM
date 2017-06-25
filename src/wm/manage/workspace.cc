#include "wm/manage/workspace.h"

#include <algorithm>
#include <cassert>
#include <deque>

namespace naive {
namespace wm {

namespace {

void ArrangeNonFloatingWindows(std::deque<ManageWindow*>& candidates,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               bool horizontal) {
  if (candidates.size() == 0)
    return;

  if (candidates.size() == 1) {
    candidates.front()->MoveResize(x, y, width, height);
    return;
  }

  ManageWindow* front = candidates.front();
  candidates.pop_front();
  if (horizontal) {
    front->MoveResize(x, y, width / 2, height);
    ArrangeNonFloatingWindows(candidates, x + width / 2, y, width / 2, height,
                              !horizontal);
  } else {
    front->MoveResize(x, y, width, height / 2);
    ArrangeNonFloatingWindows(candidates, x, y + height / 2, width, height / 2,
                              !horizontal);
  }
}

}  // namespace

ManageWindow::ManageWindow(Window* window, WMPrimitives* primitives)
    : window_(window),
      primitives_(primitives),
      is_floating_(window->is_transient() || window->is_popup()) {}

Workspace::Workspace(uint32_t tag) : tag_(tag) {}

void Workspace::AddWindow(std::unique_ptr<ManageWindow> window) {
  windows_.push_back(std::move(window));
  current_window_ = windows_.size() - 1;
}

ManageWindow* Workspace::CurrentWindow() {
  assert(current_window_ <= windows_.size());
  if (windows_.size() == 0)
    return nullptr;
  return windows_[current_window_].get();
}

std::unique_ptr<ManageWindow> Workspace::PopWindow(Window* window) {
  if (current_window_ >= windows_.size() - 1)
    current_window_ = 0;
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

void Workspace::ArrangeWindows(int32_t width, int32_t height) {
  std::vector<ManageWindow*> floating_windows;
  std::deque<ManageWindow*> normal_windows;

  for (auto& mw : windows_) {
    if (mw->window()->is_visible()) {
      if (mw->is_floating())
        floating_windows.push_back(mw.get());
      else
        normal_windows.push_back(mw.get());
    }
  }

  ArrangeNonFloatingWindows(normal_windows, 0, 0, width, height, true);

  for (auto* window : floating_windows)
    window->window()->Raise();
}

void Workspace::AddWindowToHead(std::unique_ptr<ManageWindow> window) {
  windows_.insert(windows_.begin(), std::move(window));
  current_window_ = 0;
}

bool Workspace::HasWindow(Window* window) {
  return std::find_if(windows_.begin(), windows_.end(), [window](auto& mw) {
           return mw->window() == window;
         }) != windows_.end();
}

}  // namespace wm
}  // namespace naive
