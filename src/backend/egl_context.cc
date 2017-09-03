#include "backend/egl_context.h"

#include <cassert>
#include <cstdint>

#include "base/logging.h"

namespace naive {
namespace backend {

namespace {

struct {
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
} gl;

}  // namespace

EglContext::EglContext(void* native_display, void* native_window) {
  EGLint major, minor, n;

  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  const EGLint config_attributes[] = {EGL_SURFACE_TYPE,
                                      EGL_WINDOW_BIT,
                                      EGL_RED_SIZE,
                                      1,
                                      EGL_GREEN_SIZE,
                                      1,
                                      EGL_BLUE_SIZE,
                                      1,
                                      EGL_ALPHA_SIZE,
                                      1,
                                      EGL_RENDERABLE_TYPE,
                                      EGL_OPENGL_ES3_BIT,
                                      EGL_NONE};

  PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
          "eglGetPlatformDisplayEXT");
  assert(get_platform_display);
  gl.display =
      get_platform_display(EGL_PLATFORM_GBM_KHR, native_display, nullptr);
  assert(eglInitialize(gl.display, &major, &minor));
  assert(eglBindAPI(EGL_OPENGL_ES_API));
  assert(eglChooseConfig(gl.display, config_attributes, &gl.config, 1, &n));
  LOG_ERROR << "eglConfig " << n;
  assert(n == 1);

  gl.context =
      eglCreateContext(gl.display, gl.config, EGL_NO_CONTEXT, context_attribs);
  assert(gl.context);
  gl.surface = eglCreateWindowSurface(gl.display, gl.config,
                                      (EGLNativeWindowType)native_window, NULL);
  assert(gl.surface);
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
  eglSwapInterval(gl.display, 1);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EglContext::CreateDrawBuffer(int32_t width, int32_t height) {
  display_width_ = width;
  display_height_ = height;
  glGenFramebuffers(1, &framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glGenTextures(1, &rendered_texture_);
  glBindTexture(GL_TEXTURE_2D, rendered_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, display_width_, display_height_, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         rendered_texture_, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EglContext::BindDrawBuffer(bool bind) {
  glBindFramebuffer(GL_FRAMEBUFFER, bind ? framebuffer_ : 0);
}

void EglContext::BlitFrameBuffer() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, display_width_, display_height_, 0, 0, display_width_,
                    display_height_, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void EglContext::MakeCurrent() {
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
}

void EglContext::EnableBlend(bool enable) {
  if (enable)
    glEnable(GL_BLEND);
  else
    glDisable(GL_BLEND);
}

void EglContext::SwapBuffers() {
  eglSwapBuffers(gl.display, gl.surface);
}

}  // namespace backend
}  // namespace naive
