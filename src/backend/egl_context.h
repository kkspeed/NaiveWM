#ifndef BACKEND_EGL_CONTEXT_H_
#define BACKEND_EGL_CONTEXT_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <cstdint>

namespace naive {
namespace backend {

class EglContext {
 public:
  EglContext(void* native_display, void* native_window);
  ~EglContext() = default;
  void CreateDrawBuffer(int32_t width, int32_t height);
  void BindDrawBuffer(bool bind);
  void BlitFrameBuffer();
  void MakeCurrent();
  void SwapBuffers();
  void EnableBlend(bool enable);

 private:
  // The buffer for content to be rendered on.
  int32_t display_width_, display_height_;
  GLuint framebuffer_;
  GLuint rendered_texture_;
};

}  // namespace backend
}  // namespace naive

#endif  // BACKEND_EGL_CONTEXT_H_
