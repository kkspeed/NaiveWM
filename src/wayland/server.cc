
#include "wayland/server.h"

#include <wayland-server.h>
#include <poll.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>

#include "xdg-shell-unstable-v6.h"

#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/compositor.h"
#include "compositor/buffer.h"
#include "compositor/region.h"
#include "compositor/shell_surface.h"
#include "compositor/subsurface.h"
#include "compositor/surface.h"
#include "wayland/display.h"
#include "wayland/pointer.h"
#include "wm/window_manager.h"
#include "wayland/display.h"

namespace naive {
namespace wayland {

namespace {

template<class T>
T* GetUserDataAs(wl_resource* resource) {
  return static_cast<T*>(wl_resource_get_user_data(resource));
}

template<class T>
std::unique_ptr<T> TakeUserDataAs(wl_resource* resource) {
  std::unique_ptr<T> user_data = std::unique_ptr<T>(GetUserDataAs<T>(resource));
  wl_resource_set_user_data(resource, nullptr);
  return user_data;
}

template<class T>
void DestroyUserData(wl_resource* resource) {
  TakeUserDataAs<T>(resource);
}

template<class T>
void SetImplementation(wl_resource* resource, const void* implementation,
                       std::unique_ptr<T> user_data) {
  wl_resource_set_implementation(resource, implementation, user_data.release(),
                                 DestroyUserData<T>);
}

///////////////////////////////////////////////////////////////////////////////
// wl_buffer_interface

void buffer_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

const struct wl_buffer_interface buffer_implementation = {buffer_destroy};

void HandleBufferReleaseCallback(wl_resource* resource) {
  LOG_ERROR << "Buffer release callback" << std::endl;
  wl_buffer_send_release(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

///////////////////////////////////////////////////////////////////////////////
// wl_surface_interface

void surface_destroy(wl_client* client, wl_resource* resource) {
  LOG_ERROR << "surface_destroy " << resource << std::endl;
  wl_resource_destroy(resource);
}

void surface_attach(wl_client* client, wl_resource* resource,
                    wl_resource* buffer, int32_t x, int32_t y) {
  GetUserDataAs<Surface>(resource)->Attach(
      buffer ? GetUserDataAs<Buffer>(buffer) : nullptr);
}

void surface_damage(wl_client* client, wl_resource* resource, int32_t x,
                    int32_t y, int32_t width, int32_t height) {
  GetUserDataAs<Surface>(resource)->Damage(
      base::geometry::Rect(x, y, width, height));
}

void surface_frame(wl_client* client, wl_resource* resource,
                   uint32_t callback) {
  // TODO: implement this optimization.
}

void surface_set_opaque_region(wl_client* client, wl_resource* resource,
                               wl_resource* region_resource) {
  GetUserDataAs<Surface>(resource)->SetOpaqueRegion(
      region_resource ? *GetUserDataAs<Region>(region_resource)
                      : Region::Empty());
}

void surface_set_input_region(wl_client* client, wl_resource* resource,
                              wl_resource* region_resource) {
  GetUserDataAs<Surface>(resource)->SetInputRegion(
      region_resource ? *GetUserDataAs<Region>(region_resource)
                      : Region::Empty());
}

void surface_commit(wl_client* client, wl_resource* resource) {
  GetUserDataAs<Surface>(resource)->Commit();
}

void surface_set_buffer_transform(wl_client* client, wl_resource* resource,
                                  int transform) {
  NOTIMPLEMENTED();
}

void surface_set_buffer_scale(wl_client* client, wl_resource* resource,
                              int32_t scale) {
  if (scale < 1) {
    wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_SCALE,
                           "buffer scale must be one (%d specified)", scale);
    return;
  }
  GetUserDataAs<Surface>(resource)->SetBufferScale(scale);
}

const struct wl_surface_interface surface_implementation = {
    .attach = surface_attach,
    .commit = surface_commit,
    .damage = surface_damage,
    .destroy = surface_destroy,
    .frame = surface_frame,
    .set_buffer_transform = surface_set_buffer_transform,
    .set_buffer_scale = surface_set_buffer_scale,
    .set_opaque_region = surface_set_opaque_region,
    .set_input_region = surface_set_input_region};

///////////////////////////////////////////////////////////////////////////////
// wl_region_interface
void region_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void region_add(wl_client* client, wl_resource* resource, int32_t x, int32_t y,
                int32_t width, int32_t height) {
  Region region(base::geometry::Rect(x, y, width, height));
  GetUserDataAs<Region>(resource)->Union(region);
}

void region_subtract(wl_client* client, wl_resource* resource, int32_t x,
                     int32_t y, int32_t width, int32_t height) {
  Region region(base::geometry::Rect(x, y, width, height));
  GetUserDataAs<Region>(resource)->Subtract(region);
}

const struct wl_region_interface region_implementation = {
    .add = region_add, .subtract = region_subtract, .destroy = region_destroy};

///////////////////////////////////////////////////////////////////////////////
// wl_compositor

void compositor_create_surface(wl_client* client, wl_resource* resource,
                               uint32_t id) {
  LOG_ERROR << "compositor_create_surface" << std::endl;
  std::unique_ptr<Surface> surface =
      GetUserDataAs<Display>(resource)->CreateSurface();
  wl_resource* surface_resource = wl_resource_create(
      client, &wl_surface_interface, wl_resource_get_version(resource), id);
  surface->set_resource(resource);
  SetImplementation(surface_resource, &surface_implementation,
                    std::move(surface));
}

void compositor_create_region(wl_client* client, wl_resource* resource,
                              uint32_t id) {
  LOG_ERROR << "compositor_create_region" << std::endl;
  wl_resource* region_resource =
      wl_resource_create(client, &wl_region_interface, 1, id);
  SetImplementation(region_resource, &region_implementation,
                    std::make_unique<Region>(Region::Empty()));
}

const struct wl_compositor_interface compositor_implementation = {
    .create_region = compositor_create_region,
    .create_surface = compositor_create_surface,
};

void bind_compositor(wl_client* client,
                     void* data,
                     uint32_t version,
                     uint32_t id) {
  LOG_ERROR << "bind_compositor" << std::endl;
  wl_resource* resource =
      wl_resource_create(client, &wl_compositor_interface, version, id);
  wl_resource_set_implementation(resource,
                                 &compositor_implementation,
                                 data,
                                 nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// wl_shm_pool_interface

void shm_pool_create_buffer(wl_client* client, wl_resource* resource,
                            uint32_t id, int32_t offset, int32_t width,
                            int32_t height, int32_t stride, uint32_t format) {
  LOG_ERROR << "shm_pool_create_buffer" << std::endl;
  if (format != WL_SHM_FORMAT_XRGB8888 && format != WL_SHM_FORMAT_ARGB8888) {
    std::cerr << "unsupported format " << format;
    return;
  }
  std::unique_ptr<Buffer> buffer =
      GetUserDataAs<SharedMemory>(resource)->CreateBuffer(width, height, format,
                                                          offset, stride);
  if (!buffer) {
    std::cerr << "unable to map buffer";
    return;
  }

  wl_resource* buffer_resource =
      wl_resource_create(client, &wl_buffer_interface, 1, id);
  buffer->set_release_callback(std::bind(&HandleBufferReleaseCallback,
                                         buffer_resource));
  SetImplementation(buffer_resource, &buffer_implementation, std::move(buffer));
}

void shm_pool_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void shm_pool_resize(wl_client* client, wl_resource* resource, int32_t size) {
  NOTIMPLEMENTED();
}

const struct wl_shm_pool_interface shm_pool_implementation = {
    .create_buffer = shm_pool_create_buffer,
    .destroy = shm_pool_destroy,
    .resize = shm_pool_resize};

///////////////////////////////////////////////////////////////////////////////
// wl_shm_interface:

void shm_create_pool(wl_client* client, wl_resource* resource, uint32_t id,
                     int fd, int32_t size) {
  LOG_ERROR << "shm_create_pool" << std::endl;
  std::unique_ptr<SharedMemory> shared_memory =
      GetUserDataAs<Display>(resource)->CreateSharedMemory(fd, size);
  if (!shared_memory) {
    wl_resource_post_no_memory(resource);
    return;
  }

  wl_resource* shm_pool_resource =
      wl_resource_create(client, &wl_shm_pool_interface, 1, id);
  SetImplementation(shm_pool_resource, &shm_pool_implementation,
                    std::move(shared_memory));
}

const struct wl_shm_interface shm_implementation = {shm_create_pool};

void bind_shm(wl_client* client, void* data, uint32_t version, uint32_t id) {
  LOG_ERROR << "bind_shm" << std::endl;
  wl_resource* resource = wl_resource_create(client, &wl_shm_interface, 1, id);

  wl_resource_set_implementation(resource, &shm_implementation, data, nullptr);

  wl_shm_send_format(resource, WL_SHM_FORMAT_XRGB8888);
  wl_shm_send_format(resource, WL_SHM_FORMAT_ARGB8888);
}

///////////////////////////////////////////////////////////////////////////////
// wl_subsurface_interface:

void subsurface_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void subsurface_set_position(wl_client* client, wl_resource* resource,
                             int32_t x, int32_t y) {
  GetUserDataAs<SubSurface>(resource)->SetPosition(x, y);
}

void subsurface_place_above(wl_client* client, wl_resource* resource,
                            wl_resource* reference) {
  GetUserDataAs<SubSurface>(resource)->PlaceAbove(
      GetUserDataAs<Surface>(reference));
}

void subsurface_place_below(wl_client* client, wl_resource* resource,
                            wl_resource* reference) {
  GetUserDataAs<SubSurface>(resource)->PlaceBelow(
      GetUserDataAs<Surface>(reference));
}

void subsurface_set_sync(wl_client* client, wl_resource* resource) {
  GetUserDataAs<SubSurface>(resource)->SetCommitBehavior(true);
}

void subsurface_set_desync(wl_client* client, wl_resource* resource) {
  GetUserDataAs<SubSurface>(resource)->SetCommitBehavior(false);
}

const struct wl_subsurface_interface subsurface_implementation{
    .destroy = subsurface_destroy, .set_position = subsurface_set_position,
    .place_above = subsurface_place_above, .place_below = subsurface_place_below,
    .set_desync = subsurface_set_desync, .set_sync = subsurface_set_sync,
};

///////////////////////////////////////////////////////////////////////////////
// wl_subcompositor_interface

void subcompositor_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void subcompositor_get_subsurface(wl_client* client, wl_resource* resource,
                                  uint32_t id, wl_resource* surface,
                                  wl_resource* parent) {
  auto subsurface = GetUserDataAs<Display>(resource)->CreateSubSurface(
      GetUserDataAs<Surface>(surface), GetUserDataAs<Surface>(parent));
  if (!subsurface) {
    wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
                           "invalid surface");
    return;
  }

  wl_resource* subsurface_resource =
      wl_resource_create(client, &wl_subsurface_interface, 1, id);

  SetImplementation(subsurface_resource, &subsurface_implementation,
                    std::move(subsurface));
}

const struct wl_subcompositor_interface subcompositor_implementation = {
    subcompositor_destroy, subcompositor_get_subsurface};

void bind_subcompositor(wl_client* client, void* data, uint32_t version,
                        uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &wl_subcompositor_interface, 1, id);
  wl_resource_set_implementation(resource, &subcompositor_implementation, data,
                                 nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// wl_shell_surface_interface:

void shell_surface_pong(wl_client* client, wl_resource* resource,
                        uint32_t serial) {
  NOTIMPLEMENTED();
}

void shell_surface_move(wl_client* client, wl_resource* resource,
                        wl_resource* seat_resource, uint32_t serial) {
  GetUserDataAs<ShellSurface>(resource)->Move();
}

void shell_surface_resize(wl_client* client, wl_resource* resource,
                          wl_resource* seat_resource, uint32_t serial,
                          uint32_t edges) {
  NOTIMPLEMENTED();
}

void shell_surface_set_toplevel(wl_client* client, wl_resource* resource) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  if (shell_surface->window()->IsManaged()) return;

  wm::WindowManager::Get()->Manage(shell_surface->window());
}

void shell_surface_set_transient(wl_client* client, wl_resource* resource,
                                 wl_resource* parent_resource, int x, int y,
                                 uint32_t flags) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  auto* parent_surface = GetUserDataAs<Surface>(parent_resource);
  shell_surface->window()->SetParent(parent_surface->window());
  shell_surface->window()->SetTransient(true);
  wm::WindowManager::Get()->Manage(shell_surface->window());
}

void shell_surface_set_fullscreen(wl_client* client, wl_resource* resource,
                                  uint32_t method, uint32_t framerate,
                                  wl_resource* output_resource) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  shell_surface->window()->SetFullscreen(true);
}

void shell_surface_set_popup(wl_client* client, wl_resource* resource,
                             wl_resource* seat_resource, uint32_t serial,
                             wl_resource* parent_resource, int32_t x, int32_t y,
                             uint32_t flags) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  auto* parent_surface = GetUserDataAs<Surface>(parent_resource);
  shell_surface->window()->SetParent(parent_surface->window());
  shell_surface->window()->SetPopup(true);
  wm::WindowManager::Get()->Manage(shell_surface->window());
}

void shell_surface_set_maximized(wl_client* client, wl_resource* resource,
                                 wl_resource* output_resource) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  shell_surface->window()->SetMaximized(true);
}

void shell_surface_set_title(wl_client* client, wl_resource* resource,
                             const char* title) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetTitle(
      std::string(title));
}

