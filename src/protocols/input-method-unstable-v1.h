/* Generated by wayland-scanner 1.14.0 */

#ifndef INPUT_METHOD_UNSTABLE_V1_SERVER_PROTOCOL_H
#define INPUT_METHOD_UNSTABLE_V1_SERVER_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "wayland-server.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

/**
 * @page page_input_method_unstable_v1 The input_method_unstable_v1 protocol
 * @section page_ifaces_input_method_unstable_v1 Interfaces
 * - @subpage page_iface_zwp_input_method_context_v1 - input method context
 * - @subpage page_iface_zwp_input_method_v1 - input method
 * - @subpage page_iface_zwp_input_panel_v1 - interface for implementing
 * keyboards
 * - @subpage page_iface_zwp_input_panel_surface_v1 -
 * @section page_copyright_input_method_unstable_v1 Copyright
 * <pre>
 *
 * Copyright © 2012, 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * </pre>
 */
struct wl_keyboard;
struct wl_output;
struct wl_surface;
struct zwp_input_method_context_v1;
struct zwp_input_method_v1;
struct zwp_input_panel_surface_v1;
struct zwp_input_panel_v1;

/**
 * @page page_iface_zwp_input_method_context_v1 zwp_input_method_context_v1
 * @section page_iface_zwp_input_method_context_v1_desc Description
 *
 * Corresponds to a text input on the input method side. An input method context
 * is created on text input activation on the input method side. It allows
 * receiving information about the text input from the application via events.
 * Input method contexts do not keep state after deactivation and should be
 * destroyed after deactivation is handled.
 *
 * Text is generally UTF-8 encoded, indices and lengths are in bytes.
 *
 * Serials are used to synchronize the state between the text input and
 * an input method. New serials are sent by the text input in the
 * commit_state request and are used by the input method to indicate
 * the known text input state in events like preedit_string, commit_string,
 * and keysym. The text input can then ignore events from the input method
 * which are based on an outdated state (for example after a reset).
 *
 * Warning! The protocol described in this file is experimental and
 * backward incompatible changes may be made. Backward compatible changes
 * may be added together with the corresponding interface version bump.
 * Backward incompatible changes are done by bumping the version number in
 * the protocol and interface names and resetting the interface version.
 * Once the protocol is to be declared stable, the 'z' prefix and the
 * version number in the protocol and interface names are removed and the
 * interface version number is reset.
 * @section page_iface_zwp_input_method_context_v1_api API
 * See @ref iface_zwp_input_method_context_v1.
 */
/**
 * @defgroup iface_zwp_input_method_context_v1 The zwp_input_method_context_v1
 * interface
 *
 * Corresponds to a text input on the input method side. An input method context
 * is created on text input activation on the input method side. It allows
 * receiving information about the text input from the application via events.
 * Input method contexts do not keep state after deactivation and should be
 * destroyed after deactivation is handled.
 *
 * Text is generally UTF-8 encoded, indices and lengths are in bytes.
 *
 * Serials are used to synchronize the state between the text input and
 * an input method. New serials are sent by the text input in the
 * commit_state request and are used by the input method to indicate
 * the known text input state in events like preedit_string, commit_string,
 * and keysym. The text input can then ignore events from the input method
 * which are based on an outdated state (for example after a reset).
 *
 * Warning! The protocol described in this file is experimental and
 * backward incompatible changes may be made. Backward compatible changes
 * may be added together with the corresponding interface version bump.
 * Backward incompatible changes are done by bumping the version number in
 * the protocol and interface names and resetting the interface version.
 * Once the protocol is to be declared stable, the 'z' prefix and the
 * version number in the protocol and interface names are removed and the
 * interface version number is reset.
 */
extern const struct wl_interface zwp_input_method_context_v1_interface;
/**
 * @page page_iface_zwp_input_method_v1 zwp_input_method_v1
 * @section page_iface_zwp_input_method_v1_desc Description
 *
 * An input method object is responsible for composing text in response to
 * input from hardware or virtual keyboards. There is one input method
 * object per seat. On activate there is a new input method context object
 * created which allows the input method to communicate with the text input.
 * @section page_iface_zwp_input_method_v1_api API
 * See @ref iface_zwp_input_method_v1.
 */
/**
 * @defgroup iface_zwp_input_method_v1 The zwp_input_method_v1 interface
 *
 * An input method object is responsible for composing text in response to
 * input from hardware or virtual keyboards. There is one input method
 * object per seat. On activate there is a new input method context object
 * created which allows the input method to communicate with the text input.
 */
