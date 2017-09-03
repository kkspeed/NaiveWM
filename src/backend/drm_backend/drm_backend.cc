#include "backend/drm_backend/drm_backend.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <drm_mode.h>
#include <fcntl.h>
#include <gbm.h>

#include <EGL/eglplatform.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <EGL/eglplatform.h>

#include "base/logging.h"
#include "base/looper.h"
#include "event/event_hub_libinput.h"
#include "resources/cursor.h"

namespace naive {
namespace backend {

namespace {
int waiting_for_flip = 0;

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
  stride = gbm_bo_get_stride(bo);
  handle = gbm_bo_get_handle(bo).u32;
  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);
  LOG_ERROR << "Bo: " << width << " " << height;

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

void* initialize_cursor() {
  void* pointer_data = nullptr;
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
    LOG_ERROR << "unable to set cusror " << strerror(errno);
    exit(1);
  }

  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = handle;
  assert(drmIoctl(drm.fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) == 0);
  pointer_data = mmap(0, creq.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm.fd,
                      mreq.offset);

  if (pointer_data == MAP_FAILED) {
    LOG_ERROR << "cannot map mouse surface";
    exit(1);
  }
  memset(pointer_data, 0, creq.size);
  memcpy(
      pointer_data, cursor_image.pixel_data,
      cursor_image.width * cursor_image.height * cursor_image.bytes_per_pixel);
  return pointer_data;
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

void finalize_draw(bool did_draw, EglContext* context) {
  if (did_draw) {
    gbm_bo* next_bo = nullptr;
    context->SwapBuffers();
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

}  // namespace

DrmBackend::DrmBackend()
    : event_hub_(std::make_unique<event::EventHubLibInput>()) {
  init_drm();
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  FD_SET(drm.fd, &fds);
  init_gbm();
  egl_ =
      std::make_unique<EglContext>(gbm.dev, gbm.surface, EGL_PLATFORM_GBM_KHR);
  egl_->SwapBuffers();
  bo = gbm_surface_lock_front_buffer(gbm.surface);
  fb = drm_fb_get_from_bo(bo);
  display_metrics_ = std::make_unique<wayland::DisplayMetrics>(
      gbm_bo_get_width(bo), gbm_bo_get_height(bo), drm.physical_width,
      drm.physical_height);
  drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0, &drm.connector_id, 1,
                 drm.mode);

  egl_->CreateDrawBuffer(display_metrics_->width_pixels,
                         display_metrics_->height_pixels);
  fb->need_modset = false;
  cursor_data_ = initialize_cursor();
}

void DrmBackend::FinalizeDraw(bool did_draw) {
  finalize_draw(did_draw, egl_.get());
}

void DrmBackend::MoveCursor(int32_t x, int32_t y) {
  move_cursor(x, y);
}

void DrmBackend::AddHandler(base::Looper* handler) {
  handler->AddFd(event_hub_->GetFileDescriptor(),
                 [this]() { this->event_hub_->HandleEvents(); });
}

}  // namespace backend
}  // namespace naive
