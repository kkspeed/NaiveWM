#include "wm/window_impl.h"

namespace naive {
namespace wm {

compositor::TextureDelegate* WindowImpl::CachedTexture() {
  return cached_texture_.get();
}

void WindowImpl::CacheTexture(
    std::unique_ptr<compositor::TextureDelegate> texture) {
  cached_texture_ = std::move(texture);
}

}  // namespace wm
}  // namespace naive