extern const struct wl_interface zwp_input_method_v1_interface;
/**
 * @page page_iface_zwp_input_panel_v1 zwp_input_panel_v1
 * @section page_iface_zwp_input_panel_v1_desc Description
 *
 * Only one client can bind this interface at a time.
 * @section page_iface_zwp_input_panel_v1_api API
 * See @ref iface_zwp_input_panel_v1.
 */
/**
 * @defgroup iface_zwp_input_panel_v1 The zwp_input_panel_v1 interface
 *
 * Only one client can bind this interface at a time.
 */
extern const struct wl_interface zwp_input_panel_v1_interface;
/**
 * @page page_iface_zwp_input_panel_surface_v1 zwp_input_panel_surface_v1
 * @section page_iface_zwp_input_panel_surface_v1_api API
 * See @ref iface_zwp_input_panel_surface_v1.
 */
/**
 * @defgroup iface_zwp_input_panel_surface_v1 The zwp_input_panel_surface_v1
 * interface
 */
extern const struct wl_interface zwp_input_panel_surface_v1_interface;

/**
 * @ingroup iface_zwp_input_method_context_v1
 * @struct zwp_input_method_context_v1_interface
 */
struct zwp_input_method_context_v1_interface {
  /**
   */
  void (*destroy)(struct wl_client* client, struct wl_resource* resource);
  /**
   * commit string
   *
   * Send the commit string text for insertion to the application.
   *
   * The text to commit could be either just a single character after
   * a key press or the result of some composing (pre-edit). It could
   * be also an empty text when some text should be removed (see
   * delete_surrounding_text) or when the input cursor should be
   * moved (see cursor_position).
   *
   * Any previously set composing text will be removed.
   * @param serial serial of the latest known text input state
   */
  void (*commit_string)(struct wl_client* client,
                        struct wl_resource* resource,
                        uint32_t serial,
                        const char* text);
  /**
   * pre-edit string
   *
   * Send the pre-edit string text to the application text input.
   *
   * The commit text can be used to replace the pre-edit text on
   * reset (for example on unfocus).
   *
   * Previously sent preedit_style and preedit_cursor requests are
   * also processed by the text_input.
   * @param serial serial of the latest known text input state
   */
  void (*preedit_string)(struct wl_client* client,
                         struct wl_resource* resource,
                         uint32_t serial,
                         const char* text,
                         const char* commit);
  /**
   * pre-edit styling
   *
   * Set the styling information on composing text. The style is
   * applied for length in bytes from index relative to the beginning
   * of the composing text (as byte offset). Multiple styles can be
   * applied to a composing text.
   *
   * This request should be sent before sending a preedit_string
   * request.
   */
  void (*preedit_styling)(struct wl_client* client,
                          struct wl_resource* resource,
                          uint32_t index,
                          uint32_t length,
                          uint32_t style);
  /**
   * pre-edit cursor
   *
   * Set the cursor position inside the composing text (as byte
   * offset) relative to the start of the composing text.
   *
   * When index is negative no cursor should be displayed.
   *
   * This request should be sent before sending a preedit_string
   * request.
   */
  void (*preedit_cursor)(struct wl_client* client,
                         struct wl_resource* resource,
                         int32_t index);
  /**
   * delete text
   *
   * Remove the surrounding text.
   *
   * This request will be handled on the text_input side directly
   * following a commit_string request.
   */
  void (*delete_surrounding_text)(struct wl_client* client,
                                  struct wl_resource* resource,
                                  int32_t index,
                                  uint32_t length);
  /**
   * set cursor to a new position
   *
   * Set the cursor and anchor to a new position. Index is the new
   * cursor position in bytes (when >= 0 this is relative to the end
   * of the inserted text, otherwise it is relative to the beginning
   * of the inserted text). Anchor is the new anchor position in
   * bytes (when >= 0 this is relative to the end of the inserted
   * text, otherwise it is relative to the beginning of the inserted
   * text). When there should be no selected text, anchor should be
   * the same as index.
   *
   * This request will be handled on the text_input side directly
   * following a commit_string request.
   */
  void (*cursor_position)(struct wl_client* client,
                          struct wl_resource* resource,
                          int32_t index,
                          int32_t anchor);
  /**
   */
  void (*modifiers_map)(struct wl_client* client,
                        struct wl_resource* resource,
                        struct wl_array* map);
  /**
   * keysym
   *
   * Notify when a key event was sent. Key events should not be
   * used for normal text input operations, which should be done with
   * commit_string, delete_surrounding_text, etc. The key event
   * follows the wl_keyboard key event convention. Sym is an XKB
   * keysym, state is a wl_keyboard key_state.
   * @param serial serial of the latest known text input state
   */
  void (*keysym)(struct wl_client* client,
                 struct wl_resource* resource,
                 uint32_t serial,
                 uint32_t time,
                 uint32_t sym,
                 uint32_t state,
                 uint32_t modifiers);
  /**
   * grab hardware keyboard
   *
   * Allow an input method to receive hardware keyboard input and
   * process key events to generate text events (with pre-edit) over
   * the wire. This allows input methods which compose multiple key
   * events for inputting text like it is done for CJK languages.
   */
  void (*grab_keyboard)(struct wl_client* client,
                        struct wl_resource* resource,
                        uint32_t keyboard);
  /**
   * forward key event
   *
   * Forward a wl_keyboard::key event to the client that was not
   * processed by the input method itself. Should be used when
   * filtering key events with grab_keyboard. The arguments should be
   * the ones from the wl_keyboard::key event.
   *
   * For generating custom key events use the keysym request instead.
   * @param serial serial from wl_keyboard::key
   * @param time time from wl_keyboard::key
   * @param key key from wl_keyboard::key
   * @param state state from wl_keyboard::key
   */
  void (*key)(struct wl_client* client,
              struct wl_resource* resource,
              uint32_t serial,
              uint32_t time,
              uint32_t key,
              uint32_t state);
  /**
   * forward modifiers event
   *
   * Forward a wl_keyboard::modifiers event to the client that was
   * not processed by the input method itself. Should be used when
   * filtering key events with grab_keyboard. The arguments should be
   * the ones from the wl_keyboard::modifiers event.
   * @param serial serial from wl_keyboard::modifiers
   * @param mods_depressed mods_depressed from wl_keyboard::modifiers
   * @param mods_latched mods_latched from wl_keyboard::modifiers
   * @param mods_locked mods_locked from wl_keyboard::modifiers
   * @param group group from wl_keyboard::modifiers
   */
  void (*modifiers)(struct wl_client* client,
                    struct wl_resource* resource,
                    uint32_t serial,
                    uint32_t mods_depressed,
                    uint32_t mods_latched,
                    uint32_t mods_locked,
                    uint32_t group);
  /**
   * @param serial serial of the latest known text input state
   */
  void (*language)(struct wl_client* client,
                   struct wl_resource* resource,
                   uint32_t serial,
                   const char* language);
  /**
   * @param serial serial of the latest known text input state
   */
  void (*text_direction)(struct wl_client* client,
                         struct wl_resource* resource,
                         uint32_t serial,
                         uint32_t direction);
};

