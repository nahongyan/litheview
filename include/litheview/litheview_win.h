#ifndef LITHEVIEW_PUBLIC_LITHEVIEW_WIN_H_
#define LITHEVIEW_PUBLIC_LITHEVIEW_WIN_H_

#include <stdint.h>

#include "litheview.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ltv_cursor_type_t {
  LTV_CURSOR_ARROW = 0,
  LTV_CURSOR_CROSS,
  LTV_CURSOR_HAND,
  LTV_CURSOR_IBEAM,
  LTV_CURSOR_WAIT,
  LTV_CURSOR_PROGRESS,
  LTV_CURSOR_HELP,
  LTV_CURSOR_SIZE_WE,
  LTV_CURSOR_SIZE_NS,
  LTV_CURSOR_SIZE_NESW,
  LTV_CURSOR_SIZE_NWSE,
  LTV_CURSOR_SIZE_ALL,
  LTV_CURSOR_NOT_ALLOWED,
  LTV_CURSOR_HIDDEN,
} ltv_cursor_type_t;

typedef enum ltv_win_ime_span_thickness_t {
  LTV_WIN_IME_SPAN_NONE = 0,
  LTV_WIN_IME_SPAN_THIN = 1,
  LTV_WIN_IME_SPAN_THICK = 2,
} ltv_win_ime_span_thickness_t;

typedef enum ltv_win_drag_operation_t {
  LTV_WIN_DRAG_OPERATION_NONE = 0,
  LTV_WIN_DRAG_OPERATION_COPY = 1,
  LTV_WIN_DRAG_OPERATION_LINK = 2,
  LTV_WIN_DRAG_OPERATION_MOVE = 16,
  LTV_WIN_DRAG_OPERATION_EVERY = 0xFFFFFFFFu,
} ltv_win_drag_operation_t;

/** @brief View-local rectangle expressed in device-independent pixels. */
typedef struct ltv_win_rect_t {
  uint32_t struct_size; /**< Size of this structure in bytes. */
  uint32_t version;     /**< Structure version; currently LTV_STRUCT_VERSION. */
  int32_t x;            /**< Left edge in DIPs. */
  int32_t y;            /**< Top edge in DIPs. */
  int32_t width;        /**< Width in DIPs. */
  int32_t height;       /**< Height in DIPs. */
} ltv_win_rect_t;

/**
 * @brief Visual span within an active IME composition.
 *
 * All offsets are UTF-16 code-unit offsets into the composition text. The end
 * offset is exclusive.
 */
typedef struct ltv_win_ime_span_t {
  uint32_t struct_size; /**< Size of this structure in bytes. */
  uint32_t version;     /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint32_t start_offset; /**< Inclusive UTF-16 code-unit offset. */
  uint32_t end_offset;   /**< Exclusive UTF-16 code-unit offset. */
  ltv_win_ime_span_thickness_t thickness; /**< Requested underline thickness. */
} ltv_win_ime_span_t;

// native_window is an HWND encoded as uintptr_t. Coordinates passed to resize
// are both logical DIPs and physical pixels so composition remains synchronized
// during per-monitor DPI and interactive native resize.
LTV_EXPORT ltv_error_t ltv_win_view_attach(ltv_view_t view,
                                           uintptr_t native_window);
LTV_EXPORT ltv_error_t ltv_win_view_detach(ltv_view_t view);
LTV_EXPORT ltv_error_t ltv_win_set_device_scale_factor(float scale_factor);
LTV_EXPORT ltv_error_t ltv_win_get_device_scale_factor(float* scale_factor);
LTV_EXPORT ltv_error_t ltv_win_view_resize(ltv_view_t view,
                                           int32_t width_dip,
                                           int32_t height_dip,
                                           int32_t width_pixels,
                                           int32_t height_pixels);