void shell_surface_set_class(wl_client* client, wl_resource* resource,
                             const char* clazz) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetClass(clazz);
}

const struct wl_shell_surface_interface shell_surface_implementation = {
    .pong = shell_surface_pong,
    .move = shell_surface_move,
    .resize = shell_surface_resize,
    .set_toplevel = shell_surface_set_toplevel,
    .set_transient = shell_surface_set_transient,
    .set_fullscreen = shell_surface_set_fullscreen,
    .set_popup = shell_surface_set_popup,
    .set_maximized = shell_surface_set_maximized,
    .set_title = shell_surface_set_title,
    .set_class = shell_surface_set_class};

///////////////////////////////////////////////////////////////////////////////
// wl_shell_interface:

void HandleShellSurfaceCloseCallback(wl_resource* resource) {
  uint32_t serial = wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource)));
  wl_shell_surface_send_ping(resource, serial);
  wl_client_flush(wl_resource_get_client(resource));
}

uint32_t HandleShellSurfaceConfigureCallback(wl_resource* resource,
                                             int32_t width, int32_t height) {
  wl_shell_surface_send_configure(resource, WL_SHELL_SURFACE_RESIZE_NONE, width,
                                  height);
  wl_client_flush(wl_resource_get_client(resource));
  return 0;
}