#define ZWP_INPUT_METHOD_CONTEXT_V1_SURROUNDING_TEXT 0
#define ZWP_INPUT_METHOD_CONTEXT_V1_RESET 1
#define ZWP_INPUT_METHOD_CONTEXT_V1_CONTENT_TYPE 2
#define ZWP_INPUT_METHOD_CONTEXT_V1_INVOKE_ACTION 3
#define ZWP_INPUT_METHOD_CONTEXT_V1_COMMIT_STATE 4
#define ZWP_INPUT_METHOD_CONTEXT_V1_PREFERRED_LANGUAGE 5

/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_SURROUNDING_TEXT_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_RESET_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_CONTENT_TYPE_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_INVOKE_ACTION_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_COMMIT_STATE_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_PREFERRED_LANGUAGE_SINCE_VERSION 1

/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_DESTROY_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_COMMIT_STRING_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_PREEDIT_STRING_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_PREEDIT_STYLING_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_PREEDIT_CURSOR_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_DELETE_SURROUNDING_TEXT_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_CURSOR_POSITION_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_MODIFIERS_MAP_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_KEYSYM_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_GRAB_KEYBOARD_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_KEY_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_MODIFIERS_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_LANGUAGE_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_context_v1
 */
#define ZWP_INPUT_METHOD_CONTEXT_V1_TEXT_DIRECTION_SINCE_VERSION 1

/**
 * @ingroup iface_zwp_input_method_context_v1
 * Sends an surrounding_text event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_context_v1_send_surrounding_text(
    struct wl_resource* resource_,
    const char* text,
    uint32_t cursor,
    uint32_t anchor) {
  wl_resource_post_event(resource_,
                         ZWP_INPUT_METHOD_CONTEXT_V1_SURROUNDING_TEXT, text,
                         cursor, anchor);
}

/**
 * @ingroup iface_zwp_input_method_context_v1
 * Sends an reset event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_context_v1_send_reset(
    struct wl_resource* resource_) {
  wl_resource_post_event(resource_, ZWP_INPUT_METHOD_CONTEXT_V1_RESET);
}

/**
 * @ingroup iface_zwp_input_method_context_v1
 * Sends an content_type event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_context_v1_send_content_type(
    struct wl_resource* resource_,
    uint32_t hint,
    uint32_t purpose) {
  wl_resource_post_event(resource_, ZWP_INPUT_METHOD_CONTEXT_V1_CONTENT_TYPE,
                         hint, purpose);
}

/**
 * @ingroup iface_zwp_input_method_context_v1
 * Sends an invoke_action event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_context_v1_send_invoke_action(
    struct wl_resource* resource_,
    uint32_t button,
    uint32_t index) {
  wl_resource_post_event(resource_, ZWP_INPUT_METHOD_CONTEXT_V1_INVOKE_ACTION,
                         button, index);
}

/**
 * @ingroup iface_zwp_input_method_context_v1
 * Sends an commit_state event to the client owning the resource.
 * @param resource_ The client's resource
 * @param serial serial of text input state
 */