LTV_EXPORT ltv_error_t ltv_win_view_prepare_native_resize(ltv_view_t view);
// Re-enables presentation and requests a redraw when a prepared native resize
// is cancelled or its system sizing loop ends without another viewport change.
LTV_EXPORT ltv_error_t ltv_win_view_cancel_native_resize(ltv_view_t view);

// message, key_state, position and screen_position use the values from the
// corresponding Win32 mouse message. Keyboard values are WPARAM/LPARAM values.
LTV_EXPORT ltv_error_t
ltv_win_view_forward_mouse_message(ltv_view_t view,
                                   uint32_t message,
                                   uintptr_t key_state,
                                   intptr_t position_in_pixels,
                                   intptr_t screen_position_in_pixels);
LTV_EXPORT ltv_error_t
ltv_win_view_forward_keyboard_message(ltv_view_t view,
                                      uint32_t message,
                                      uintptr_t virtual_key,
                                      intptr_t key_data);
// Delivers an external local-file drag sequence to the page. Coordinates are
// view-local physical pixels plus screen physical pixels, matching forwarded
// mouse messages. utf8_paths must contain absolute local file paths.
LTV_EXPORT ltv_error_t
ltv_win_view_drag_files_enter(ltv_view_t view,
                              const char* const* utf8_paths,
                              uint32_t path_count,
                              int32_t x_pixels,
                              int32_t y_pixels,
                              int32_t screen_x_pixels,
                              int32_t screen_y_pixels,
                              uint32_t allowed_operations);
LTV_EXPORT ltv_error_t
ltv_win_view_drag_files_over(ltv_view_t view,
                             int32_t x_pixels,
                             int32_t y_pixels,
                             int32_t screen_x_pixels,
                             int32_t screen_y_pixels,
                             uint32_t allowed_operations);
LTV_EXPORT ltv_error_t ltv_win_view_drag_files_leave(ltv_view_t view);
LTV_EXPORT ltv_error_t
ltv_win_view_drag_files_drop(ltv_view_t view,
                             const char* const* utf8_paths,
                             uint32_t path_count,
                             int32_t x_pixels,
                             int32_t y_pixels,
                             int32_t screen_x_pixels,
                             int32_t screen_y_pixels);
LTV_EXPORT ltv_error_t ltv_win_view_get_cursor(ltv_view_t view,
                                               int32_t x_dip,
                                               int32_t y_dip,
                                               ltv_cursor_type_t* cursor);
// The host translates its platform IME into these structured UTF-16 calls.
// The caret rectangle is in view-local logical DIPs and can be used to place
// native candidate and composition windows. A zero-length selection is a
// caret; otherwise it identifies the selected target clause.
LTV_EXPORT ltv_error_t ltv_win_view_get_ime_caret_rect(ltv_view_t view,
                                                       ltv_win_rect_t* rect);
LTV_EXPORT ltv_error_t
ltv_win_view_ime_set_composition(ltv_view_t view,
                                 const uint16_t* text,
                                 uint32_t text_length,
                                 const ltv_win_ime_span_t* spans,
                                 uint32_t span_count,
                                 uint32_t selection_start,
                                 uint32_t selection_end);
LTV_EXPORT ltv_error_t ltv_win_view_ime_commit_text(ltv_view_t view,
                                                    const uint16_t* text,
                                                    uint32_t text_length);
LTV_EXPORT ltv_error_t ltv_win_view_ime_finish_composition(ltv_view_t view);
LTV_EXPORT ltv_error_t ltv_win_view_ime_cancel_composition(ltv_view_t view);
// Enable immediately before, and disable immediately after, a Win32 API that
// enters a native modal loop (for example DefWindowProc for WM_SYSCOMMAND).
// Calls may be nested and must be balanced on the LitheView UI thread.
LTV_EXPORT ltv_error_t ltv_win_set_os_modal_loop(int32_t enabled);

#ifdef __cplusplus
}
#endif

#endif  // LITHEVIEW_PUBLIC_LITHEVIEW_WIN_H_