void shell_get_shell_surface(wl_client* client, wl_resource* resource,
                             uint32_t id, wl_resource* surface) {
  auto shell_surface = GetUserDataAs<Display>(resource)->CreateShellSurface(
      GetUserDataAs<Surface>(surface));
  if (!shell_surface) {
    wl_resource_post_error(resource, WL_SHELL_ERROR_ROLE,
                           "surface has already been assigned a role");
    return;
  }

  wl_resource* shell_surface_resource =
      wl_resource_create(client, &wl_shell_surface_interface, 1, id);
  shell_surface->set_close_callback(
      std::bind(&HandleShellSurfaceCloseCallback, shell_surface_resource));
  shell_surface->set_configure_callback(
      std::bind(&HandleShellSurfaceConfigureCallback, shell_surface_resource,
                std::placeholders::_1, std::placeholders::_2));
  shell_surface->set_destroy_callback(
      std::bind(&wl_resource_destroy, shell_surface_resource));
  SetImplementation(shell_surface_resource, &shell_surface_implementation,
                    std::move(shell_surface));
}

const struct wl_shell_interface shell_implementation = {
    shell_get_shell_surface};

void bind_shell(wl_client* client, void* data, uint32_t version, uint32_t id) {
  LOG_ERROR << "bind shell" << std::endl;
  wl_resource* resource =
      wl_resource_create(client, &wl_shell_interface, 1, id);
  wl_resource_set_implementation(resource, &shell_implementation, data,
                                 nullptr);
}