static inline void zwp_input_method_context_v1_send_commit_state(
    struct wl_resource* resource_,
    uint32_t serial) {
  wl_resource_post_event(resource_, ZWP_INPUT_METHOD_CONTEXT_V1_COMMIT_STATE,
                         serial);
}

/**
 * @ingroup iface_zwp_input_method_context_v1
 * Sends an preferred_language event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_context_v1_send_preferred_language(
    struct wl_resource* resource_,
    const char* language) {
  wl_resource_post_event(
      resource_, ZWP_INPUT_METHOD_CONTEXT_V1_PREFERRED_LANGUAGE, language);
}

#define ZWP_INPUT_METHOD_V1_ACTIVATE 0
#define ZWP_INPUT_METHOD_V1_DEACTIVATE 1

/**
 * @ingroup iface_zwp_input_method_v1
 */
#define ZWP_INPUT_METHOD_V1_ACTIVATE_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_method_v1
 */
#define ZWP_INPUT_METHOD_V1_DEACTIVATE_SINCE_VERSION 1

/**
 * @ingroup iface_zwp_input_method_v1
 * Sends an activate event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_v1_send_activate(
    struct wl_resource* resource_,
    struct wl_resource* id) {
  wl_resource_post_event(resource_, ZWP_INPUT_METHOD_V1_ACTIVATE, id);
}

/**
 * @ingroup iface_zwp_input_method_v1
 * Sends an deactivate event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void zwp_input_method_v1_send_deactivate(
    struct wl_resource* resource_,
    struct wl_resource* context) {
  wl_resource_post_event(resource_, ZWP_INPUT_METHOD_V1_DEACTIVATE, context);
}

/**
 * @ingroup iface_zwp_input_panel_v1
 * @struct zwp_input_panel_v1_interface
 */
struct zwp_input_panel_v1_interface {
  /**
   */
  void (*get_input_panel_surface)(struct wl_client* client,
                                  struct wl_resource* resource,
                                  uint32_t id,
                                  struct wl_resource* surface);
};

/**
 * @ingroup iface_zwp_input_panel_v1
 */
#define ZWP_INPUT_PANEL_V1_GET_INPUT_PANEL_SURFACE_SINCE_VERSION 1

#ifndef ZWP_INPUT_PANEL_SURFACE_V1_POSITION_ENUM
#define ZWP_INPUT_PANEL_SURFACE_V1_POSITION_ENUM
enum zwp_input_panel_surface_v1_position {
  ZWP_INPUT_PANEL_SURFACE_V1_POSITION_CENTER_BOTTOM = 0,
};
#endif /* ZWP_INPUT_PANEL_SURFACE_V1_POSITION_ENUM */

/**
 * @ingroup iface_zwp_input_panel_surface_v1
 * @struct zwp_input_panel_surface_v1_interface
 */
struct zwp_input_panel_surface_v1_interface {
  /**
   * set the surface type as a keyboard
   *
   * Set the input_panel_surface type to keyboard.
   *
   * A keyboard surface is only shown when a text input is active.
   */
  void (*set_toplevel)(struct wl_client* client,
                       struct wl_resource* resource,
                       struct wl_resource* output,
                       uint32_t position);
  /**
   * set the surface type as an overlay panel
   *
   * Set the input_panel_surface to be an overlay panel.
   *
   * This is shown near the input cursor above the application window
   * when a text input is active.
   */
  void (*set_overlay_panel)(struct wl_client* client,
                            struct wl_resource* resource);
};

/**
 * @ingroup iface_zwp_input_panel_surface_v1
 */
#define ZWP_INPUT_PANEL_SURFACE_V1_SET_TOPLEVEL_SINCE_VERSION 1
/**
 * @ingroup iface_zwp_input_panel_surface_v1
 */
#define ZWP_INPUT_PANEL_SURFACE_V1_SET_OVERLAY_PANEL_SINCE_VERSION 1

#ifdef __cplusplus
}
#endif

#endif
