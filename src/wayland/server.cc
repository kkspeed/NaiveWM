
#include <cstdint>
#include <memory>
#include <wayland-server.h>

#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/buffer.h"
#include "compositor/region.h"
#include "compositor/surface.h"
#include "compositor/subsurface.h"
#include "wayland/display.h"
#include "wayland/shared_memory.h"

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
void SetImplementation(wl_resource* resource,
                       const void* implementation,
                       std::unique_ptr<T> user_data) {
  wl_resource_set_implementation(resource,
                                 implementation,
                                 user_data.release(),
                                 DestroyUserData<T>);
}

///////////////////////////////////////////////////////////////////////////////
// wl_buffer_interface

void buffer_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

const struct wl_buffer_interface buffer_implementation = {buffer_destroy};

void HandleBufferReleaseCallback(wl_resource* resource) {
  wl_buffer_send_release(resource);
  wl_client_flush(wl_resource_get_client(resource));
}

///////////////////////////////////////////////////////////////////////////////
// wl_surface_interface

void surface_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void surface_attach(wl_client* client,
                    wl_resource* resource,
                    wl_resource* buffer,
                    int32_t x,
                    int32_t y) {
  GetUserDataAs<Surface>(resource)
      ->Attach(buffer ? GetUserDataAs<Buffer>(buffer) : nullptr);
}

void surface_damage(wl_client* client,
                    wl_resource* resource,
                    int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height) {
  GetUserDataAs<Surface>(resource)
      ->Damage(base::geometry::Rect(x, y, width, height));
}

void surface_frame(wl_client* client,
                   wl_resource* resource,
                   uint32_t callback) {
  // TODO: implement this optimization.
}

void surface_set_opaque_region(wl_client* client,
                               wl_resource* resource,
                               wl_resource* region_resource) {
  GetUserDataAs<Surface>(resource)->SetOpaqueRegion(
      region_resource ? *GetUserDataAs<Region>(region_resource)
                      : Region::Empty());
}

void surface_set_input_region(wl_client* client,
                              wl_resource* resource,
                              wl_resource* region_resource) {
  GetUserDataAs<Surface>(resource)->SetInputRegion(
      region_resource ? *GetUserDataAs<Region>(region_resource)
                      : Region::Empty());
}

void surface_commit(wl_client* client, wl_resource* resource) {
  GetUserDataAs<Surface>(resource)->Commit();
}

void surface_set_buffer_transform(wl_client* client,
                                  wl_resource* resource,
                                  int transform) {
  NOTIMPLEMENTED();
}

void surface_set_buffer_scale(wl_client* client,
                              wl_resource* resource,
                              int32_t scale) {
  if (scale < 1) {
    wl_resource_post_error(resource,
                           WL_SURFACE_ERROR_INVALID_SCALE,
                           "buffer scale must be one (%d specified)",
                           scale);
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
    .set_input_region = surface_set_input_region
};

///////////////////////////////////////////////////////////////////////////////
// wl_region_interface
void region_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void region_add(wl_client* client,
                wl_resource* resource,
                int32_t x,
                int32_t y,
                int32_t width,
                int32_t height) {
  GetUserDataAs<Region>(resource)
      ->Union(Region(base::geometry::Rect(x, y, width, height)));
}

void region_subtract(wl_client* client,
                     wl_resource* resource,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height) {
  GetUserDataAs<Region>(resource)
      ->Subtract(Region(base::geometry::Rect(x, y, width, height)));
}

const struct wl_region_interface region_implementation = {
    .add = region_add,
    .subtract = region_subtract,
    .destroy = region_destroy
};

///////////////////////////////////////////////////////////////////////////////
// wl_compositor

void compositor_create_surface(wl_client* client,
                               wl_resource* resource,
                               uint32_t id) {
  std::unique_ptr<Surface> surface =
      GetUserDataAs<Display>(resource)->CreateSurface();
  wl_resource* surface_resource =
      wl_resource_create(client,
                         &wl_surface_interface,
                         wl_resource_get_version(resource),
                         id);
  SetImplementation(surface_resource,
                    &surface_implementation,
                    std::move(surface));
}

void compositor_create_region(wl_client* client,
                              wl_resource* resource,
                              uint32_t id) {
  wl_resource* regoin_resource =
      wl_resource_create(client, &wl_region_interface, 1, id);
  SetImplementation(resource,
                    &region_implementation,
                    std::make_unique<Region>(Region::Empty()));
}

const struct wl_compositor_interface compositor_implementation = {
    .create_region = compositor_create_region,
    .create_surface = compositor_create_surface,
};

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
  if (format != WL_SHM_FORMAT_XRGB8888 && format != WL_SHM_FORMAT_ARGB8888) {
    std::cerr << "unsupported format " << format;
    return;
  }
  std::unique_ptr<Buffer> buffer =
      GetUserDataAs<SharedMemory>(resource)
          ->CreateBuffer(width,
                         height,
                         format,
                         offset,
                         stride);
  if (!buffer) {
    std::cerr << "unable to map buffer";
    return;
  }

  wl_resource* buffer_resource =
      wl_resource_create(client, &wl_buffer_interface, 1, id);
  // TODO: buffer destroy callback
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
    .resize = shm_pool_resize
};

