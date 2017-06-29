#include "compositor/compositor.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stack>

#include <sys/stat.h>
#include <sys/types.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_mode.h>
#include <fcntl.h>
#include <gbm.h>
#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "base/logging.h"
#include "compositor/buffer.h"
#include "compositor/surface.h"
#include "compositor/texture_delegate.h"
#include "wm/window.h"
#include "wm/window_manager.h"

namespace naive {
namespace compositor {

namespace {

struct {
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
  EGLSurface mouse_surface;
  int32_t display_width, display_height;
} gl;

struct {
  gbm_device* dev;
  gbm_surface* surface;
  gbm_surface* mouse_surface;
} gbm;

struct {
  int fd;
  drmModeModeInfo* mode;
  uint32_t crtc_id;
  uint32_t connector_id;
} drm;

struct drm_fb {
  uint32_t fb_id;
};

int32_t find_crtc_for_encoder(const drmModeRes* resources,
                              const drmModeEncoder* encoder) {
  for (uint32_t i = 0; i < resources->count_crtcs; i++) {
    const uint32_t crtc_mask = ((uint32_t) 1) << i;
    const uint32_t crtc_id = resources->crtcs[i];
    if (encoder->possible_crtcs & crtc_mask)
      return crtc_id;
  }

  return -1;
}

int32_t find_crtc_for_connector(const drmModeRes* resources,
                                const drmModeConnector* connector) {
  for (int32_t i = 0; i < connector->count_encoders; i++) {
    const uint32_t encoder_id = connector->encoders[i];
    drmModeEncoder* encoder = drmModeGetEncoder(drm.fd, encoder_id);
    if (encoder) {
      const int32_t crtc_id = find_crtc_for_encoder(resources, encoder);
      drmModeFreeEncoder(encoder);
      if (crtc_id != 0)
        return crtc_id;
    }
  }

  return -1;
}

void init_drm() {
  drmModeRes* resources;
  drmModeConnector* connector = nullptr;
  drmModeEncoder* encoder = nullptr;

  drm.fd = open("/dev/dri/card0", O_RDWR);
  assert(drm.fd >= 0);

  resources = drmModeGetResources(drm.fd);
  assert(resources);

  for (int i = 0; i < resources->count_connectors; i++) {
    connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
    if (connector->connection == DRM_MODE_CONNECTED)
      break;
    drmModeFreeConnector(connector);
    connector = nullptr;
  }

  assert(connector);

  for (int i = 0, area = 0; i < connector->count_modes; i++) {
    drmModeModeInfo* current_mode = &connector->modes[i];
    if (current_mode->type & DRM_MODE_TYPE_PREFERRED)
      drm.mode = current_mode;
    int current_area = current_mode->hdisplay * current_mode->vdisplay;
    if (current_area > area) {
      drm.mode = current_mode;
      area = current_area;
    }
  }

  assert(drm.mode);

  for (int i = 0; i < resources->count_encoders; i++) {
    encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
    if (encoder->encoder_id == connector->encoder_id)
      break;
    drmModeFreeEncoder(encoder);
    encoder = nullptr;
  }

  if (encoder) {
    drm.crtc_id = encoder->crtc_id;
  } else {
    int32_t crtc_id = find_crtc_for_connector(resources, connector);
    assert(crtc_id != 0);
    drm.crtc_id = static_cast<uint32_t>(crtc_id);
  }

  drm.connector_id = connector->connector_id;
}

void init_gbm() {
  gbm.dev = gbm_create_device(drm.fd);
  gbm.surface =
      gbm_surface_create(gbm.dev, drm.mode->hdisplay, drm.mode->vdisplay,
                         GBM_FORMAT_ARGB8888,  // TODO: ARGB?
                         GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  gbm.mouse_surface =
      gbm_surface_create(gbm.dev, 64, 64, GBM_FORMAT_ARGB8888,
                         GBM_BO_USE_CURSOR | GBM_BO_USE_RENDERING);
  assert(gbm.surface);
  assert(gbm.mouse_surface);
}

void init_gl() {
  EGLint major, minor, n;

  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  const EGLint config_attributes[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                      EGL_RED_SIZE, 1,
                                      EGL_GREEN_SIZE, 1,
                                      EGL_BLUE_SIZE, 1,
                                      EGL_ALPHA_SIZE, 1,
                                      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                                      EGL_NONE};

  PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress(
          "eglGetPlatformDisplayEXT");
  assert(get_platform_display);
  gl.display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, nullptr);

  assert(eglInitialize(gl.display, &major, &minor));
  assert(eglBindAPI(EGL_OPENGL_API));
  assert(eglChooseConfig(gl.display, config_attributes, &gl.config, 1, &n));
  LOG_ERROR << "eglConfig " << n << std::endl;
  assert(n == 1);

  gl.context =
      eglCreateContext(gl.display, gl.config, EGL_NO_CONTEXT, context_attribs);
  assert(gl.context);
  gl.surface = eglCreateWindowSurface(gl.display, gl.config,
                                      (EGLNativeWindowType) gbm.surface, NULL);
  gl.mouse_surface = eglCreateWindowSurface(
      gl.display, gl.config, (EGLNativeWindowType) gbm.mouse_surface, NULL);
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
}

void drm_fb_destroy_callback(gbm_bo* bo, void* data) {
  drm_fb* fb = static_cast<drm_fb*>(data);
  gbm_device* gbm = gbm_bo_get_device(bo);
  if (fb->fb_id)
    drmModeRmFB(drm.fd, fb->fb_id);
  free(fb);
}

drm_fb* drm_fb_get_from_bo(gbm_bo* bo) {
  drm_fb* fb = static_cast<drm_fb*>(gbm_bo_get_user_data(bo));
  uint32_t width, height, stride, handle;

  if (fb)
    return fb;

  fb = new drm_fb;
  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);
  stride = gbm_bo_get_stride(bo);
  handle = gbm_bo_get_handle(bo).u32;

