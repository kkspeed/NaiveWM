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
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <drm_mode.h>
#include <fcntl.h>
#include <gbm.h>
#include <resources/cursor.h>
#include <sys/mman.h>
#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "base/logging.h"
#include "compositor/buffer.h"
#include "compositor/compositor_view.h"
#include "compositor/draw_quad.h"
#include "compositor/gl_renderer.h"
#include "compositor/surface.h"
#include "compositor/texture_delegate.h"
#include "resources/cursor.h"
#include "wm/window.h"
#include "wm/window_impl.h"
#include "wm/window_manager.h"

namespace naive {
namespace compositor {

namespace {

int waiting_for_flip = 0;
GLuint framebuffer;
GLuint renderedTexture;

struct {
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
  void* mouse_surface_data;
  int32_t display_width, display_height;
} gl;

struct {
  gbm_device* dev;
  gbm_surface* surface;
} gbm;

struct {
  int fd;
  drmModeModeInfo* mode;
  uint32_t crtc_id;
  uint32_t connector_id;
  int32_t physical_height, physical_width;
} drm;

struct drm_fb {
  uint32_t fb_id;
  bool need_modset = true;
};

int32_t find_crtc_for_encoder(const drmModeRes* resources,
                              const drmModeEncoder* encoder) {
  for (uint32_t i = 0; i < resources->count_crtcs; i++) {
    const uint32_t crtc_mask = ((uint32_t)1) << i;
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

  drm.physical_height = connector->mmHeight;
  drm.physical_width = connector->mmWidth;
  drm.connector_id = connector->connector_id;
}

void init_gbm() {
  gbm.dev = gbm_create_device(drm.fd);
  gbm.surface =
      gbm_surface_create(gbm.dev, drm.mode->hdisplay, drm.mode->vdisplay,
                         GBM_FORMAT_ARGB8888,  // TODO: ARGB?
                         GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT);
  assert(gbm.surface);
}

void init_gl() {
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
  gl.display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, nullptr);
  assert(eglInitialize(gl.display, &major, &minor));
  assert(eglBindAPI(EGL_OPENGL_ES_API));
  assert(eglChooseConfig(gl.display, config_attributes, &gl.config, 1, &n));
  LOG_ERROR << "eglConfig " << n << std::endl;
  assert(n == 1);

  gl.context =
      eglCreateContext(gl.display, gl.config, EGL_NO_CONTEXT, context_attribs);
  assert(gl.context);
  gl.surface = eglCreateWindowSurface(gl.display, gl.config,
                                      (EGLNativeWindowType)gbm.surface, NULL);
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
  eglSwapInterval(gl.display, 1);
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

drmEventContext evctx{
    DRM_EVENT_CONTEXT_VERSION, nullptr, page_flip_handler,
};

fd_set fds;
gbm_bo* bo = nullptr;
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
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glGenTextures(1, &renderedTexture);
  glBindTexture(GL_TEXTURE_2D, renderedTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gl.display_width, gl.display_height, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         renderedTexture, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  fb->need_modset = false;
}

void initialize_cursor() {
  drm_mode_create_dumb creq;
  drm_mode_map_dumb mreq;
  memset(&creq, 0, sizeof(creq));
  creq.width = 64;
  creq.height = 64;
  creq.bpp = 32;
  assert(drmIoctl(drm.fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) >= 0);
  uint32_t handle = creq.handle;

  int result = drmModeSetCursor(drm.fd, drm.crtc_id, handle, 64, 64);
  if (result < 0) {
    LOG_ERROR << "unable to set cusror " << strerror(errno) << std::endl;
    exit(1);
  }

  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = handle;
  assert(drmIoctl(drm.fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) == 0);
  gl.mouse_surface_data = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                               drm.fd, mreq.offset);

  if (gl.mouse_surface_data == MAP_FAILED) {
    LOG_ERROR << "cannot map mouse surface";
    exit(1);
  }
  memset(gl.mouse_surface_data, 0, creq.size);
  memcpy(
      gl.mouse_surface_data, cursor_image.pixel_data,
      cursor_image.width * cursor_image.height * cursor_image.bytes_per_pixel);
}

void move_cursor(int32_t x, int32_t y) {
  drmModeMoveCursor(drm.fd, drm.crtc_id, x, y);
}

void wait_page_flip() {
  if (fb) {
    waiting_for_flip = 1;
    drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT,
                    &waiting_for_flip);
    while (waiting_for_flip) {
      select(drm.fd + 1, &fds, nullptr, nullptr, nullptr);
      drmHandleEvent(drm.fd, &evctx);
    }
  }
}

void finalize_draw(bool did_draw) {
  if (did_draw) {
    gbm_bo* next_bo = nullptr;
    eglSwapBuffers(gl.display, gl.surface);
    next_bo = gbm_surface_lock_front_buffer(gbm.surface);
    fb = drm_fb_get_from_bo(next_bo);
    if (fb->need_modset) {
      drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0, &drm.connector_id, 1,
                     drm.mode);
      fb->need_modset = false;
    } else {
      wait_page_flip();
    }
    gbm_surface_release_buffer(gbm.surface, bo);
    bo = next_bo;
  } else {
    wait_page_flip();
  }
}

class Texture : public TextureDelegate {
 public:
  Texture(int width,
          int height,
          int32_t format,
          void* data,
          GlRenderer* renderer)
      : width_(width), height_(height), identifier_(0), renderer_(renderer) {
    if (format != WL_SHM_FORMAT_ARGB8888 && format != WL_SHM_FORMAT_XRGB8888) {
      TRACE("buffer format not WL_SHM_FORMAT_ARGB8888");
    }
    needs_backdrop_ = format == WL_SHM_FORMAT_XRGB8888;
    glGenTextures(1, &identifier_);
    glBindTexture(GL_TEXTURE_2D, identifier_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA,
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
    TRACE(
        "Draw: offset (%d %d) (in buffer offset: %d %d) (dimension: %d %d), "
        "texture dimension: (%d %d)",
        x, y, patch_x, patch_y, width, height, width_, height_);
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

    GLint vertices[] = {x + patch_x,         y + patch_y + height, x + patch_x,
                        y + patch_y,         x + patch_x + width,  y + patch_y,
                        x + patch_x + width, y + patch_y + height};
    float top_left_x = ((float)patch_x) / width_;
    float top_left_y = ((float)patch_y) / height_;
    float bottom_right_x = ((float)(patch_x + width)) / width_;
    float bottom_right_y = ((float)(patch_y + height)) / height_;

    TRACE("Texture coord: tl (%f %f), br (%f %f)", top_left_x, top_left_y,
          bottom_right_x, bottom_right_y);

    GLfloat tex_coords[] = {
        top_left_x,     bottom_right_y, top_left_x,     top_left_y,
        bottom_right_x, top_left_y,     bottom_right_x, bottom_right_y,
    };

    if (needs_backdrop_) {
      glDisable(GL_BLEND);
      renderer_->DrawTextureQuad(vertices, tex_coords, identifier_);
      glEnable(GL_BLEND);
    } else
      renderer_->DrawTextureQuad(vertices, tex_coords, identifier_);
  }

 private:
  GLuint identifier_;
  GlRenderer* renderer_;
  int32_t width_, height_;
  bool needs_backdrop_{false};
};

std::vector<wm::Window*> CollectNewlyCommittedWindows() {
  std::vector<wm::Window*> result;

  if (wm::WindowManager::Get()->wallpaper_window())
    result.push_back(wm::WindowManager::Get()->wallpaper_window());
  for (auto* window : wm::WindowManager::Get()->windows()) {
    std::stack<wm::Window*> stack;
    stack.push(window);
    while (!stack.empty()) {
      auto* current = stack.top();
      if (current->window_impl()->HasCommit())
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
  display_metrics_ = std::make_unique<wayland::DisplayMetrics>(
      gl.display_width, gl.display_height, drm.physical_width,
      drm.physical_height);
  initialize_cursor();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  renderer_ = std::make_unique<GlRenderer>(gl.display_width, gl.display_height);
}

void Compositor::AddGlobalDamage(const base::geometry::Rect& rect,
                                 wm::Window* window) {
  global_damage_region_.Union(rect * window->window_impl()->GetScale());
}

wayland::DisplayMetrics* Compositor::GetDisplayMetrics() {
  return display_metrics_.get();
}

#ifdef NAIVE_COMPOSITOR
void Compositor::Draw() {
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
  std::vector<wm::Window*> committed_windows = CollectNewlyCommittedWindows();
  if (!committed_windows.empty() || draw_forced_) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0, 0.0, 1.0, 1.0);
    for (auto* window : wm::WindowManager::Get()->windows()) {
      if (window->is_visible()) {
        DrawWindowRecursive(window, window->wm_x(), window->wm_y());
        DrawWindowBorder(window);
      }
    }
    finalize_draw(true);
  }
  if (copy_request_) {
    std::vector<uint8_t> screen_data;
    screen_data.resize(sizeof(uint32_t) * gl.display_width * gl.display_height);
    glReadPixels(0, 0, gl.display_width, gl.display_height, GL_RGBA,
                 GL_UNSIGNED_BYTE, screen_data.data());
    (*copy_request_)(std::move(screen_data), gl.display_width,
                     gl.display_height);
    copy_request_.reset();
  }

  DrawPointer();
  // TODO: still cannot disable this yet, since the surface dependency is not
  // resolved
  draw_forced_ = true;
}
#else
void Compositor::Draw() {
  eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  auto* wallpaper_window = wm::WindowManager::Get()->wallpaper_window();
  CompositorViewList view_list =
      wallpaper_window ? CompositorView::BuildCompositorViewHierarchyRecursive(
                             wallpaper_window, display_metrics_->scale)
                       : CompositorViewList();
  // TODO: clean this up and L520 as well.
  auto* panel_window = wm::WindowManager::Get()->panel_window();
  if (panel_window) {
    auto views = CompositorView::BuildCompositorViewHierarchyRecursive(
        panel_window, display_metrics_->scale);
    view_list.insert(view_list.end(), std::make_move_iterator(views.begin()),
                     std::make_move_iterator(views.end()));
  }

  for (auto* window : wm::WindowManager::Get()->windows()) {
    if (window->is_visible()) {
      CompositorViewList view_list_window =
          CompositorView::BuildCompositorViewHierarchyRecursive(
              window, display_metrics_->scale);
      view_list.insert(view_list.end(),
                       std::make_move_iterator(view_list_window.begin()),
                       std::make_move_iterator(view_list_window.end()));
    }
  }

  auto* ime_top_level = wm::WindowManager::Get()->input_panel_top_level();
  if (ime_top_level) {
    auto views = CompositorView::BuildCompositorViewHierarchyRecursive(
        ime_top_level, display_metrics_->scale);
    view_list.insert(view_list.end(), std::make_move_iterator(views.begin()),
                     std::make_move_iterator(views.end()));
  }

  auto* ime_overlay = wm::WindowManager::Get()->input_panel_overlay();
  if (ime_overlay) {
    auto views = CompositorView::BuildCompositorViewHierarchyRecursive(
        ime_overlay, display_metrics_->scale);
    view_list.insert(view_list.end(), std::make_move_iterator(views.begin()),
                     std::make_move_iterator(views.end()));
  }

  bool has_global_damage = !global_damage_region_.is_empty();

  if (has_global_damage) {
    for (auto& v : view_list) {
      auto additional_damage = global_damage_region_.Clone();
      additional_damage.Intersect(v->global_region());
      v->damaged_region().Union(additional_damage);
    }
  }

  for (int i = 0; i < view_list.size(); i++) {
    for (int j = i + 1; j < view_list.size(); j++) {
      // TRACE("subtracting %p: %s, from %p", view_list[j]->window(),
      //      view_list[j]->global_bounds().ToString().c_str(),
      //      view_list[i]->window());
      view_list[i]->damaged_region().Subtract(view_list[j]->global_region());
    }
  }

  if (!global_damage_region_.is_empty())
    global_damage_region_.Clear();

  bool did_draw = false;
  for (auto& view : view_list) {
    auto* window = view->window();
    window->window_impl()->ClearDamage();
    window->NotifyFrameCallback();
    if (window->window_impl()->HasCommit()) {
      window->window_impl()->ClearCommit();
      auto quad = window->window_impl()->GetQuad();
      if (quad.has_data()) {
        auto texture = std::make_unique<Texture>(quad.width(), quad.height(),
                                                 quad.format(), quad.data(),
                                                 renderer_.get());
        window->window_impl()->CacheTexture(std::move(texture));
      }
    }
    for (auto& rect : view->damaged_region().rectangles()) {
      auto bounds = view->global_bounds();
      TRACE("rectangle: %s, window %p, global bounds: %s, did draw: %d",
            rect.ToString().c_str(), view->window(), bounds.ToString().c_str(),
            did_draw);
      if (window->window_impl()->CachedTexture()) {
        int32_t physical_x = bounds.x();  // * display_metrics_->scale;
        int32_t physical_y = bounds.y();  // * display_metrics_->scale;
        int32_t to_draw_x =
            (rect.x() - bounds.x());  // * display_metrics_->scale;
        int32_t to_draw_y =
            (rect.y() - bounds.y());              // * display_metrics_->scale;
        int32_t physical_width = rect.width();    // * display_metrics_->scale;
        int32_t physical_height = rect.height();  // * display_metrics_->scale;
        window->window_impl()->CachedTexture()->Draw(
            physical_x, physical_y, to_draw_x, to_draw_y, physical_width,
            physical_height);
        did_draw = true;
      }
    }
  }

  if (did_draw) {
    for (int i = 0; i < view_list.size(); i++) {
      for (int j = i + 1; j < view_list.size(); j++)
        view_list[i]->border_region().Subtract(view_list[j]->global_region());
      for (auto& rect : view_list[i]->border_region().rectangles()) {
        if (!view_list[i]->window()->has_border())
          continue;
        if (view_list[i]->window()->focused() &&
            !view_list[i]->window()->parent())
          FillRect(rect, 0.0, 1.0, 1.0);
        else
          FillRect(rect, 0.0, 0.3, 0.3);
      }
    }
  }

  if (has_global_damage && !wallpaper_window) {
    Region full_screen = Region(base::geometry::Rect(
        0, 0, display_metrics_->width_dp, display_metrics_->height_dp));
    for (auto& v : view_list)
      full_screen.Subtract(v->global_region());
    did_draw = did_draw || !full_screen.is_empty();
    for (auto& r : full_screen.rectangles()) {
      TRACE("Fullscreen filling %s", r.ToString().c_str());
      FillRect(r, 0.0, 0.0, 0.0);
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (did_draw) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, gl.display_width, gl.display_height, 0, 0,
                      gl.display_width, gl.display_height, GL_COLOR_BUFFER_BIT,
                      GL_LINEAR);
  }

  if (copy_request_) {
    std::vector<uint8_t> screen_data;
    screen_data.resize(sizeof(uint32_t) * gl.display_width * gl.display_height);
    glReadPixels(0, 0, gl.display_width, gl.display_height, GL_RGBA,
                 GL_UNSIGNED_BYTE, screen_data.data());
    (*copy_request_)(std::move(screen_data), gl.display_width,
                     gl.display_height);
    copy_request_.reset();
  }

  finalize_draw(did_draw);
  DrawPointer();
  // TODO: still cannot disable this yet, since the surface dependency is not
  // resolved
  draw_forced_ = true;
}
#endif

void Compositor::DrawWindowRecursive(wm::Window* window,
                                     int32_t start_x,
                                     int32_t start_y) {
  window->window_impl()->NotifyFrameRendered();
  // TODO: child windows needs to be handled as well!
  if (window->window_impl()->HasCommit() || draw_forced_) {
    int32_t physical_x =
        (start_x + window->geometry().x()) * display_metrics_->scale;
    int32_t physical_y =
        (start_y + window->geometry().y()) * display_metrics_->scale;
    int32_t to_draw_x = window->GetToDrawRegion().x() * display_metrics_->scale;
    int32_t to_draw_y = window->GetToDrawRegion().y() * display_metrics_->scale;
    int32_t physical_width =
        window->GetToDrawRegion().width() * display_metrics_->scale;
    int32_t physical_height =
        window->GetToDrawRegion().height() * display_metrics_->scale;
    if (!window->window_impl()->HasCommit() &&
        window->window_impl()->CachedTexture()) {
      TRACE("Drawing Window: %p at %d %d", window,
            start_x + window->geometry().x(), start_y + window->geometry().y());
      window->window_impl()->CachedTexture()->Draw(
          physical_x, physical_y, to_draw_x, to_draw_y, physical_width,
          physical_height);
    } else {
      auto quad = window->window_impl()->GetQuad();
      if (quad.has_data()) {
        auto texture = std::make_unique<Texture>(quad.width(), quad.height(),
                                                 quad.format(), quad.data(),
                                                 renderer_.get());
        TRACE("Drawing Window: %p at %d %d", window,
              start_x + window->geometry().x(),
              start_y + window->geometry().y());
        texture->Draw(physical_x, physical_y, to_draw_x, to_draw_y,
                      physical_width, physical_height);
        window->window_impl()->CacheTexture(std::move(texture));
      }
    }
    window->window_impl()->ClearCommit();
  }

  for (auto* child : window->children()) {
    // TODO: Child widget coordinates might not be alright
    DrawWindowRecursive(child, start_x + window->geometry().x(),
                        start_y + window->geometry().y());
  }
}

void Compositor::FillRect(base::geometry::Rect rect,
                          float r,
                          float g,
                          float b) {
  int32_t x = rect.x();
  int32_t y = rect.y();
  GLint coords[] = {
      x,
      y,
      x,
      y + rect.height(),
      x + rect.width(),
      y + rect.height(),
      x + rect.width(),
      y,
  };
  renderer_->DrawSolidQuad(coords, r, g, b, true);
}

void Compositor::DrawWindowBorder(wm::Window* window) {
  if (!window->has_border())
    return;
  glLineWidth(1.5);
  int32_t x = window->wm_x() * display_metrics_->scale;
  int32_t y = window->wm_y() * display_metrics_->scale;
  auto rect = window->geometry();
  GLint coords[] = {
      x,
      y,
      x,
      y + rect.height() * display_metrics_->scale,
      x + rect.width() * display_metrics_->scale,
      y + rect.height() * display_metrics_->scale,
      x + rect.width() * display_metrics_->scale,
      y,
  };
  if (window->focused())
    renderer_->DrawSolidQuad(coords, 1.0, 0.0, 0.0, false);
  else
    renderer_->DrawSolidQuad(coords, 0.0, 1.0, 0.0, false);
}

void Compositor::DrawPointer() {
  auto pointer = wm::WindowManager::Get()->mouse_position();
  if (wm::WindowManager::Get()->pointer_updated()) {
    auto* pointer_window = wm::WindowManager::Get()->mouse_pointer();
    if (pointer_window) {
      auto quad = pointer_window->window_impl()->GetQuad();
      if (quad.has_data()) {
        memset(gl.mouse_surface_data, 0, 64 * 64 * 4);
        uint8_t* mouse_data = (uint8_t*)quad.data();
        for (size_t height = 0; height < quad.height(); height++) {
          for (size_t width = 0; width < quad.stride(); width++) {
            ((uint8_t*)gl.mouse_surface_data)[height * 256 + width] =
                mouse_data[height * quad.stride() + width];
          }
        }
      } else {
        memcpy((uint8_t*)(gl.mouse_surface_data), cursor_image.pixel_data,
               cursor_image.bytes_per_pixel * cursor_image.height *
                   cursor_image.width);
      }
      wm::WindowManager::Get()->clear_pointer_update();
    }
  }
  move_cursor(static_cast<int32_t>(pointer.x()),
              static_cast<int32_t>(pointer.y()));
}

}  // namespace compositor
}  // namespace naive
