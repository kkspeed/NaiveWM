
#include "wayland/server.h"

#include <poll.h>
#include <signal.h>
#include <wayland-server.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>

#include "xdg-shell-unstable-v5.h"
#include "xdg-shell-unstable-v6.h"

#include "base/geometry.h"
#include "base/logging.h"
#include "base/time.h"
#include "config.h"
#include "compositor/buffer.h"
#include "compositor/compositor.h"
#include "compositor/region.h"
#include "compositor/shell_surface.h"
#include "compositor/subsurface.h"
#include "compositor/surface.h"
#include "wayland/display.h"
#include "wayland/keyboard.h"
#include "wayland/pointer.h"
#include "wm/window_manager.h"

namespace naive {
namespace wayland {

namespace {

template <class T>
T* GetUserDataAs(wl_resource* resource) {
  return static_cast<T*>(wl_resource_get_user_data(resource));
}

template <class T>
std::unique_ptr<T> TakeUserDataAs(wl_resource* resource) {
  std::unique_ptr<T> user_data = std::unique_ptr<T>(GetUserDataAs<T>(resource));
  wl_resource_set_user_data(resource, nullptr);
  return user_data;
}

template <class T>
void DestroyUserData(wl_resource* resource) {
  TakeUserDataAs<T>(resource);
}

template <class T>
void SetImplementation(wl_resource* resource,
                       const void* implementation,
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
  TRACE();
  wl_buffer_send_release(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

///////////////////////////////////////////////////////////////////////////////
// wl_surface_interface

void surface_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

void surface_attach(wl_client* client,
                    wl_resource* resource,
                    wl_resource* buffer,
                    int32_t x,
                    int32_t y) {
  TRACE();
  GetUserDataAs<Surface>(resource)->Attach(
      buffer ? GetUserDataAs<Buffer>(buffer) : nullptr);
}

void surface_damage(wl_client* client,
                    wl_resource* resource,
                    int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height) {
  TRACE();
  GetUserDataAs<Surface>(resource)->Damage(
      base::geometry::Rect(x, y, width, height));
}

void HandleSurfaceFrameCallback(wl_resource* resource) {
  TRACE();
  wl_callback_send_done(resource, base::Time::CurrentTimeMilliSeconds());
  wl_client_flush(wl_resource_get_client(resource));
  wl_resource_destroy(resource);
}

void surface_frame(wl_client* client,
                   wl_resource* resource,
                   uint32_t callback) {
  TRACE();
  wl_resource* callback_resource =
      wl_resource_create(client, &wl_callback_interface, 1, callback);
  auto frame_callback = std::make_unique<std::function<void()>>(
      std::bind(&HandleSurfaceFrameCallback, callback_resource));
  GetUserDataAs<Surface>(resource)->SetFrameCallback(frame_callback.get());
  SetImplementation(callback_resource, nullptr, std::move(frame_callback));
}

void surface_set_opaque_region(wl_client* client,
                               wl_resource* resource,
                               wl_resource* region_resource) {
  TRACE();
  GetUserDataAs<Surface>(resource)->SetOpaqueRegion(
      region_resource ? *GetUserDataAs<Region>(region_resource)
                      : Region::Empty());
}

void surface_set_input_region(wl_client* client,
                              wl_resource* resource,
                              wl_resource* region_resource) {
  TRACE();
  GetUserDataAs<Surface>(resource)->SetInputRegion(
      region_resource ? *GetUserDataAs<Region>(region_resource)
                      : Region::Empty());
}

void surface_commit(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<Surface>(resource)->Commit();
}

void surface_set_buffer_transform(wl_client* client,
                                  wl_resource* resource,
                                  int transform) {
  TRACE();
  NOTIMPLEMENTED();
}

void surface_set_buffer_scale(wl_client* client,
                              wl_resource* resource,
                              int32_t scale) {
  TRACE(" scale: %d", scale);
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
  TRACE();
  wl_resource_destroy(resource);
}

void region_add(wl_client* client,
                wl_resource* resource,
                int32_t x,
                int32_t y,
                int32_t width,
                int32_t height) {
  TRACE();
  Region region(base::geometry::Rect(x, y, width, height));
  GetUserDataAs<Region>(resource)->Union(region);
}

void region_subtract(wl_client* client,
                     wl_resource* resource,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height) {
  TRACE();
  Region region(base::geometry::Rect(x, y, width, height));
  GetUserDataAs<Region>(resource)->Subtract(region);
}

const struct wl_region_interface region_implementation = {
    .add = region_add,
    .subtract = region_subtract,
    .destroy = region_destroy};

///////////////////////////////////////////////////////////////////////////////
// wl_compositor

void compositor_create_surface(wl_client* client,
                               wl_resource* resource,
                               uint32_t id) {
  std::unique_ptr<Surface> surface =
      GetUserDataAs<Display>(resource)->CreateSurface();
  wl_resource* surface_resource = wl_resource_create(
      client, &wl_surface_interface, wl_resource_get_version(resource), id);
  surface->set_resource(surface_resource);
  TRACE("creating %p, window: %p", surface.get(), surface->window());
  SetImplementation(surface_resource, &surface_implementation,
                    std::move(surface));
}

void compositor_create_region(wl_client* client,
                              wl_resource* resource,
                              uint32_t id) {
  TRACE();
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
  TRACE();
  wl_resource* resource =
      wl_resource_create(client, &wl_compositor_interface, version, id);
  wl_resource_set_implementation(resource, &compositor_implementation, data,
                                 nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// wl_shm_pool_interface

void shm_pool_create_buffer(wl_client* client,
                            wl_resource* resource,
                            uint32_t id,
                            int32_t offset,
                            int32_t width,
                            int32_t height,
                            int32_t stride,
                            uint32_t format) {
  TRACE("create buffer: %d %d %d %d", offset, width, height, stride);
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
  buffer->set_release_callback(
      std::bind(&HandleBufferReleaseCallback, buffer_resource));
  SetImplementation(buffer_resource, &buffer_implementation, std::move(buffer));
}

void shm_pool_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

void shm_pool_resize(wl_client* client, wl_resource* resource, int32_t size) {
  TRACE();
  GetUserDataAs<SharedMemory>(resource)->Resize(static_cast<uint32_t>(size));
}

const struct wl_shm_pool_interface shm_pool_implementation = {
    .create_buffer = shm_pool_create_buffer,
    .destroy = shm_pool_destroy,
    .resize = shm_pool_resize};

///////////////////////////////////////////////////////////////////////////////
// wl_shm_interface:

void shm_create_pool(wl_client* client,
                     wl_resource* resource,
                     uint32_t id,
                     int fd,
                     int32_t size) {
  TRACE();
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
  TRACE();
  wl_resource* resource = wl_resource_create(client, &wl_shm_interface, 1, id);

  wl_resource_set_implementation(resource, &shm_implementation, data, nullptr);

  wl_shm_send_format(resource, WL_SHM_FORMAT_XRGB8888);
  wl_shm_send_format(resource, WL_SHM_FORMAT_ARGB8888);
}

///////////////////////////////////////////////////////////////////////////////
// wl_subsurface_interface:

void subsurface_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

void subsurface_set_position(wl_client* client,
                             wl_resource* resource,
                             int32_t x,
                             int32_t y) {
  TRACE();
  GetUserDataAs<SubSurface>(resource)->SetPosition(x, y);
}

void subsurface_place_above(wl_client* client,
                            wl_resource* resource,
                            wl_resource* reference) {
  TRACE();
  GetUserDataAs<SubSurface>(resource)->PlaceAbove(
      GetUserDataAs<Surface>(reference));
}

void subsurface_place_below(wl_client* client,
                            wl_resource* resource,
                            wl_resource* reference) {
  TRACE();
  GetUserDataAs<SubSurface>(resource)->PlaceBelow(
      GetUserDataAs<Surface>(reference));
}

void subsurface_set_sync(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<SubSurface>(resource)->SetCommitBehavior(true);
}

void subsurface_set_desync(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<SubSurface>(resource)->SetCommitBehavior(false);
}

const struct wl_subsurface_interface subsurface_implementation {
  .destroy = subsurface_destroy, .set_position = subsurface_set_position,
  .place_above = subsurface_place_above, .place_below = subsurface_place_below,
  .set_desync = subsurface_set_desync, .set_sync = subsurface_set_sync,
};

///////////////////////////////////////////////////////////////////////////////
// wl_subcompositor_interface

void subcompositor_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

void subcompositor_get_subsurface(wl_client* client,
                                  wl_resource* resource,
                                  uint32_t id,
                                  wl_resource* surface,
                                  wl_resource* parent) {
  TRACE();
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

void bind_subcompositor(wl_client* client,
                        void* data,
                        uint32_t version,
                        uint32_t id) {
  TRACE();
  wl_resource* resource =
      wl_resource_create(client, &wl_subcompositor_interface, 1, id);
  wl_resource_set_implementation(resource, &subcompositor_implementation, data,
                                 nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// wl_shell_surface_interface:

void shell_surface_pong(wl_client* client,
                        wl_resource* resource,
                        uint32_t serial) {
  TRACE();
  NOTIMPLEMENTED();
}

void shell_surface_move(wl_client* client,
                        wl_resource* resource,
                        wl_resource* seat_resource,
                        uint32_t serial) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->Move();
}

void shell_surface_resize(wl_client* client,
                          wl_resource* resource,
                          wl_resource* seat_resource,
                          uint32_t serial,
                          uint32_t edges) {
  TRACE();
  NOTIMPLEMENTED();
}

void shell_surface_set_toplevel(wl_client* client, wl_resource* resource) {
  TRACE();
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  if (shell_surface->window()->IsManaged())
    return;

  shell_surface->window()->set_to_be_managed(true);
  // wm::WindowManager::Get()->Manage(shell_surface->window());
}

void shell_surface_set_transient(wl_client* client,
                                 wl_resource* resource,
                                 wl_resource* parent_resource,
                                 int x,
                                 int y,
                                 uint32_t flags) {
  TRACE();
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  auto* parent_surface = GetUserDataAs<Surface>(parent_resource);
  parent_surface->window()->AddChild(shell_surface->window());
  shell_surface->window()->set_transient(true);
  // shell_surface->window()->set_to_be_managed(true);
  shell_surface->window()->SetPosition(x, y);
  // wm::WindowManager::Get()->Manage(shell_surface->window());
}

void shell_surface_set_fullscreen(wl_client* client,
                                  wl_resource* resource,
                                  uint32_t method,
                                  uint32_t framerate,
                                  wl_resource* output_resource) {
  TRACE();
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  shell_surface->window()->set_fullscreen(true);
}

void shell_surface_set_popup(wl_client* client,
                             wl_resource* resource,
                             wl_resource* seat_resource,
                             uint32_t serial,
                             wl_resource* parent_resource,
                             int32_t x,
                             int32_t y,
                             uint32_t flags) {
  TRACE();
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  auto* parent_surface = GetUserDataAs<Surface>(parent_resource);
  parent_surface->window()->AddChild(shell_surface->window());
  shell_surface->window()->set_popup(true);
  // shell_surface->window()->set_to_be_managed(true);
  shell_surface->window()->SetPosition(x, y);
  shell_surface->set_ungrab_callback(
      std::bind([](wl_client* c, wl_resource* r) {
        TRACE("ungrabbing surface");
        wl_shell_surface_send_popup_done(r);
        wl_client_flush(c);
      }, client, resource));
  wm::WindowManager::Get()->GlobalGrabWindow(shell_surface->window());
  // wm::WindowManager::Get()->Manage(shell_surface->window());
}

void shell_surface_set_maximized(wl_client* client,
                                 wl_resource* resource,
                                 wl_resource* output_resource) {
  TRACE();
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  shell_surface->window()->set_maximized(true);
}

void shell_surface_set_title(wl_client* client,
                             wl_resource* resource,
                             const char* title) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_title(
      std::string(title));
}

void shell_surface_set_class(wl_client* client,
                             wl_resource* resource,
                             const char* clazz) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_class(clazz);
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
  TRACE();
  // uint32_t serial = wl_display_next_serial(
  //     wl_client_get_display(wl_resource_get_client(resource)));
  // wl_shell_surface_send_ping(resource, serial);
  pid_t pid;
  gid_t gid;
  uid_t uid;
  wl_client_get_credentials(wl_resource_get_client(resource), &pid, &uid, &gid);
  kill(pid, SIGTERM);
  wl_client_flush(wl_resource_get_client(resource));
}

uint32_t HandleShellSurfaceConfigureCallback(wl_resource* resource,
                                             int32_t width,
                                             int32_t height) {
  TRACE();
  wl_shell_surface_send_configure(resource, WL_SHELL_SURFACE_RESIZE_NONE, width,
                                  height);
  wl_client_flush(wl_resource_get_client(resource));
  return 0;
}

void shell_get_shell_surface(wl_client* client,
                             wl_resource* resource,
                             uint32_t id,
                             wl_resource* surface) {
  TRACE();
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
  TRACE();
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
  TRACE();
  // TODO: implement set cursor.
  NOTIMPLEMENTED();
}

void pointer_release(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

const struct wl_pointer_interface pointer_implementation {
  .release = pointer_release, .set_cursor = pointer_set_cursor
};

//////////////////////////////////////////////////////////////////////////////
// wl_keyboard_interface:
void keyboard_release(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

const struct wl_keyboard_interface keyboard_implementation {
  .release = keyboard_release
};

///////////////////////////////////////////////////////////////////////////////
// wl_seat interface:

void seat_get_pointer(wl_client* client, wl_resource* resource, uint32_t id) {
  wl_resource* pointer_resource = wl_resource_create(
      client, &wl_pointer_interface, wl_resource_get_version(resource), id);
  auto pointer = std::make_unique<Pointer>(pointer_resource);
  TRACE("Getting pointer: %p, client: %p, resource: %p", pointer.get(), client, resource);
  SetImplementation(pointer_resource, &pointer_implementation,
                    std::move(pointer));
}

void seat_get_keyboard(wl_client* client, wl_resource* resource, uint32_t id) {
  TRACE();
  int version = wl_resource_get_version(resource);
  wl_resource* keyboard_resource =
      wl_resource_create(client, &wl_keyboard_interface, version, id);
  auto keyboard = std::make_unique<Keyboard>(keyboard_resource);
  TRACE("Getting keyboard: %p, client: %p, resource: %p", keyboard.get(), client, resource);
  SetImplementation(keyboard_resource, &keyboard_implementation,
                    std::move(keyboard));
  if (version >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
    wl_keyboard_send_repeat_info(keyboard_resource, 400, 500);
}

void seat_get_touch(wl_client* client, wl_resource* resource, uint32_t id) {
  TRACE();
  // TODO: no need to implement touch?
  NOTIMPLEMENTED();
}

void seat_release(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

const struct wl_seat_interface seat_implementation = {
    .get_keyboard = seat_get_keyboard,
    .get_pointer = seat_get_pointer,
    .get_touch = seat_get_touch,
    .release = seat_release};

void bind_seat(wl_client* client, void* data, uint32_t version, uint32_t id) {
  TRACE();
  wl_resource* resource =
      wl_resource_create(client, &wl_seat_interface, version, id);
  wl_resource_set_implementation(resource, &seat_implementation, data, nullptr);
  wl_seat_send_name(resource, "seat0");
  // TODO: add keyboard
  uint32_t capabilities =
      WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD;
  wl_seat_send_capabilities(resource, capabilities);
}

///////////////////////////////////////////////////////////////////////////////
// wl_output interfaces:

class WaylandOutput {
 public:
  WaylandOutput(wl_resource* resource) : output_resource_(resource) {
    SendDisplayMetrics();
  }

  void SendDisplayMetrics() {
    const float kInchInMm = 25.4f;
    const char* kUnknownMake = "unknown";
    const char* kUnknownModel = "unknown";

    // TODO: Get real display information.
    int32_t metrics[2];
    compositor::Compositor::Get()->GetDisplayMetrics(metrics);
    base::geometry::Rect bounds(0, 0, metrics[0], metrics[1]);
    wl_output_send_geometry(
        output_resource_, bounds.x(), bounds.y(),
        static_cast<int>(kInchInMm * bounds.width() / (100.0 * kScreenScale)),
        static_cast<int>(kInchInMm * bounds.height() / (100.0 * kScreenScale)),
        WL_OUTPUT_SUBPIXEL_UNKNOWN, kUnknownMake, kUnknownModel,
        WL_OUTPUT_TRANSFORM_NORMAL);

    wl_output_send_scale(output_resource_, kScreenScale);
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
  TRACE();
  wl_resource* resource =
      wl_resource_create(client, &wl_output_interface, version, id);
  SetImplementation(resource, nullptr,
                    std::make_unique<WaylandOutput>(resource));
}

///////////////////////////////////////////////////////////////////////////////
// xdg_surface_v5_interface:
void xdg_surface_v5_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

void xdg_surface_v5_set_parent(wl_client* client, wl_resource* resource,
                               wl_resource* parent) {
  TRACE();
  auto shell_surface = GetUserDataAs<ShellSurface>(resource);
  if (!parent) {
    shell_surface->window()->set_parent(nullptr);
    return;
  }
  auto parent_surface = GetUserDataAs<ShellSurface>(parent);
  parent_surface->window()->AddChild(shell_surface->window());
}

void xdg_surface_v5_set_title(wl_client* client, wl_resource* resource,
                              const char* title) {
  TRACE("title: %s", title);
  GetUserDataAs<ShellSurface>(resource)->window()->set_title(title);
}

void xdg_surface_v5_set_app_id(wl_client* client, wl_resource* resource,
                               const char* app_id) {
  TRACE("app id: %s", app_id);
  GetUserDataAs<ShellSurface>(resource)->window()->set_appid(app_id);
}

void xdg_surface_v5_show_window_menu(wl_client* client, wl_resource* resource,
                                     wl_resource* seat, uint32_t serial,
                                     int32_t x, int32_t y) {
  TRACE();
  NOTIMPLEMENTED();
}

void xdg_surface_v5_move(wl_client* client, wl_resource* resource,
                         wl_resource* seat, uint32_t serial) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->BeginMove();
}

void xdg_surface_v5_resize(wl_client* client, wl_resource* resource,
                           wl_resource* seat, uint32_t serial, uint32_t edges) {
  TRACE();
  NOTIMPLEMENTED();
}

void xdg_surface_v5_ack_configure(wl_client* client, wl_resource* resource,
                                  uint32_t serial) {
  TRACE();
  NOTIMPLEMENTED();
}

void xdg_surface_v5_set_window_geometry(
    wl_client* client, wl_resource* resource, int32_t x, int32_t y,
    int32_t width, int32_t height) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->SetVisibleRegion
      (base::geometry::Rect(x, y, width, height));
}

void xdg_surface_v5_set_maximized(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_maximized(true);
}

void xdg_surface_v5_unset_maximized(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_maximized(false);
}

void xdg_surface_v5_set_fullscreen(wl_client* client, wl_resource* resource,
                                   wl_resource* output) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_fullscreen(true);
}

void xdg_surface_v5_unset_fullscreen(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_fullscreen(false);
}

void xdg_surface_v5_set_minimized(wl_client* client, wl_resource* resource) {
  TRACE();
  NOTIMPLEMENTED();
}

const struct xdg_surface_interface xdg_surface_v5_implementation = {
    .destroy = xdg_surface_v5_destroy,
    .set_fullscreen = xdg_surface_v5_set_fullscreen,
    .unset_fullscreen = xdg_surface_v5_unset_fullscreen,
    .set_maximized = xdg_surface_v5_set_maximized,
    .unset_maximized = xdg_surface_v5_unset_maximized,
    .ack_configure = xdg_surface_v5_ack_configure,
    .move = xdg_surface_v5_move,
    .resize = xdg_surface_v5_resize,
    .set_app_id = xdg_surface_v5_set_app_id,
    .set_title = xdg_surface_v5_set_title,
    .set_window_geometry = xdg_surface_v5_set_window_geometry,
    .set_parent = xdg_surface_v5_set_parent,
    .show_window_menu = xdg_surface_v5_show_window_menu,
    .set_minimized = xdg_surface_v5_set_minimized
};

//////////////////////////////////////////////////////////////////////////////
// xdg_shell_v5_popup_interface:
void xdg_popup_v5_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

const struct xdg_popup_interface xdg_popup_v5_implementation = {
    .destroy = xdg_popup_v5_destroy
};

///////////////////////////////////////////////////////////////////////////////
// xdg_shell_v5_interface:

void xdg_shell_v5_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  // DO NOTHING.
}

void xdg_shell_v5_use_unstable_version(wl_client* client,
                                       wl_resource* resource, int32_t version) {
  TRACE();
  NOTIMPLEMENTED();
}

void HandleXdgSurfaceV5CloseCallback(wl_resource* resource) {
  xdg_surface_send_close(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

void AddXdgSurfaceV5State(wl_array* states, xdg_surface_state state) {
  xdg_surface_state* value = static_cast<xdg_surface_state*>(
      wl_array_add(states, sizeof(xdg_surface_state)));
  assert(value);
  *value = state;
}

uint32_t HandleXdgSurfaceV5ConfigureCallback(
    wl_resource* resource,
    int32_t width,
    int32_t height) {
  wl_array states;
  wl_array_init(&states);
  AddXdgSurfaceV5State(&states, XDG_SURFACE_STATE_MAXIMIZED);
  AddXdgSurfaceV5State(&states, XDG_SURFACE_STATE_ACTIVATED);
  uint32_t serial = wl_display_next_serial(
      wl_client_get_display(wl_resource_get_client(resource)));
  xdg_surface_send_configure(resource, width, height, &states, serial);
  wl_client_flush(wl_resource_get_client(resource));
  wl_array_release(&states);
  return serial;
}

void xdg_shell_v5_get_xdg_surface(wl_client* client, wl_resource* resource,
                                  uint32_t id, wl_resource* surface) {
  TRACE();
  auto shell_surface = GetUserDataAs<Display>(resource)->CreateShellSurface(
      GetUserDataAs<Surface>(surface));
  wl_resource* xdg_surface_resource = wl_resource_create(
      client, &xdg_surface_interface, 1, id);
  shell_surface->set_close_callback(
      std::bind(&HandleXdgSurfaceV5CloseCallback, xdg_surface_resource));
  shell_surface->set_configure_callback(
      std::bind(&HandleXdgSurfaceV5ConfigureCallback, resource,
                std::placeholders::_1, std::placeholders::_2));

  shell_surface->window()->set_to_be_managed(true);
  SetImplementation(xdg_surface_resource, &xdg_surface_v5_implementation,
                    std::move(shell_surface));
}

void HandleXdgPopupV5CloseCallback(wl_resource* resource) {
  TRACE();
  xdg_popup_send_popup_done(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

void xdg_shell_v5_get_popup(wl_client* client,
                            wl_resource* resource,
                            uint32_t id,
                            wl_resource* surface,
                            wl_resource* parent,
                            wl_resource* seat,
                            uint32_t serial,
                            int32_t x,
                            int32_t y) {
  TRACE();
  auto* parent_surface = GetUserDataAs<ShellSurface>(parent);
  wl_resource* xdg_popup_resource =
      wl_resource_create(client, &xdg_popup_interface, 1, id);
  auto shell_surface =
      GetUserDataAs<Display>(resource)
          ->CreateShellSurface(GetUserDataAs<Surface>(surface));
  parent_surface->window()->AddChild(shell_surface->window());
  shell_surface->window()->SetPosition(x, y);
  shell_surface->set_close_callback(
      std::bind(HandleXdgPopupV5CloseCallback, xdg_popup_resource));
  SetImplementation(xdg_popup_resource, &xdg_popup_v5_implementation,
                    std::move(shell_surface));
}

void xdg_shell_v5_pong(wl_client* client,
                       wl_resource* resource,
                       uint32_t serial) {
  NOTIMPLEMENTED();
}

const struct xdg_shell_interface xdg_shell_v5_implementation = {
    .destroy = xdg_shell_v5_destroy,
    .use_unstable_version = xdg_shell_v5_use_unstable_version,
    .get_xdg_surface = xdg_shell_v5_get_xdg_surface,
    .get_xdg_popup = xdg_shell_v5_get_popup,
    .pong = xdg_shell_v5_pong
};

void bind_xdg_shell_v5(wl_client* client,
                       void* data,
                       uint32_t version,
                       uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &xdg_shell_interface, 1, id);

  wl_resource_set_implementation(resource, &xdg_shell_v5_implementation,
                                 data, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// xdg_positioner_interface:

class XdgPositioner {
 public:
  XdgPositioner()
      : width_(0),
        height_(0),
        x_(0),
        y_(0),
        anchor_rect_(base::geometry::Rect(0, 0, 0, 0)),
        constraint_adjustment_(0),
        anchor_(0),
        gravity_(0) {}

  base::geometry::Rect bounds() {
    int32_t anchor_x = anchor_rect_.x() + anchor_rect_.width() / 2;
    int32_t anchor_y = anchor_rect_.y() + anchor_rect_.height() / 2;

    if (anchor_ & ZXDG_POSITIONER_V6_ANCHOR_LEFT)
      anchor_x = anchor_rect_.x();

    if (anchor_ & ZXDG_POSITIONER_V6_ANCHOR_TOP)
      anchor_y = anchor_rect_.y();

    if (anchor_ & ZXDG_POSITIONER_V6_ANCHOR_RIGHT)
      anchor_x = anchor_rect_.x() + anchor_rect_.width();

    if (anchor_ & ZXDG_POSITIONER_V6_ANCHOR_BOTTOM)
      anchor_y = anchor_rect_.y() + anchor_rect_.height();

    auto result = base::geometry::Rect(anchor_x - width_ / 2,
                                       anchor_y - height_ / 2, width_, height_);
    if (gravity_ & ZXDG_POSITIONER_V6_GRAVITY_RIGHT)
      result.x_ = anchor_x;

    if (gravity_ & ZXDG_POSITIONER_V6_GRAVITY_BOTTOM)
      result.y_ = anchor_y;

    if (gravity_ & ZXDG_POSITIONER_V6_GRAVITY_TOP)
      result.y_ = anchor_y - height_;

    if (gravity_ & ZXDG_POSITIONER_V6_GRAVITY_LEFT)
      result.x_ = anchor_x - width_;

    result.x_ += x_;
    result.y_ += y_;
    TRACE("bounds: %d %d w: %d h: %d", result.x(), result.y(), result.width(),
          result.height());
    return result;
  }

  base::geometry::Rect anchor_rect() { return anchor_rect_; }
  uint32_t anchor() { return anchor_; }
  void set_offset(int32_t x, int32_t y) {
    TRACE("offset: %d %d", x, y);
    x_ = x;
    y_ = y;
  }
  void set_size(int32_t width, int32_t height) {
    width_ = width;
    height_ = height;
  }
  void set_anchor_rect(base::geometry::Rect rect) {
    TRACE("anchor: %d %d, w: %d, h: %d", rect.x(), rect.y(), rect.width(),
          rect.height());
    anchor_rect_ = rect;
  }
  void set_anchor(uint32_t anchor) { anchor_ = anchor; }
  void set_constraint_adjustment(uint32_t c) { constraint_adjustment_ = c; }
  void set_gravity(uint32_t gravity) { gravity_ = gravity; }

 private:
  int32_t width_, height_, x_, y_;
  base::geometry::Rect anchor_rect_;
  uint32_t constraint_adjustment_;
  uint32_t anchor_, gravity_;
};

void xdg_positioner_v6_destroy(wl_client* client, wl_resource* resource) {
  TRACE();
  wl_resource_destroy(resource);
}

void xdg_positioner_v6_set_size(wl_client* client,
                                wl_resource* resource,
                                int32_t width,
                                int32_t height) {
  TRACE();
  auto* positioner = GetUserDataAs<XdgPositioner>(resource);
  positioner->set_size(width, height);
}

void xdg_positioner_v6_set_anchor_rect(wl_client* client,
                                       wl_resource* resource,
                                       int32_t x,
                                       int32_t y,
                                       int32_t width,
                                       int32_t height) {
  TRACE();
  GetUserDataAs<XdgPositioner>(resource)->set_anchor_rect(
      base::geometry::Rect(x, y, width, height));
}

void xdg_positioner_v6_set_anchor(wl_client* client,
                                  wl_resource* resource,
                                  uint32_t anchor) {
  TRACE();
  GetUserDataAs<XdgPositioner>(resource)->set_anchor(anchor);
}

void xdg_positioner_v6_set_gravity(wl_client* client,
                                   wl_resource* resource,
                                   uint32_t gravity) {
  TRACE();
  GetUserDataAs<XdgPositioner>(resource)->set_gravity(gravity);
}

void xdg_positioner_v6_set_constraint_adjustment(
    wl_client* client,
    wl_resource* resource,
    uint32_t constraint_adjustment) {
  TRACE();
  GetUserDataAs<XdgPositioner>(resource)->set_constraint_adjustment(
      constraint_adjustment);
}

void xdg_positioner_v6_set_offset(wl_client* client,
                                  wl_resource* resource,
                                  int32_t x,
                                  int32_t y) {
  TRACE();
  GetUserDataAs<XdgPositioner>(resource)->set_offset(x, y);
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
  TRACE();
  wl_resource_destroy(resource);
}

void xdg_toplevel_v6_set_parent(wl_client* client,
                                wl_resource* resource,
                                wl_resource* parent) {
  TRACE("%p", parent);
  if (!parent) {
    GetUserDataAs<ShellSurface>(resource)->window()->set_parent(nullptr);
    return;
  }

  GetUserDataAs<ShellSurface>(parent)->window()->AddChild(
      GetUserDataAs<ShellSurface>(resource)->window());
}

void xdg_toplevel_v6_set_title(wl_client* client,
                               wl_resource* resource,
                               const char* title) {
  TRACE("%s", title);
  GetUserDataAs<ShellSurface>(resource)->window()->set_title(
      std::string(title));
}

void xdg_toplevel_v6_set_app_id(wl_client* client,
                                wl_resource* resource,
                                const char* app_id) {
  TRACE("%s", app_id);
  GetUserDataAs<ShellSurface>(resource)->window()->set_appid(
      std::string(app_id));
}

void xdg_toplevel_v6_show_window_menu(wl_client* client,
                                      wl_resource* resource,
                                      wl_resource* seat,
                                      uint32_t serial,
                                      int32_t x,
                                      int32_t y) {
  TRACE();
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_move(wl_client* client,
                          wl_resource* resource,
                          wl_resource* seat,
                          uint32_t serial) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->Move();
}

void xdg_toplevel_v6_resize(wl_client* client,
                            wl_resource* resource,
                            wl_resource* seat,
                            uint32_t serial,
                            uint32_t edges) {
  TRACE();
  // TODO: Implement resize.
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_set_max_size(wl_client* client,
                                  wl_resource* resource,
                                  int32_t width,
                                  int32_t height) {
  TRACE("maxsize: %d %d", width, height);
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_set_min_size(wl_client* client,
                                  wl_resource* resource,
                                  int32_t width,
                                  int32_t height) {
  TRACE("minsize: %d %d", width, height);
  NOTIMPLEMENTED();
}

void xdg_toplevel_v6_set_maximized(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_maximized(true);
}

void xdg_toplevel_v6_unset_maximized(wl_client* client, wl_resource* resource) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_maximized(false);
}

void xdg_toplevel_v6_set_fullscreen(wl_client* client,
                                    wl_resource* resource,
                                    wl_resource* output) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_fullscreen(true);
}

void xdg_toplevel_v6_unset_fullscreen(wl_client* client,
                                      wl_resource* resource) {
  TRACE();
  GetUserDataAs<ShellSurface>(resource)->window()->set_fullscreen(false);
}

void xdg_toplevel_v6_set_minimized(wl_client* client, wl_resource* resource) {
  TRACE();
  NOTIMPLEMENTED();
}

const struct zxdg_toplevel_v6_interface xdg_toplevel_v6_implementation = {
    .destroy = xdg_toplevel_v6_destroy,
    .set_parent = xdg_toplevel_v6_set_parent,
    .set_title = xdg_toplevel_v6_set_title,
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
  TRACE();
  wl_resource_destroy(resource);
}

void xdg_popup_v6_grab(wl_client* client,
                       wl_resource* resource,
                       wl_resource* seat,
                       uint32_t serial) {
  TRACE();
  zxdg_surface_v6_send_configure(resource, serial);
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  shell_surface->set_ungrab_callback(std::bind(&zxdg_popup_v6_send_popup_done,
                                               resource));
  wm::WindowManager::Get()->GlobalGrabWindow(shell_surface->window());
}

const struct zxdg_popup_v6_interface xdg_popup_v6_implementation = {
    .destroy = xdg_popup_v6_destroy,
    .grab = xdg_popup_v6_grab};

///////////////////////////////////////////////////////////////////////////////
// xdg_surface_interface:

void HandleXdgToplevelV6CloseCallback(wl_resource* resource) {
  TRACE();
  zxdg_toplevel_v6_send_close(resource);
}

void AddXdgToplevelV6State(wl_array* states, zxdg_toplevel_v6_state state) {
  TRACE();
  zxdg_toplevel_v6_state* value = static_cast<zxdg_toplevel_v6_state*>(
      wl_array_add(states, sizeof(zxdg_toplevel_v6_state)));
  *value = state;
}

uint32_t HandleXdgToplevelV6ConfigureCallback(wl_resource* resource,
                                              wl_resource* surface_resource,
                                              int32_t width,
                                              int32_t height) {
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  TRACE("configure: window %p, %d %d", shell_surface->window(), width, height);
  wl_array states;
  wl_array_init(&states);
  // TODO: if window is visible, we'll need maximized state to eliminate window
  // decorations.
  AddXdgToplevelV6State(&states, ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED);

  // TODO: only for debugging, this needs to be activated via mouse focus
  if (shell_surface->window()->focused())
    AddXdgToplevelV6State(&states, ZXDG_TOPLEVEL_V6_STATE_ACTIVATED);
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
  TRACE();
  wl_resource_destroy(resource);
}

void xdg_surface_v6_get_toplevel(wl_client* client,
                                 wl_resource* resource,
                                 uint32_t id) {
  TRACE();
  auto* shell_surface = GetUserDataAs<ShellSurface>(resource);
  // TODO: consider add a mapped status to window
  /*
if (shell_surface->window()->IsManaged()) {
  wl_resource_post_error(resource, ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED,
                         "surface has already been constructed");
    return;
  }
                         */
  wl_resource* xdg_toplevel_resource =
      wl_resource_create(client, &zxdg_toplevel_v6_interface, 1, id);
  shell_surface->set_close_callback(
      std::bind(&HandleXdgToplevelV6CloseCallback, xdg_toplevel_resource));
  shell_surface->set_configure_callback(
      std::bind(&HandleXdgToplevelV6ConfigureCallback, xdg_toplevel_resource,
                resource, std::placeholders::_1, std::placeholders::_2));
  wl_resource_set_implementation(xdg_toplevel_resource,
                                 &xdg_toplevel_v6_implementation, shell_surface,
                                 nullptr);

  shell_surface->window()->set_to_be_managed(true);
  // wm::WindowManager::Get()->Manage(shell_surface->window());
}

void HandleXdgPopupV6CloseCallback(wl_resource* resource) {
  TRACE();
  zxdg_popup_v6_send_popup_done(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

void xdg_surface_v6_get_popup(wl_client* client,
                              wl_resource* resource,
                              uint32_t id,
                              wl_resource* parent,
                              wl_resource* positioner) {
  TRACE();
  ShellSurface* shell_surface = GetUserDataAs<ShellSurface>(resource);
  zxdg_surface_v6_send_configure(
      resource, wl_display_next_serial(wl_client_get_display(client)));

  /*
if (!shell_surface->window()->IsManaged()) {
  wl_resource_post_error(resource, ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED,
                         "surface has already been constructed");
    return;
  }*/

  // shell_surface->window()->set_popup(true);
  wl_resource* xdg_popup_resource =
      wl_resource_create(client, &zxdg_popup_v6_interface, 1, id);

  shell_surface->set_close_callback(
      std::bind(&HandleXdgPopupV6CloseCallback, xdg_popup_resource));
  shell_surface->set_configure_callback([](uint32_t, uint32_t){TRACE(); return 0; });

  wl_resource_set_implementation(
      xdg_popup_resource, &xdg_popup_v6_implementation, shell_surface, nullptr);
  auto* xdg_positioner = GetUserDataAs<XdgPositioner>(positioner);
  auto bounds = xdg_positioner->bounds();
  TRACE("popup bounds %d %d, w: %d, h: %d", bounds.x(), bounds.y(),
        bounds.width(), bounds.height());
  shell_surface->SetGeometry(
      base::geometry::Rect(bounds.x(), bounds.y(), bounds.width(),
                           bounds.height()));  // xdg_positioner->bounds());

  ShellSurface* parent_surface = GetUserDataAs<ShellSurface>(parent);
  parent_surface->window()->AddChild(shell_surface->window());
  /*
  auto parent_bounds = parent_surface->window()->geometry();
  int32_t parent_x = parent_surface->window()->wm_x();
  int32_t parent_y = parent_surface->window()->wm_y();
  shell_surface->window()->WmSetPosition(
      parent_x + parent_bounds.x() + bounds.x(),
      parent_y + parent_bounds.y() + bounds.y());
  shell_surface->window()->set_popup(true);
  shell_surface->window()->set_to_be_managed(true);
   */
  TRACE("bounds: %d %d, w: %d, h: %d, parent: %p", bounds.x(), bounds.y(),
        bounds.width(), bounds.height(), parent_surface->window());
  zxdg_popup_v6_send_configure(xdg_popup_resource, bounds.x(), bounds.y(),
                               bounds.width(), bounds.height());
  // wm::WindowManager::Get()->Manage(shell_surface->window());
}

void xdg_surface_v6_set_window_geometry(wl_client* client,
                                        wl_resource* resource,
                                        int32_t x,
                                        int32_t y,
                                        int32_t width,
                                        int32_t height) {
  TRACE();
  // TODO: we should separate window offset and geometry.
  GetUserDataAs<ShellSurface>(resource)->SetVisibleRegion(
      base::geometry::Rect(x, y, width, height));
}

void xdg_surface_v6_ack_configure(wl_client* client,
                                  wl_resource* resource,
                                  uint32_t serial) {
  TRACE();
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
  TRACE();
}

void xdg_shell_v6_create_positioner(wl_client* client,
                                    wl_resource* resource,
                                    uint32_t id) {
  TRACE();
  wl_resource* xdg_positioner_resource =
      wl_resource_create(client, &zxdg_positioner_v6_interface, 1, id);

  SetImplementation(xdg_positioner_resource, &xdg_positioner_v6_implementation,
                    std::make_unique<XdgPositioner>());
}

void xdg_shell_v6_get_xdg_surface(wl_client* client,
                                  wl_resource* resource,
                                  uint32_t id,
                                  wl_resource* surface) {
  TRACE();
  std::unique_ptr<ShellSurface> shell_surface =
      GetUserDataAs<Display>(resource)->CreateShellSurface(
          GetUserDataAs<Surface>(surface));
  if (!shell_surface) {
    wl_resource_post_error(resource, ZXDG_SHELL_V6_ERROR_ROLE,
                           "surface has already been assigned a role");
    return;
  }

  // TODO: Xdg shell v6 surfaces are initially disabled and needs to be
  // explicitly mapped before they are enabled and can become visible.
  // wm::WindowManager::Get()->Manage(shell_surface->window());

  wl_resource* xdg_surface_resource =
      wl_resource_create(client, &zxdg_surface_v6_interface, 1, id);

  zxdg_surface_v6_send_configure(
      xdg_surface_resource,
      wl_display_next_serial(wl_client_get_display(client)));

  SetImplementation(xdg_surface_resource, &xdg_surface_v6_implementation,
                    std::move(shell_surface));
}

void xdg_shell_v6_pong(wl_client* client,
                       wl_resource* resource,
                       uint32_t serial) {
  TRACE();
  NOTIMPLEMENTED();
}

const struct zxdg_shell_v6_interface xdg_shell_v6_implementation = {
    xdg_shell_v6_destroy, xdg_shell_v6_create_positioner,
    xdg_shell_v6_get_xdg_surface, xdg_shell_v6_pong};

void bind_xdg_shell_v6(wl_client* client,
                       void* data,
                       uint32_t version,
                       uint32_t id) {
  TRACE();
  wl_resource* resource =
      wl_resource_create(client, &zxdg_shell_v6_interface, 1, id);

  wl_resource_set_implementation(resource, &xdg_shell_v6_implementation, data,
                                 nullptr);
}

////////////////////////////////////////////////////////////////////////////////
// wl_data_device_interface:

void data_device_start_drag(wl_client* client,
                            wl_resource* resource,
                            wl_resource* source_resource,
                            wl_resource* origin_resource,
                            wl_resource* icon_resource,
                            uint32_t serial) {
  TRACE();
  NOTIMPLEMENTED();
}

void data_device_set_selection(wl_client* client,
                               wl_resource* resource,
                               wl_resource* data_source,
                               uint32_t serial) {
  TRACE();
  NOTIMPLEMENTED();
}

const struct wl_data_device_interface data_device_implementation = {
    data_device_start_drag, data_device_set_selection};

//////////////////////////////////////////////////////////////////////////////
// wl_data_device_manager_interface:

void data_device_manager_create_data_source(wl_client* client,
                                            wl_resource* resource,
                                            uint32_t id) {
  // TODO: implement data source
  TRACE();
  NOTIMPLEMENTED();
}

void data_device_manager_get_data_device(wl_client* client,
                                         wl_resource* resource,
                                         uint32_t id,
                                         wl_resource* seat_resource) {
  // TODO: implement data device
  TRACE();
  wl_resource* data_device_resource =
      wl_resource_create(client, &wl_data_device_interface, 1, id);

  wl_resource_set_implementation(data_device_resource,
                                 &data_device_implementation, nullptr, nullptr);
}

const struct wl_data_device_manager_interface
    data_device_manager_implementation = {
        data_device_manager_create_data_source,
        data_device_manager_get_data_device};

void bind_data_device_manager(wl_client* client,
                              void* data,
                              uint32_t version,
                              uint32_t id) {
  TRACE();
  wl_resource* resource =
      wl_resource_create(client, &wl_data_device_manager_interface, 1, id);
  wl_resource_set_implementation(resource, &data_device_manager_implementation,
                                 data, nullptr);
}

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// Server
Server::Server(Display* display) : display_(display) {
  wl_display_ = wl_display_create();
  AddSocket();
  wl_global_create(wl_display_, &wl_compositor_interface, 3, display_,
                   &bind_compositor);
  wl_global_create(wl_display_, &wl_shm_interface, 1, display_, &bind_shm);
  wl_global_create(wl_display_, &wl_subcompositor_interface, 1, display_,
                   &bind_subcompositor);
  wl_global_create(wl_display_, &wl_shell_interface, 1, display_, &bind_shell);
  wl_global_create(wl_display_, &wl_output_interface, 2, display_,
                   &bind_output);
  wl_global_create(wl_display_, &zxdg_shell_v6_interface, 1, display_,
                   &bind_xdg_shell_v6);
  wl_global_create(wl_display_, &xdg_shell_interface, 1, display_,
                   &bind_xdg_shell_v5);
  wl_global_create(wl_display_, &wl_seat_interface, 1, display_, &bind_seat);
  wl_global_create(wl_display_, &wl_data_device_manager_interface, 1, display_,
                   bind_data_device_manager);
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
