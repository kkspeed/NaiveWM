#ifndef COMPOSITOR_TEXTUREDELEGATE_H_
#define COMPOSITOR_TEXTUREDELEGATE_H_

#include <memory>

namespace naive {
namespace compositor {

class TextureDelegate {
 public:
  virtual void Draw(int x,
                    int y,
                    int patch_x,
                    int patch_y,
                    int width,
                    int height) = 0;
  virtual ~TextureDelegate() = default;
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_TEXTURE_DELEGATE_