//////////////////////////////////////////////////////////////////////////////
// wl_pointer_interface:

void pointer_set_cursor(wl_client* client,
                        wl_resource* resource,
                        uint32_t serial,
                        wl_resource* surface,
                        int32_t hotspot_x,
                        int32_t hotspoy_y) {
  // TODO: implement set cursor.
  NOTIMPLEMENTED();
}

void pointer_release(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

const struct wl_pointer_interface pointer_implementation {
    .release = pointer_release,
    .set_cursor = pointer_set_cursor
};

//////////////////////////////////////////////////////////////////////////////
// wl_keyboard_interface:

///////////////////////////////////////////////////////////////////////////////
// wl_seat interface:

void seat_get_pointer(wl_client* client, wl_resource* resource, uint32_t id) {
  wl_resource* pointer_resource =
      wl_resource_create(client, &wl_pointer_interface,
                         wl_resource_get_version(resource), id);
  auto pointer = std::make_unique<Pointer>(pointer_resource);
  SetImplementation(pointer_resource, &pointer_implementation,
                    std::move(pointer));
}

void seat_get_keyboard(wl_client* client, wl_resource* resource, uint32_t id) {
  // TODO: implement wayland keyboard interface.
  NOTIMPLEMENTED();
}

void seat_get_touch(wl_client* client, wl_resource* resource, uint32_t id) {
  // TODO: no need to implement touch?
  NOTIMPLEMENTED();
}

void seat_release(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

const struct wl_seat_interface seat_implementation = {
    .get_keyboard = seat_get_keyboard,
    .get_pointer = seat_get_pointer,
    .get_touch = seat_get_touch,
    .release = seat_release
};

void bind_seat(wl_client* client, void* data, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_seat_interface, version, id);
  wl_resource_set_implementation(resource, &seat_implementation, data, nullptr);
  wl_seat_send_name(resource, "seat0");
  // TODO: add keyboard
  uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER;
  wl_seat_send_capabilities(resource, capabilities);
}

///////////////////////////////////////////////////////////////////////////////
// wl_output interfaces:

class WaylandOutput {
 public:
  WaylandOutput(wl_resource* resource): output_resource_(resource) {
    SendDisplayMetrics();
  }

  void SendDisplayMetrics() {
    const float kInchInMm = 25.4f;
    const char* kUnknownMake = "unknown";
    const char* kUnknownModel = "unknown";

    // TODO: Get real display information.
    base::geometry::Rect bounds(0, 0, 2560, 1080);
    wl_output_send_geometry(
        output_resource_, bounds.x(), bounds.y(),
        static_cast<int>(kInchInMm * bounds.width() / 100.0),
        static_cast<int>(kInchInMm * bounds.height() / 100.0),
        WL_OUTPUT_SUBPIXEL_UNKNOWN, kUnknownMake, kUnknownModel,
        WL_OUTPUT_TRANSFORM_NORMAL);

    wl_output_send_mode(
        output_resource_, WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED,
        bounds.width(), bounds.height(), static_cast<int>(60000));

    if (wl_resource_get_version(output_resource_) >=
        WL_OUTPUT_DONE_SINCE_VERSION) {
      wl_output_send_done(output_resource_);
    }
  }
 private:
  wl_resource* output_resource_;
};

void bind_output(wl_client* client, void* data, uint32_t version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &wl_output_interface, version, id);
  SetImplementation(resource, nullptr,
                    std::make_unique<WaylandOutput>(resource));

}