  // TODO: This is a dirty workaround. Move somewhere else.
  gl.display_width = width;
  gl.display_height = height;
  assert(
      !drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id));
  gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);
  return fb;
}

void page_flip_handler(int fd,
                       unsigned int frame,
                       unsigned int sec,
                       unsigned int usec,
                       void* data) {
  int* waiting_for_flip = static_cast<int*>(data);
  *waiting_for_flip = 0;
}

drmEventContext evctx = {
    .version = DRM_EVENT_CONTEXT_VERSION,
    .page_flip_handler = page_flip_handler,
};

fd_set fds;
gbm_bo* bo;
drm_fb* fb;

void drm_egl_init() {
  init_drm();
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  FD_SET(drm.fd, &fds);
  init_gbm();
  init_gl();
  eglSwapBuffers(gl.display, gl.surface);
  bo = gbm_surface_lock_front_buffer(gbm.surface);
  fb = drm_fb_get_from_bo(bo);
  drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0, &drm.connector_id, 1,
                 drm.mode);
}

void initialize_cursor() {
  eglMakeCurrent(gl.display, gl.mouse_surface, gl.mouse_surface, gl.context);
  eglSwapBuffers(gl.display, gl.mouse_surface);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport(0, 0, 64, 64);
  glMatrixMode(GL_PROJECTION);
  glOrtho(0, 64, 64, 0, 1, -1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glColor4f(0.0, 0.0, 0.0, 1.0);
  glBegin(GL_TRIANGLES);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(20.0f, 0.0f);
  glVertex2f(0.0f, 20.0f);
  glEnd();
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glBegin(GL_TRIANGLES);
  glVertex2f(2.0f, 2.0f);
  glVertex2f(22.0f, 2.0f);
  glVertex2f(2.0f, 22.0f);
  glEnd();

  eglSwapBuffers(gl.display, gl.mouse_surface);
  gbm_bo* bo = gbm_surface_lock_front_buffer(gbm.mouse_surface);
  int result =
      drmModeSetCursor(drm.fd, drm.crtc_id, gbm_bo_get_handle(bo).u32, 64, 64);
  if (result < 0) {
    LOG_ERROR << "unable to set cusror " << strerror(errno) << std::endl;
    exit(1);
  }
  gbm_surface_release_buffer(gbm.mouse_surface, bo);
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
}

void move_cursor(int32_t x, int32_t y) {
  drmModeMoveCursor(drm.fd, drm.crtc_id, x, y);
}

void finalize_draw() {
  gbm_bo* next_bo;
  int waiting_for_flip = 1;
  eglSwapBuffers(gl.display, gl.surface);
  next_bo = gbm_surface_lock_front_buffer(gbm.surface);
  fb = drm_fb_get_from_bo(next_bo);

  drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT,
                  &waiting_for_flip);
  while (waiting_for_flip) {
    select(drm.fd + 1, &fds, nullptr, nullptr, nullptr);
    drmHandleEvent(drm.fd, &evctx);
  }

  gbm_surface_release_buffer(gbm.surface, bo);
  bo = next_bo;
}

