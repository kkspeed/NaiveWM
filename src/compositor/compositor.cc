#include "compositor/compositor.h"

#include <cassert>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <wayland-server.h>

#include "backend/backend.h"
#include "backend/egl_context.h"
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

}  // namespace

Compositor* Compositor::g_compositor = nullptr;

// static
void Compositor::InitializeCompoistor(backend::Backend* backend) {
  g_compositor = new Compositor(backend);
}

// static
Compositor* Compositor::Get() {
  assert(g_compositor);
  return g_compositor;
}

Compositor::Compositor(backend::Backend* backend)
    : backend_(backend),
      egl_(backend->egl()),
      display_metrics_(backend->display_metrics()) {
  egl_->EnableBlend(true);
  renderer_ = std::make_unique<GlRenderer>(display_metrics_->width_pixels,
                                           display_metrics_->height_pixels);
}

void Compositor::AddGlobalDamage(const base::geometry::Rect& rect,
                                 wm::Window* window) {
  global_damage_region_.Union(rect * window->window_impl()->GetScale());
}

wayland::DisplayMetrics* Compositor::GetDisplayMetrics() {
  return display_metrics_;
}

void Compositor::Draw() {
  // egl_->MakeCurrent();
  egl_->BindDrawBuffer(true);
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

  bool has_any_commit = !global_damage_region_.is_empty();

  if (!global_damage_region_.is_empty())
    global_damage_region_.Clear();

  for (auto& view : view_list) {
    auto* window = view->window();
    window->NotifyFrameCallback();
    if (window->window_impl()->HasCommit())
      has_any_commit = true;
  }

  bool did_draw = false;
  if (has_any_commit) {
    for (auto& view : view_list) {
      has_any_commit = false;
      auto* window = view->window();
      window->window_impl()->ClearDamage();
      // window->NotifyFrameCallback();

      if (window->window_impl()->HasCommit()) {
        window->window_impl()->ClearCommit();
        has_any_commit = true;
        auto quad = window->window_impl()->GetQuad();
        if (quad.has_data()) {
          auto texture = std::make_unique<Texture>(quad.width(), quad.height(),
                                                   quad.format(), quad.data(),
                                                   renderer_.get());
          window->window_impl()->CacheTexture(std::move(texture));
        }
      }
#ifdef __NAIVE_COMPOSITOR__
      auto rectangles =
          std::vector<base::geometry::Rect>{view->global_bounds()};
#else
      auto rectangles(view->damaged_region().rectangles());
#endif
      for (auto& rect : rectangles) {
        auto bounds = view->global_bounds();
        TRACE("rectangle: %s, window %p, global bounds: %s, did draw: %d",
              rect.ToString().c_str(), view->window(),
              bounds.ToString().c_str(), did_draw);
        if (window->window_impl()->CachedTexture()) {
          int32_t physical_x = bounds.x();  // * display_metrics_->scale;
          int32_t physical_y = bounds.y();  // * display_metrics_->scale;
          int32_t to_draw_x =
              (rect.x() - bounds.x());  // * display_metrics_->scale;
          int32_t to_draw_y =
              (rect.y() - bounds.y());            // * display_metrics_->scale;
          int32_t physical_width = rect.width();  // * display_metrics_->scale;
          int32_t physical_height =
              rect.height();  // * display_metrics_->scale;
          window->window_impl()->CachedTexture()->Draw(
              physical_x, physical_y, to_draw_x, to_draw_y, physical_width,
              physical_height);
          did_draw = true;
        }
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

  egl_->BindDrawBuffer(false);

  if (did_draw)
    egl_->BlitFrameBuffer();

  if (copy_request_) {
    std::vector<uint8_t> screen_data;
    screen_data.resize(sizeof(uint32_t) * display_metrics_->width_pixels *
                       display_metrics_->height_pixels);
    glReadPixels(0, 0, display_metrics_->width_pixels,
                 display_metrics_->height_pixels, GL_RGBA, GL_UNSIGNED_BYTE,
                 screen_data.data());
    (*copy_request_)(std::move(screen_data), display_metrics_->width_pixels,
                     display_metrics_->height_pixels);
    copy_request_.reset();
  }

  backend_->FinalizeDraw(did_draw);
  if (backend_->SupportHwCursor())
    DrawPointer();
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
        memset(backend_->PointerData(), 0, 64 * 64 * 4);
        uint8_t* mouse_data = (uint8_t*)quad.data();
        for (size_t height = 0; height < quad.height(); height++) {
          for (size_t width = 0; width < quad.stride(); width++) {
            ((uint8_t*)backend_->PointerData())[height * 256 + width] =
                mouse_data[height * quad.stride() + width];
          }
        }
      } else {
        memcpy((uint8_t*)(backend_->PointerData()), cursor_image.pixel_data,
               cursor_image.bytes_per_pixel * cursor_image.height *
                   cursor_image.width);
      }
      wm::WindowManager::Get()->clear_pointer_update();
    }
  }
  backend_->MoveCursor(static_cast<int32_t>(pointer.x()),
                       static_cast<int32_t>(pointer.y()));
}

}  // namespace compositor
}  // namespace naive
