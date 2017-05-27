
#include <cstdint>
#include <memory>
#include <wayland-server.h>

#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/buffer.h"
#include "compositor/region.h"
#include "compositor/surface.h"

namespace naive {
namespace wayland {

namespace {

template<class T>
T* GetUserDataAs(wl_resource* resource) {
  return static_cast<T*>(wl_resource_get_user_data(resource));
}

template<class T>
std::unique_ptr<T> TakeUserDataAs(wl_resource* resource) {
  std::unique_ptr<T> user_data = std::make_unique(GetUserDataAs<T>(resource));
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

}  // namespace

}  // namespace wayland
}  // namespace naive