///////////////////////////////////////////////////////////////////////////////
// xdg_positioner_interface:

void xdg_positioner_v6_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void xdg_positioner_v6_set_size(wl_client* client, wl_resource* resource,
                                int32_t width, int32_t height) {
  NOTIMPLEMENTED();
}

void xdg_positioner_v6_set_anchor_rect(wl_client* client, wl_resource* resource,
                                       int32_t x, int32_t y, int32_t width,
                                       int32_t height) {
  NOTIMPLEMENTED();
}

void xdg_positioner_v6_set_anchor(wl_client* client, wl_resource* resource,
                                  uint32_t anchor) {
  NOTIMPLEMENTED();
}

void xdg_positioner_v6_set_gravity(wl_client* client, wl_resource* resource,
                                   uint32_t gravity) {
  NOTIMPLEMENTED();
}

void xdg_positioner_v6_set_constraint_adjustment(
    wl_client* client, wl_resource* resource, uint32_t constraint_adjustment) {
  NOTIMPLEMENTED();
}

void xdg_positioner_v6_set_offset(wl_client* client, wl_resource* resource,
                                  int32_t x, int32_t y) {
  NOTIMPLEMENTED();
}

const struct zxdg_positioner_v6_interface xdg_positioner_v6_implementation = {
    .destroy = xdg_positioner_v6_destroy,
    .set_anchor = xdg_positioner_v6_set_anchor,
    .set_anchor_rect = xdg_positioner_v6_set_anchor_rect,
    .set_constraint_adjustment = xdg_positioner_v6_set_constraint_adjustment,
    .set_gravity = xdg_positioner_v6_set_gravity,
    .set_offset = xdg_positioner_v6_set_offset,
    .set_size = xdg_positioner_v6_set_size};