class Texture : public TextureDelegate {
 public:
  Texture(int width, int height, int32_t format, void* data)
      : width_(width), height_(height), identifier_(0) {
    if (format != WL_SHM_FORMAT_ARGB8888 && format != WL_SHM_FORMAT_XRGB8888) {
      TRACE("buffer format not WL_SHM_FORMAT_ARGB8888");
    }
    glGenTextures(1, &identifier_);
    glBindTexture(GL_TEXTURE_2D, identifier_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  ~Texture() {
    TRACE();
    if (identifier_)
      glDeleteTextures(1, &identifier_);
  }

  void Draw(int x, int y, int patch_x, int patch_y, int width, int height)
  override {
    TRACE("Draw: offset (%d %d) (in buffer offset: %d %d) (dimension: %d %d)",
          x, y, patch_x, patch_y, width, height);
    glColor3f(1.0, 1.0, 1.0);
    if (!identifier_)
      return;

    if (width_ == 0)
      width_ = 1;
    if (height_ == 0)
      height_ = 1;
    if (width > width_)
      width = width_;
    if (height > height_)
      height = height_;

    GLint vertices[] = {x + patch_x, y + patch_y, x + patch_x + width,
                        y + patch_y, x + patch_x + width, y + patch_y + height,
                        x + patch_x, y + patch_y + height};
    float top_left_x = ((float) patch_x) / width_;
    float top_left_y = ((float) patch_y) / height_;
    float bottom_right_x = ((float) (patch_x + width)) / width_;
    float bottom_right_y = ((float) (patch_y + height)) / height_;

    TRACE("Texture coord: tl (%f %f), br (%f %f)", top_left_x, top_left_y,
          bottom_right_x, bottom_right_y);

    GLfloat tex_coords[] = {top_left_x, top_left_y, bottom_right_x,
                            top_left_y, bottom_right_x, bottom_right_y,
                            top_left_x, bottom_right_y};

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glBindTexture(GL_TEXTURE_2D, identifier_);
    glVertexPointer(2, GL_INT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, tex_coords);

    glDrawArrays(GL_QUADS, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_RECTANGLE);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
  }

 private:
  GLuint identifier_;
  int32_t width_, height_;
};

std::vector<wm::Window*> CollectNewlyCommittedWindows() {
  std::vector<wm::Window*> result;

  for (auto* window : wm::WindowManager::Get()->windows()) {
    std::stack<wm::Window*> stack;
    stack.push(window);
    while (!stack.empty()) {
      auto* current = stack.top();
      if (current->surface()->has_commit())
        result.push_back(current);
      stack.pop();
      for (auto* child : current->children()) {
        stack.push(child);
      }
    }
  }

  return result;
}

}  // namespace

Compositor* Compositor::g_compositor = nullptr;

// static
void Compositor::InitializeCompoistor() {
  g_compositor = new Compositor();
}

// static
Compositor* Compositor::Get() {
  assert(g_compositor);
  return g_compositor;
}

Compositor::Compositor() {
  drm_egl_init();
  int width = gl.display_width;
  int height = gl.display_height;
  initialize_cursor();
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, height, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
}

void Compositor::Draw() {
  std::vector<wm::Window*> committed_windows = CollectNewlyCommittedWindows();
  if (!committed_windows.empty() || draw_forced_) {
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0, 0.0, 1.0, 1.0);
    for (auto* window : wm::WindowManager::Get()->windows()) {
      if (window->is_visible()) {
        DrawWindowRecursive(window, window->wm_x(), window->wm_y());
        DrawWindowBorder(window);
      }
    }
    finalize_draw();
  }
  if (copy_request_) {
    std::vector<uint8_t> screen_data;
    screen_data.resize(sizeof(uint32_t) * gl.display_width * gl.display_height);
    glReadPixels(0,
                 0,
                 gl.display_width, gl.display_height,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 screen_data.data());
    (*copy_request_)(std::move(screen_data), gl.display_width,
                     gl.display_height);
    copy_request_.reset();
  }