///////////////////////////////////////////////////////////////////////////////
// wl_shm_interface:

void shm_create_pool(wl_client* client,
                     wl_resource* resource,
                     uint32_t id,
                     int fd,
                     int32_t size) {
  std::unique_ptr<SharedMemory> shared_memory =
      GetUserDataAs<Display>(resource)->CreateSharedMemory(fd, size);
  if (!shared_memory) {
    wl_resource_post_no_memory(resource);
    return;
  }

  wl_resource* shm_pool_resource =
      wl_resource_create(client, &wl_shm_pool_interface, 1, id);
  SetImplementation(shm_pool_resource,
                    &shm_pool_implementation,
                    std::move(shared_memory));
}

const struct wl_shm_interface shm_implementation = {shm_create_pool};

///////////////////////////////////////////////////////////////////////////////
// wl_subsurface_interface:

void subsurface_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void subsurface_set_position(wl_client* client,
                             wl_resource* resource,
                             int32_t x,
                             int32_t y) {
  GetUserDataAs<SubSurface>(resource)->SetPosition(x, y);
}

void subsurface_place_above(wl_client* client,
                            wl_resource* resource,
                            wl_resource* reference) {
  GetUserDataAs<SubSurface>(resource)
      ->PlaceAbove(GetUserDataAs<Surface>(reference));
}

void subsurface_place_below(wl_client* client,
                            wl_resource* resource,
                            wl_resource* reference) {
  GetUserDataAs<SubSurface>(resource)
      ->PlaceBelow(GetUserDataAs<Surface>(reference));
}

void subsurface_set_sync(wl_client* client,
                         wl_resource* resource) {
  GetUserDataAs<SubSurface>(resource)
      ->SetCommitBehavior(true);
}

void subsurface_set_desync(wl_client* client,
                           wl_resource* resource) {
  GetUserDataAs<SubSurface>(resource)->SetCommitBehavior(false);
}

const struct wl_subsurface_interface subsurface_implementation {
    .destroy = subsurface_destroy,
    .set_position = subsurface_set_position,
    .place_above = subsurface_place_above,
    .place_below = subsurface_place_below,
    .set_desync = subsurface_set_desync,
    .set_sync = subsurface_set_sync,
};

///////////////////////////////////////////////////////////////////////////////
// wl_subcompositor_interface

void subcompositor_destroy(wl_client* client, wl_resource* resource) {
  wl_resource_destroy(resource);
}

void subcompositor_get_subsurface(wl_client* client, wl_resource* resource, uint32_t id, wl_resource* surface, wl_resource* parent) {

}



};  // namespace

}  // namespace wayland
}  // namespace naive