///////////////////////////////////////////////////////////////////////////////
// xdg_toplevel_interface

void xdg_toplevel_v6_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void xdg_toplevel_v6_set_parent(wl_client* client, wl_resource* resource,
                                wl_resource* parent) {
  if (!parent) {
    GetUserDataAs<ShellSurface>(resource)->window()->SetParent(nullptr);
    return;
  }

  if (GetUserDataAs<ShellSurface>(resource)->window())
    GetUserDataAs<ShellSurface>(resource)->window()->SetParent(
        GetUserDataAs<ShellSurface>(parent)->window());
}

void xdg_toplevel_v6_set_title(wl_client* client, wl_resource* resource,
                               const char* title) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetTitle(
      std::string(title));
}

void xdg_toplevel_v6_set_app_id(wl_client* client, wl_resource* resource,
                                const char* app_id) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetAppId(
      std::string(app_id));
}

void xdg_toplevel_v6_show_window_menu(wl_client* client, wl_resource* resource,
                                      wl_resource* seat, uint32_t serial,
                                      int32_t x, int32_t y) {
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_move(wl_client* client, wl_resource* resource,
                          wl_resource* seat, uint32_t serial) {
  GetUserDataAs<ShellSurface>(resource)->Move();
}

void xdg_toplevel_v6_resize(wl_client* client, wl_resource* resource,
                            wl_resource* seat, uint32_t serial,
                            uint32_t edges) {
  // TODO: Implement resize.
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_set_max_size(wl_client* client, wl_resource* resource,
                                  int32_t width, int32_t height) {
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_set_min_size(wl_client* client, wl_resource* resource,
                                  int32_t width, int32_t height) {
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_set_maximized(wl_client* client, wl_resource* resource) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetMaximized(true);
}

void xdg_toplevel_v6_unset_maximized(wl_client* client, wl_resource* resource) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetMaximized(false);
}

void xdg_toplevel_v6_set_fullscreen(wl_client* client, wl_resource* resource,
                                    wl_resource* output) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetFullscreen(true);
}

void xdg_toplevel_v6_unset_fullscreen(wl_client* client,
                                      wl_resource* resource) {
  GetUserDataAs<ShellSurface>(resource)->window()->SetFullscreen(false);
}

void xdg_toplevel_v6_set_minimized(wl_client* client, wl_resource* resource) {
  NOTIMPLEMENTED();
}

const struct zxdg_toplevel_v6_interface xdg_toplevel_v6_implementation = {
    .destroy = xdg_toplevel_v6_destroy,
    .move = xdg_toplevel_v6_move,
    .resize = xdg_toplevel_v6_resize,
    .set_app_id = xdg_toplevel_v6_set_app_id,
    .set_fullscreen = xdg_toplevel_v6_set_fullscreen,
    .unset_fullscreen = xdg_toplevel_v6_unset_fullscreen,
    .set_max_size = xdg_toplevel_v6_set_max_size,
    .set_min_size = xdg_toplevel_v6_set_min_size,
    .set_maximized = xdg_toplevel_v6_set_maximized,
    .unset_maximized = xdg_toplevel_v6_unset_maximized,
    .set_minimized = xdg_toplevel_v6_set_minimized,
    .show_window_menu = xdg_toplevel_v6_show_window_menu,
};

///////////////////////////////////////////////////////////////////////////////
// xdg_popup_interface:

void xdg_popup_v6_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void xdg_popup_v6_grab(wl_client* client, wl_resource* resource,
                       wl_resource* seat, uint32_t serial) {
  NOTIMPLEMENTED();
}

const struct zxdg_popup_v6_interface xdg_popup_v6_implementation = {
    .destroy = xdg_popup_v6_destroy, .grab = xdg_popup_v6_grab};

///////////////////////////////////////////////////////////////////////////////
// xdg_surface_interface:

void HandleXdgToplevelV6CloseCallback(wl_resource* resource) {
  zxdg_toplevel_v6_send_close(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

void AddXdgToplevelV6State(wl_array* states, zxdg_toplevel_v6_state state) {
  zxdg_toplevel_v6_state* value = static_cast<zxdg_toplevel_v6_state*>(
      wl_array_add(states, sizeof(zxdg_toplevel_v6_state)));
  *value = state;
}

uint32_t HandleXdgToplevelV6ConfigureCallback(wl_resource* resource,
                                              wl_resource* surface_resource,
                                              int32_t width, int32_t height) {
  wl_array states;
  wl_array_init(&states);
  // TODO: handle activated state (focus in)
  zxdg_toplevel_v6_send_configure(resource, width, height, &states);
  uint32_t serial = wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(surface_resource)));
  zxdg_surface_v6_send_configure(surface_resource, serial);
  wl_client_flush(wl_resource_get_client(resource));
  wl_array_release(&states);
  return serial;
}

void xdg_surface_v6_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void xdg_surface_v6_get_toplevel(wl_client* client, wl_resource* resource,
                                 uint32_t id) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  // TODO: consider add a mapped status to window
  if (shell_surface->window()->IsManaged()) {
    wl_resource_post_error(resource, ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED,
                           "surface has already been constructed");
    return;
  }
  wm::WindowManager::Get()->Manage(shell_surface->window());
  wl_resource* xdg_toplevel_resource =
      wl_resource_create(client, &zxdg_toplevel_v6_interface, 1, id);
  shell_surface->set_close_callback(
      std::bind(&HandleXdgToplevelV6CloseCallback, resource));
  shell_surface->set_configure_callback(std::bind(
      &HandleXdgToplevelV6ConfigureCallback, resource, xdg_toplevel_resource,
      std::placeholders::_1, std::placeholders::_2));
  wl_resource_set_implementation(xdg_toplevel_resource,
                                 &xdg_toplevel_v6_implementation, shell_surface,
                                 nullptr);
}