  DrawPointer();
  // TODO: still cannot disable this yet, since the surface dependency is not
  // resolved
  draw_forced_ = true;
}

void Compositor::DrawWindowRecursive(wm::Window* window,
                                     int32_t start_x,
                                     int32_t start_y) {
  // TODO: child windows needs to be handled as well!
  if (window->surface()->has_commit() || draw_forced_) {
    window->surface()->RunSurfaceCallback();
    if (!window->surface()->has_commit() &&
        window->surface()->cached_texture()) {
      TRACE("Drawing Window: %p at %d %d", window,
            start_x + window->geometry().x(), start_y + window->geometry().y());
      window->surface()->cached_texture()->Draw(
          start_x + window->geometry().x(), start_y + window->geometry().y(),
          window->GetToDrawRegion().x(), window->GetToDrawRegion().y(),
          window->GetToDrawRegion().width(),
          window->GetToDrawRegion().height());
    } else {
      auto* buffer = window->surface()->committed_buffer();
      if (buffer && buffer->data()) {
        auto texture =
            std::make_unique<Texture>(buffer->width(), buffer->height(),
                                      buffer->format(), buffer->local_data());
        TRACE("Drawing Window: %p at %d %d", window,
              start_x + window->geometry().x(),
              start_y + window->geometry().y());
        texture->Draw(
            start_x + window->geometry().x(), start_y + window->geometry().y(),
            window->GetToDrawRegion().x(), window->GetToDrawRegion().y(),
            window->GetToDrawRegion().width(),
            window->GetToDrawRegion().height());
        window->surface()->cache_texture(std::move(texture));
      }
    }
    window->surface()->clear_commit();
  }

  for (auto* child : window->children()) {
    // TODO: Child widget coordinates might not be alright
    DrawWindowRecursive(child, start_x + window->geometry().x(),
                        start_y + window->geometry().y());
  }
}

void Compositor::DrawWindowBorder(wm::Window* window) {
  glLineWidth(2.5);
  if (window->focused())
    glColor3f(1.0, 0.0, 0.0);
  else
    glColor3f(0.0, 1.0, 0.0);
  int32_t x = window->wm_x(), y = window->wm_y();
  auto rect = window->geometry();
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y);
  glVertex2f(x + rect.width(), y);
  glVertex2f(x + rect.width(), y + rect.height());
  glVertex2f(x, y + rect.height());
  glEnd();
}

void Compositor::DrawPointer() {
  auto pointer = wm::WindowManager::Get()->mouse_position();
  move_cursor(static_cast<int32_t>(pointer.x()),
              static_cast<int32_t>(pointer.y()));
}

}  // namespace compositor
}  // namespace naive