void HandleXdgPopupV6CloseCallback(wl_resource* resource) {
  zxdg_popup_v6_send_popup_done(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

void xdg_surface_v6_get_popup(wl_client* client, wl_resource* resource,
                              uint32_t id, wl_resource* parent,
                              wl_resource* positioner) {
  ShellSurface* shell_surface = GetUserDataAs<ShellSurface>(resource);
  if (shell_surface->window()->IsManaged()) {
    wl_resource_post_error(resource, ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED,
                           "surface has already been constructed");
    return;
  }

  wm::WindowManager::Get()->Manage(shell_surface->window());

  wl_resource* xdg_popup_resource =
      wl_resource_create(client, &zxdg_popup_v6_interface, 1, id);

  shell_surface->set_close_callback(
      std::bind(&HandleXdgPopupV6CloseCallback, xdg_popup_resource));

  wl_resource_set_implementation(
      xdg_popup_resource, &xdg_popup_v6_implementation, shell_surface, nullptr);
}

void xdg_surface_v6_set_window_geometry(wl_client* client,
                                        wl_resource* resource, int32_t x,
                                        int32_t y, int32_t width,
                                        int32_t height) {
  GetUserDataAs<ShellSurface>(resource)->SetGeometry(
      base::geometry::Rect(x, y, width, height));
}

void xdg_surface_v6_ack_configure(wl_client* client, wl_resource* resource,
                                  uint32_t serial) {
  GetUserDataAs<ShellSurface>(resource)->AcknowledgeConfigure(serial);
}

const struct zxdg_surface_v6_interface xdg_surface_v6_implementation = {
    .destroy = xdg_surface_v6_destroy,
    .get_toplevel = xdg_surface_v6_get_toplevel,
    .get_popup = xdg_surface_v6_get_popup,
    .set_window_geometry = xdg_surface_v6_set_window_geometry,
    .ack_configure = xdg_surface_v6_ack_configure};

///////////////////////////////////////////////////////////////////////////////
// xdg_shell_interface:

void xdg_shell_v6_destroy(wl_client* client, wl_resource* resource) {
  // TODO: do nothing?
}

void xdg_shell_v6_create_positioner(wl_client* client, wl_resource* resource,
                                    uint32_t id) {
  wl_resource* xdg_positioner_resource =
      wl_resource_create(client, &zxdg_positioner_v6_interface, 1, id);

  wl_resource_set_implementation(xdg_positioner_resource,
                                 &xdg_positioner_v6_implementation, nullptr,
                                 nullptr);
}

void xdg_shell_v6_get_xdg_surface(wl_client* client, wl_resource* resource,
                                  uint32_t id, wl_resource* surface) {
  std::unique_ptr<ShellSurface> shell_surface =
      GetUserDataAs<Display>(resource)->CreateShellSurface(
          GetUserDataAs<Surface>(surface));
  if (!shell_surface) {
    wl_resource_post_error(resource, ZXDG_SHELL_V6_ERROR_ROLE,
                           "surface has already been assigned a role");
    return;
  }

  // Xdg shell v6 surfaces are initially disabled and needs to be explicitly
  // mapped before they are enabled and can become visible.
  wm::WindowManager::Get()->Manage(shell_surface->window());

  wl_resource* xdg_surface_resource =
      wl_resource_create(client, &zxdg_surface_v6_interface, 1, id);

  SetImplementation(xdg_surface_resource, &xdg_surface_v6_implementation,
                    std::move(shell_surface));
}

void xdg_shell_v6_pong(wl_client* client, wl_resource* resource,
                       uint32_t serial) {
  NOTIMPLEMENTED();
}

const struct zxdg_shell_v6_interface xdg_shell_v6_implementation = {
    xdg_shell_v6_destroy, xdg_shell_v6_create_positioner,
    xdg_shell_v6_get_xdg_surface, xdg_shell_v6_pong};

void bind_xdg_shell_v6(wl_client* client,
                       void* data, uint32_t version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zxdg_shell_v6_interface, 1, id);

  wl_resource_set_implementation(resource, &xdg_shell_v6_implementation, data,
                                 nullptr);
}

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// Server
Server::Server(Display* display)
    : display_(display) {
  wl_display_ = wl_display_create();
  AddSocket();
  wl_global_create(wl_display_,
                   &wl_compositor_interface,
                   3,
                   display_,
                   &bind_compositor);
  wl_global_create(wl_display_,
                   &wl_shm_interface,
                   1,
                   display_,
                   &bind_shm);
  wl_global_create(wl_display_, &wl_subcompositor_interface, 1, display_,
                   &bind_subcompositor);
  wl_global_create(wl_display_, &wl_shell_interface, 1, display_,
                   &bind_shell);
  wl_global_create(wl_display_, &wl_output_interface, 1,
                   display_, &bind_output);
  wl_global_create(wl_display_, &zxdg_shell_v6_interface, 1, display_,
                   &bind_xdg_shell_v6);
  wl_global_create(wl_display_, &wl_seat_interface, 1, display_,
                   &bind_seat);
  LOG_ERROR << "SERVER CTOR" << std::endl;
}

void Server::AddSocket() {
  wl_display_add_socket_auto(wl_display_);
}

int Server::GetFileDescriptor() {
  wl_event_loop* event_loop = wl_display_get_event_loop(wl_display_);
  assert(event_loop);
  return wl_event_loop_get_fd(event_loop);
}

void Server::DispatchEvents() {
  wl_event_loop* event_loop = wl_display_get_event_loop(wl_display_);
  assert(event_loop);
  wl_event_loop_dispatch(event_loop, 3);
  wl_display_flush_clients(wl_display_);
}

}  // namespace wayland
}  // namespace naive
