#ifndef LITHEVIEW_PUBLIC_LITHEVIEW_H_
#define LITHEVIEW_PUBLIC_LITHEVIEW_H_

#include <stdint.h>

#include "litheview_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LTV_API_VERSION 1u
#define LTV_STRUCT_VERSION 1u

typedef enum ltv_error_t {
  LTV_OK = 0,
  LTV_ERR_INVALID_ARG = 1,
  LTV_ERR_INVALID_STATE = 2,
  LTV_ERR_WRONG_THREAD = 3,
  LTV_ERR_LOAD_FAILED = 4,
  LTV_ERR_JS_EXCEPTION = 5,
  LTV_ERR_IO = 6,
  LTV_ERR_INTERNAL = 7,
  LTV_ERR_UNSUPPORTED = 8,
  LTV_ERR_RESOURCE_EXHAUSTED = 9,
} ltv_error_t;

// Stable SDK values for termination of the primary renderer process. These
// values are independent of Chromium's internal TerminationStatus numbering.
typedef enum ltv_renderer_termination_status_t {
  LTV_RENDERER_TERMINATION_NORMAL = 0,
  LTV_RENDERER_TERMINATION_ABNORMAL = 1,
  LTV_RENDERER_TERMINATION_KILLED = 2,
  LTV_RENDERER_TERMINATION_CRASHED = 3,
  LTV_RENDERER_TERMINATION_LAUNCH_FAILED = 4,
  LTV_RENDERER_TERMINATION_OUT_OF_MEMORY = 5,
  LTV_RENDERER_TERMINATION_INTEGRITY_FAILURE = 6,
  LTV_RENDERER_TERMINATION_EVICTED = 7,
  LTV_RENDERER_TERMINATION_UNKNOWN = 8,
} ltv_renderer_termination_status_t;

typedef enum ltv_close_decision_t {
  LTV_CLOSE_ALLOW = 0,
  LTV_CLOSE_CANCEL = 1,
} ltv_close_decision_t;

typedef enum ltv_popup_decision_t {
  LTV_POPUP_BLOCK = 0,
  LTV_POPUP_ALLOW = 1,
} ltv_popup_decision_t;

typedef enum ltv_popup_disposition_t {
  LTV_POPUP_DISPOSITION_UNKNOWN = 0,
  LTV_POPUP_DISPOSITION_NEW_POPUP = 1,
  LTV_POPUP_DISPOSITION_NEW_WINDOW = 2,
  LTV_POPUP_DISPOSITION_NEW_FOREGROUND_TAB = 3,
  LTV_POPUP_DISPOSITION_NEW_BACKGROUND_TAB = 4,
} ltv_popup_disposition_t;

typedef enum ltv_console_level_t {
  LTV_CONSOLE_LEVEL_VERBOSE = 0,
  LTV_CONSOLE_LEVEL_INFO = 1,
  LTV_CONSOLE_LEVEL_WARNING = 2,
  LTV_CONSOLE_LEVEL_ERROR = 3,
} ltv_console_level_t;

typedef enum ltv_console_decision_t {
  LTV_CONSOLE_USE_DEFAULT = 0,
  LTV_CONSOLE_HANDLED = 1,
} ltv_console_decision_t;

typedef enum ltv_javascript_dialog_type_t {
  LTV_JAVASCRIPT_DIALOG_ALERT = 0,
  LTV_JAVASCRIPT_DIALOG_CONFIRM = 1,
  LTV_JAVASCRIPT_DIALOG_PROMPT = 2,
  LTV_JAVASCRIPT_DIALOG_BEFORE_UNLOAD = 3,
} ltv_javascript_dialog_type_t;

typedef enum ltv_dialog_handling_t {
  LTV_DIALOG_UNHANDLED = 0,
  LTV_DIALOG_HANDLED = 1,
} ltv_dialog_handling_t;

typedef enum ltv_certificate_status_flag_t {
  LTV_CERT_STATUS_COMMON_NAME_INVALID = 1u << 0,
  LTV_CERT_STATUS_DATE_INVALID = 1u << 1,
  LTV_CERT_STATUS_AUTHORITY_INVALID = 1u << 2,
  LTV_CERT_STATUS_NO_REVOCATION_MECHANISM = 1u << 3,
  LTV_CERT_STATUS_UNABLE_TO_CHECK_REVOCATION = 1u << 4,
  LTV_CERT_STATUS_REVOKED = 1u << 5,
  LTV_CERT_STATUS_INVALID = 1u << 6,
  LTV_CERT_STATUS_WEAK_SIGNATURE_ALGORITHM = 1u << 7,
  LTV_CERT_STATUS_NON_UNIQUE_NAME = 1u << 8,
  LTV_CERT_STATUS_WEAK_KEY = 1u << 9,
  LTV_CERT_STATUS_PINNED_KEY_MISSING = 1u << 10,
  LTV_CERT_STATUS_NAME_CONSTRAINT_VIOLATION = 1u << 11,
  LTV_CERT_STATUS_VALIDITY_TOO_LONG = 1u << 12,
  LTV_CERT_STATUS_CERTIFICATE_TRANSPARENCY_REQUIRED = 1u << 13,
  LTV_CERT_STATUS_KNOWN_INTERCEPTION_BLOCKED = 1u << 14,
  LTV_CERT_STATUS_SELF_SIGNED_LOCAL_NETWORK = 1u << 15,
} ltv_certificate_status_flag_t;

typedef enum ltv_certificate_error_handling_t {
  LTV_CERTIFICATE_ERROR_UNHANDLED = 0,
  LTV_CERTIFICATE_ERROR_HANDLED = 1,
} ltv_certificate_error_handling_t;

typedef enum ltv_certificate_error_decision_t {
  LTV_CERTIFICATE_ERROR_DENY = 0,
  LTV_CERTIFICATE_ERROR_ALLOW = 1,
} ltv_certificate_error_decision_t;

typedef enum ltv_file_dialog_mode_t {
  LTV_FILE_DIALOG_OPEN = 0,
  LTV_FILE_DIALOG_OPEN_MULTIPLE = 1,
  LTV_FILE_DIALOG_UPLOAD_FOLDER = 2,
  LTV_FILE_DIALOG_OPEN_DIRECTORY = 3,
  LTV_FILE_DIALOG_SAVE = 4,
} ltv_file_dialog_mode_t;

typedef enum ltv_file_dialog_handling_t {
  LTV_FILE_DIALOG_UNHANDLED = 0,
  LTV_FILE_DIALOG_HANDLED = 1,
} ltv_file_dialog_handling_t;

typedef enum ltv_resource_type_t {
  LTV_RESOURCE_TYPE_MAIN_FRAME = 0,
  LTV_RESOURCE_TYPE_SUB_FRAME = 1,
  LTV_RESOURCE_TYPE_STYLESHEET = 2,
  LTV_RESOURCE_TYPE_SCRIPT = 3,
  LTV_RESOURCE_TYPE_IMAGE = 4,
  LTV_RESOURCE_TYPE_FONT = 5,
  LTV_RESOURCE_TYPE_MEDIA = 6,
  LTV_RESOURCE_TYPE_FETCH_XHR = 7,
  LTV_RESOURCE_TYPE_WORKER = 8,
  LTV_RESOURCE_TYPE_WEBSOCKET = 9,
  LTV_RESOURCE_TYPE_OTHER = 10,
} ltv_resource_type_t;

typedef enum ltv_request_decision_t {
  LTV_REQUEST_ALLOW = 0,
  LTV_REQUEST_BLOCK = 1,
} ltv_request_decision_t;

typedef enum ltv_download_handling_t {
  LTV_DOWNLOAD_UNHANDLED = 0,
  LTV_DOWNLOAD_HANDLED = 1,
} ltv_download_handling_t;

typedef enum ltv_download_state_t {
  LTV_DOWNLOAD_STATE_IN_PROGRESS = 0,
  LTV_DOWNLOAD_STATE_COMPLETE = 1,
  LTV_DOWNLOAD_STATE_CANCELLED = 2,
  LTV_DOWNLOAD_STATE_INTERRUPTED = 3,
} ltv_download_state_t;

typedef enum ltv_download_interrupt_reason_t {
  LTV_DOWNLOAD_INTERRUPT_NONE = 0,
  LTV_DOWNLOAD_INTERRUPT_FILE = 1,
  LTV_DOWNLOAD_INTERRUPT_NETWORK = 2,
  LTV_DOWNLOAD_INTERRUPT_SERVER = 3,
  LTV_DOWNLOAD_INTERRUPT_USER = 4,
  LTV_DOWNLOAD_INTERRUPT_CRASH = 5,
  LTV_DOWNLOAD_INTERRUPT_OTHER = 6,
} ltv_download_interrupt_reason_t;

typedef enum ltv_edit_command_t {
  LTV_EDIT_COMMAND_UNDO = 0,
  LTV_EDIT_COMMAND_REDO = 1,
  LTV_EDIT_COMMAND_CUT = 2,
  LTV_EDIT_COMMAND_COPY = 3,
  LTV_EDIT_COMMAND_PASTE = 4,
  LTV_EDIT_COMMAND_DELETE = 5,
  LTV_EDIT_COMMAND_SELECT_ALL = 6,
} ltv_edit_command_t;

typedef enum ltv_gpu_delivery_t {
  /** The exported resource is the compositor render target itself. */
  LTV_GPU_DELIVERY_DIRECT = 1,
  /** The compositor copies into an exportable GPU resource without CPU readback. */
  LTV_GPU_DELIVERY_GPU_COPY = 2,
} ltv_gpu_delivery_t;

typedef enum ltv_gpu_handle_type_t {
  LTV_GPU_HANDLE_NONE = 0,
  /** An operating-system handle for a shareable 2D GPU texture. */
  LTV_GPU_HANDLE_SHARED_TEXTURE = 1,
} ltv_gpu_handle_type_t;

typedef enum ltv_gpu_format_t {
  LTV_GPU_FORMAT_UNKNOWN = 0,
  LTV_GPU_FORMAT_BGRA8_UNORM = 1,
  LTV_GPU_FORMAT_RGBA8_UNORM = 2,
  LTV_GPU_FORMAT_RGBA16_FLOAT = 3,
} ltv_gpu_format_t;

typedef struct ltv_rect_t {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} ltv_rect_t;

/** @brief Configuration for continuous GPU frame output. */
typedef struct ltv_gpu_output_settings_t {
  uint32_t struct_size;
  uint32_t version;
  /** Required delivery path. Unsupported paths fail instead of falling back. */
  ltv_gpu_delivery_t delivery;
  /** Maximum callback rate in frames per second, from 1 through 240. */
  uint32_t max_frame_rate;
} ltv_gpu_output_settings_t;

/**
 * @brief One leased GPU frame.
 *
 * The native handle and referenced texture remain valid until frame_token is
 * passed to ltv_view_release_gpu_frame(). Frame delivery runs on the LitheView
 * UI thread after GPU production has completed. The host must import and use
 * the texture in the same process and on a device for the same GPU adapter.
 * Do not release the frame until all host GPU reads have completed. Alpha is
 * premultiplied. The native handle is borrowed and must not be closed.
 */
typedef struct ltv_gpu_frame_t {
  uint32_t struct_size;
  uint32_t version;
  /** Unique lease identifier accepted exactly once by the release function. */
  uint64_t frame_token;
  /** Stable identity while Chromium reuses the same underlying texture. */
  uint64_t resource_id;
  /** Delivery path used to produce this frame. */
  ltv_gpu_delivery_t delivery;
  /** Kind of operating-system resource referenced by native_handle. */
  ltv_gpu_handle_type_t handle_type;
  /** Borrowed operating-system handle; the host must not close it. */
  uintptr_t native_handle;
  /** Channel order and component representation of the texture. */
  ltv_gpu_format_t format;
  /** Allocated texture dimensions in physical pixels. */
  uint32_t width;
  uint32_t height;
  /** Valid pixels, captured content, and changed region in top-left pixels. */
  ltv_rect_t visible_rect;
  ltv_rect_t content_rect;
  ltv_rect_t damage_rect;
  /** Capture-stream timestamp, in microseconds. */
  int64_t timestamp_us;
  /** Producer sequence number, or zero when unavailable. */
  uint64_t capture_counter;
  /** Changes when the capture target or output dimensions change. */
  uint64_t surface_generation;
  /** Opaque graphics-adapter identifier, or zero when unavailable. */
  uint64_t device_id;
} ltv_gpu_frame_t;

/**
 * @brief Global settings used to initialize the LitheView runtime.
 *
 * Public structures are append-only. Initialize @c struct_size to the size of
 * the structure and @c version to LTV_STRUCT_VERSION. LitheView ignores
 * fields beyond the structure size it understands.
 */
typedef struct ltv_settings_t {
  uint32_t struct_size;       /**< Size of this structure in bytes. */
  uint32_t version;           /**< Structure version; use LTV_STRUCT_VERSION. */
  int32_t width_dip;          /**< Initial view width in device-independent pixels. */
  int32_t height_dip;         /**< Initial view height in device-independent pixels. */
  float device_scale_factor;  /**< Initial physical pixels per DIP; must be positive. */
  /** Loopback DevTools port used when remote_debugging_enabled is non-zero. */
  int32_t remote_debugging_port;
  /** Loopback Inspector origin port allowed to connect to DevTools. */
  int32_t devtools_frontend_port;
  /** Non-zero explicitly enables remote debugging for this process. */
  int32_t remote_debugging_enabled;
} ltv_settings_t;

/**
 * @brief Describes a page-initiated popup request.
 *
 * The structure and all pointer fields are borrowed and remain valid only for
 * the duration of the popup callback.
 */
typedef struct ltv_popup_request_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  const char* target_url; /**< Borrowed UTF-8 target URL; never owned by the host. */
  ltv_popup_disposition_t disposition; /**< Requested popup/window disposition. */
  int32_t user_gesture;  /**< Non-zero when initiated by a user gesture. */
  int32_t x_dip;         /**< Requested left edge in screen DIPs. */
  int32_t y_dip;         /**< Requested top edge in screen DIPs. */
  int32_t width_dip;     /**< Requested content width in DIPs, or zero if unspecified. */
  int32_t height_dip;    /**< Requested content height in DIPs, or zero if unspecified. */
} ltv_popup_request_t;

/**
 * @brief Console message emitted by a page.
 *
 * The structure and strings are borrowed for the callback duration.
 */
typedef struct ltv_console_message_t {
  uint32_t struct_size;       /**< Size of this structure in bytes. */
  uint32_t version;           /**< Structure version; currently LTV_STRUCT_VERSION. */
  ltv_console_level_t level;  /**< Severity reported by the renderer. */
  const char* message;        /**< Borrowed UTF-8 console text. */
  const char* source_url;     /**< Borrowed UTF-8 source URL; may be empty. */
  int32_t line_number;        /**< One-based source line, or zero when unavailable. */
} ltv_console_message_t;

/**
 * @brief Pending JavaScript dialog presented by a page.
 *
 * The structure and strings are borrowed for the callback duration. A handled
 * request remains pending until it is replied to or cancelled by a client,
 * navigation, or View lifetime change.
 */
typedef struct ltv_javascript_dialog_request_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint64_t dialog_id;    /**< Non-zero identifier used to complete this dialog. */
  ltv_javascript_dialog_type_t type; /**< Kind of JavaScript dialog. */
  const char* source_url;     /**< Borrowed UTF-8 URL of the requesting page. */
  const char* message;        /**< Borrowed UTF-8 dialog message. */
  const char* default_prompt; /**< Borrowed UTF-8 prompt default; empty otherwise. */
  int32_t is_reload;          /**< Non-zero for a before-unload reload request. */
} ltv_javascript_dialog_request_t;

/**
 * @brief Certificate validation failure for a resource request.
 *
 * The structure and strings are borrowed for the callback duration. A non-zero
 * @c request_id can be completed once. A zero identifier is informational
 * and cannot be overridden.
 */
typedef struct ltv_certificate_error_request_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint64_t request_id;   /**< Completion identifier, or zero if not overridable. */
  const char* request_url; /**< Borrowed UTF-8 URL whose certificate failed. */
  int32_t error_code;      /**< Chromium network certificate error code. */
  uint32_t status_flags;   /**< Bitwise OR of ltv_certificate_status_flag_t. */
  int32_t is_primary_main_frame; /**< Non-zero for the primary main frame. */
  int32_t strict_enforcement;    /**< Non-zero when policy forbids an override. */
  int32_t can_override;          /**< Non-zero when the host may allow the request. */
  const char* subject;           /**< Borrowed UTF-8 certificate subject. */
  const char* issuer;            /**< Borrowed UTF-8 certificate issuer. */
  const char* sha256_fingerprint; /**< Borrowed printable SHA-256 fingerprint. */
  int64_t valid_start_unix_ms;   /**< Validity start as milliseconds since Unix epoch. */
  int64_t valid_expiry_unix_ms;  /**< Validity end as milliseconds since Unix epoch. */
} ltv_certificate_error_request_t;

/**
 * @brief Pending file chooser request.
 *
 * The structure, strings, and arrays are borrowed for the callback duration.
 * Array pointers may be NULL when their corresponding count is zero. A handled
 * request remains pending until it is replied to or cancelled by the library.
 */
typedef struct ltv_file_dialog_request_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint64_t dialog_id;    /**< Non-zero identifier used to complete this dialog. */
  ltv_file_dialog_mode_t mode; /**< Requested chooser mode. */
  const char* title;            /**< Borrowed UTF-8 dialog title; may be empty. */
  const char* default_file_name; /**< Borrowed UTF-8 suggested path or name. */
  const char* const* selected_files; /**< Borrowed UTF-8 preselected paths. */
  uint32_t selected_file_count;      /**< Number of selected_files entries. */
  const char* const* accept_types;   /**< Borrowed UTF-8 MIME/extension filters. */
  uint32_t accept_type_count;        /**< Number of accept_types entries. */
  const char* requestor_url;         /**< Borrowed UTF-8 requesting frame URL. */
  int32_t need_local_path; /**< Non-zero when local filesystem paths are required. */
  int32_t use_media_capture; /**< Non-zero for a media capture chooser. */
  int32_t open_writable; /**< Non-zero when the selected target must be writable. */
} ltv_file_dialog_request_t;

/**
 * @brief Network request metadata supplied to the request policy callback.
 *
 * The structure and strings are borrowed for the callback duration. The
 * identifier remains stable across redirects for one logical request and is
 * informational in this synchronous contract.
 */
typedef struct ltv_request_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint64_t request_id;   /**< Stable identifier for this logical request. */
  ltv_resource_type_t resource_type; /**< SDK-normalized resource category. */
  const char* url;       /**< Borrowed UTF-8 request URL. */
  const char* method;    /**< Borrowed UTF-8 HTTP method. */
  const char* initiator; /**< Borrowed UTF-8 initiator origin; may be empty. */
  const char* referrer;  /**< Borrowed UTF-8 referrer; may be empty. */
  int32_t is_navigation; /**< Non-zero for a navigation request. */
  int32_t is_redirect;   /**< Non-zero after an HTTP redirect. */
} ltv_request_t;

/**
 * @brief Metadata for a download awaiting target selection.
 *
 * The structure and strings are borrowed for the callback duration. A handled
 * download remains pending until it is replied to or cancelled by the library.
 */
typedef struct ltv_download_request_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint64_t download_id;  /**< Non-zero identifier for download operations. */
  const char* url;       /**< Borrowed UTF-8 download URL. */
  const char* suggested_file_name; /**< Borrowed UTF-8 suggested file name. */
  const char* mime_type; /**< Borrowed UTF-8 MIME type; may be empty. */
  const char* content_disposition; /**< Borrowed UTF-8 response header value. */
  int64_t total_bytes;   /**< Expected byte count, or -1 when unknown. */
  int32_t has_user_gesture; /**< Non-zero when initiated by a user gesture. */
} ltv_download_request_t;

/**
 * @brief Current state and progress of a download.
 *
 * The structure and strings are borrowed for the callback duration. The target
 * path is empty until the host accepts a target.
 */
typedef struct ltv_download_update_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  uint64_t download_id;  /**< Identifier matching the original request. */
  ltv_download_state_t state; /**< Current terminal or in-progress state. */
  ltv_download_interrupt_reason_t interrupt_reason; /**< Failure category. */
  const char* target_path; /**< Borrowed UTF-8 absolute target path, or empty. */
  int64_t received_bytes;  /**< Number of bytes received so far. */
  int64_t total_bytes;     /**< Expected byte count, or -1 when unknown. */
  int64_t current_speed;   /**< Current transfer rate in bytes per second. */
  int32_t percent_complete; /**< Completion percentage, or -1 when unknown. */
  int32_t is_paused;        /**< Non-zero while the download is paused. */
} ltv_download_update_t;

/**
 * @brief Message posted from a page to its host.
 *
 * The structure and strings are borrowed for the callback duration. The JSON
 * text contains one complete JSON value, not a JavaScript expression.
 */
typedef struct ltv_javascript_message_t {
  uint32_t struct_size;   /**< Size of this structure in bytes. */
  uint32_t version;       /**< Structure version; currently LTV_STRUCT_VERSION. */
  const char* source_url; /**< Borrowed UTF-8 source frame URL. */
  const char* json_value; /**< Borrowed UTF-8 serialized JSON value. */
  int32_t is_main_frame;  /**< Non-zero when posted by the primary main frame. */
} ltv_javascript_message_t;

/**
 * @brief Borrowed result of asynchronous JavaScript execution.
 *
 * Exactly one of @c json_result and @c error is non-NULL. All pointers are
 * valid only for the result callback duration.
 */
typedef struct ltv_javascript_result_t {
  uint32_t struct_size; /**< Size of this structure in bytes. */
  uint32_t version;     /**< Structure version; currently LTV_STRUCT_VERSION. */
  ltv_error_t code;     /**< LTV_OK or the execution failure code. */
  uint32_t reserved;    /**< Reserved for ABI growth; currently zero. */
  const char* json_result; /**< Borrowed UTF-8 JSON result on success. */
  const char* error;       /**< Borrowed UTF-8 error text on failure. */
} ltv_javascript_result_t;

/** @brief Snapshot of navigation capabilities and loading state. */
typedef struct ltv_navigation_state_t {
  uint32_t struct_size;  /**< Size of this structure in bytes. */
  uint32_t version;      /**< Structure version; currently LTV_STRUCT_VERSION. */
  int32_t can_go_back;   /**< Non-zero when backward navigation is available. */
  int32_t can_go_forward; /**< Non-zero when forward navigation is available. */
  int32_t is_loading;    /**< Non-zero while the View is loading a document. */
} ltv_navigation_state_t;

typedef struct ltv_view_s* ltv_view_t;

typedef void(LTV_CALLBACK* ltv_loading_changed_callback_t)(ltv_view_t view,
                                                           int32_t is_loading,
                                                           void* user_data);
typedef void(LTV_CALLBACK* ltv_url_changed_callback_t)(ltv_view_t view,
                                                       const char* url,
                                                       void* user_data);
typedef void(LTV_CALLBACK* ltv_title_changed_callback_t)(ltv_view_t view,
                                                         const char* title,
                                                         void* user_data);
typedef void(LTV_CALLBACK* ltv_load_error_callback_t)(ltv_view_t view,
                                                      const char* url,
                                                      int32_t error_code,
                                                      const char* error_text,
                                                      void* user_data);
typedef void(LTV_CALLBACK* ltv_view_closed_callback_t)(ltv_view_t view,
                                                       void* user_data);
typedef void(LTV_CALLBACK* ltv_renderer_terminated_callback_t)(
    ltv_view_t view,
    ltv_renderer_termination_status_t status,
    void* user_data);
typedef ltv_close_decision_t(LTV_CALLBACK* ltv_close_requested_callback_t)(
    ltv_view_t view,
    void* user_data);
typedef ltv_popup_decision_t(LTV_CALLBACK* ltv_before_popup_callback_t)(
    ltv_view_t opener,
    const ltv_popup_request_t* request,
    void* user_data);
typedef void(LTV_CALLBACK* ltv_popup_created_callback_t)(
    ltv_view_t opener,
    ltv_view_t popup,
    const ltv_popup_request_t* request,
    void* user_data);
typedef ltv_console_decision_t(LTV_CALLBACK* ltv_console_message_callback_t)(
    ltv_view_t view,
    const ltv_console_message_t* message,
    void* user_data);
typedef ltv_dialog_handling_t(LTV_CALLBACK* ltv_javascript_dialog_callback_t)(
    ltv_view_t view,
    const ltv_javascript_dialog_request_t* request,
    void* user_data);
typedef ltv_certificate_error_handling_t(
    LTV_CALLBACK* ltv_certificate_error_callback_t)(
    ltv_view_t view,
    const ltv_certificate_error_request_t* request,
    void* user_data);
typedef ltv_file_dialog_handling_t(LTV_CALLBACK* ltv_file_dialog_callback_t)(
    ltv_view_t view,
    const ltv_file_dialog_request_t* request,
    void* user_data);
typedef ltv_request_decision_t(LTV_CALLBACK* ltv_before_request_callback_t)(
    ltv_view_t view,
    const ltv_request_t* request,
    void* user_data);
typedef ltv_download_handling_t(LTV_CALLBACK* ltv_before_download_callback_t)(
    ltv_view_t view,
    const ltv_download_request_t* request,
    void* user_data);
typedef void(LTV_CALLBACK* ltv_download_updated_callback_t)(
    ltv_view_t view,
    const ltv_download_update_t* update,
    void* user_data);
typedef void(LTV_CALLBACK* ltv_javascript_message_callback_t)(
    ltv_view_t view,
    const ltv_javascript_message_t* message,
    void* user_data);
typedef void(LTV_CALLBACK* ltv_javascript_result_callback_t)(
    ltv_view_t view,
    const ltv_javascript_result_t* result,
    void* user_data);
typedef void(LTV_CALLBACK* ltv_navigation_state_callback_t)(
    ltv_view_t view,
    const ltv_navigation_state_t* state,
    void* user_data);
typedef void(LTV_CALLBACK* ltv_gpu_frame_callback_t)(
    ltv_view_t view,
    const ltv_gpu_frame_t* frame,
    void* user_data);

/**
 * @brief Host callbacks associated with one View.
 *
 * The library copies the prefix identified by @c struct_size. Callback data
 * is borrowed for each callback duration. Callbacks run on the LitheView UI
 * thread and stop before the View is destroyed. Any callback field may be NULL
 * unless its field documentation states that another feature requires it.
 */
typedef struct ltv_view_client_t {
  uint32_t struct_size; /**< Size of this structure prefix in bytes. */
  uint32_t version;     /**< Structure version; currently LTV_STRUCT_VERSION. */
  void* user_data;      /**< Opaque host pointer passed to every callback. */
  ltv_loading_changed_callback_t on_loading_changed; /**< Loading state callback. */
  ltv_url_changed_callback_t on_url_changed; /**< Committed URL callback. */
  ltv_title_changed_callback_t on_title_changed; /**< Document title callback. */
  ltv_load_error_callback_t on_load_error; /**< Main-frame load failure callback. */
  /**
   * Posted after the page closes its native WebContents. The View handle
   * remains caller-owned and must still be passed to ltv_view_destroy().
   */
  ltv_view_closed_callback_t on_closed;
  /**
   * Reports primary renderer termination. The View remains open and may be
   * navigated to create a replacement renderer.
   */
  ltv_renderer_terminated_callback_t on_renderer_terminated;
  /**
   * Synchronous page-close policy callback. Destructive SDK calls are not
   * allowed from it. A NULL callback allows the default close behavior.
   */
  ltv_close_requested_callback_t on_close_requested;
  /**
   * Synchronous popup policy callback. Destructive SDK calls are not allowed
   * from it. Popups require this field and @c on_popup_created.
   */
  ltv_before_popup_callback_t on_before_popup;
  /**
   * Posted after an allowed child View is owned by the runtime. The child
   * handle is caller-owned and must be released with ltv_view_destroy().
   */
  ltv_popup_created_callback_t on_popup_created;
  /** Console callback; return LTV_CONSOLE_HANDLED to suppress default output. */
  ltv_console_message_callback_t on_console_message;
  /**
   * JavaScript dialog policy callback. Return LTV_DIALOG_HANDLED only when the
   * host will complete the request. Unhandled ordinary dialogs are cancelled;
   * unhandled before-unload dialogs are accepted.
   */
  ltv_javascript_dialog_callback_t on_javascript_dialog;
  /**
   * Certificate policy callback. Return handled only for a non-zero request ID
   * the host will complete. Missing, unhandled, and non-overridable errors are
   * denied.
   */
  ltv_certificate_error_callback_t on_certificate_error;
  /**
   * File chooser callback. Return handled only when the host will complete the
   * request. Missing and unhandled callbacks cancel selection.
   */
  ltv_file_dialog_callback_t on_file_dialog;
  /**
   * Synchronous pre-network request policy callback. A NULL callback allows
   * requests. Destructive SDK calls are not allowed from this callback.
   */
  ltv_before_request_callback_t on_before_request;
  /**
   * Synchronous download-target callback. Return handled only when the host
   * will complete target selection. Missing and unhandled callbacks cancel the
   * download. Other destructive SDK calls are not allowed from this callback.
   */
  ltv_before_download_callback_t on_before_download;
  /** Receives download state and progress updates. */
  ltv_download_updated_callback_t on_download_updated;
  /**
   * Receives window.litheview.postMessage(value) from any frame. This callback
   * is posted and may call ordinary SDK functions.
   */
  ltv_javascript_message_callback_t on_javascript_message;
  /** Receives navigation capability and loading-state changes. */
  ltv_navigation_state_callback_t on_navigation_state_changed;
  /**
   * Receives leased GPU frames after ltv_view_start_gpu_output(). Frames must
   * be returned explicitly. Destructive SDK calls are not allowed from this
   * callback, but releasing the delivered frame is allowed.
   */
  ltv_gpu_frame_callback_t on_gpu_frame;
} ltv_view_client_t;

/**
 * @brief Caller-owned result of synchronous JavaScript evaluation.
 *
 * String fields must be released with ltv_free(). Passing NULL to ltv_free()
 * is valid.
 */
typedef struct ltv_eval_result_t {
  uint32_t struct_size; /**< Size of this structure in bytes. */
  uint32_t version;     /**< Structure version; currently LTV_STRUCT_VERSION. */
  ltv_error_t code;     /**< LTV_OK or the evaluation failure code. */
  char* json_result;    /**< Owned UTF-8 JSON result on success, otherwise NULL. */
  char* error;          /**< Owned UTF-8 error text on failure, otherwise NULL. */
} ltv_eval_result_t;

typedef void(LTV_CALLBACK* ltv_ready_callback_t)(void* user_data);

// All lifecycle and view functions, including callbacks, run on the thread
// that calls ltv_run. The ready callback runs after that thread's browser task
// loop can accept SDK calls. ltv_run blocks until ltv_shutdown finishes.
// Handles are owned by the caller and must be destroyed before shutdown.
// Remote debugging is opt-in through
// ltv_settings_t::remote_debugging_enabled and
// ltv_settings_t::remote_debugging_port. Command-line switches cannot enable
// DevTools by themselves. LitheView listens on loopback; a frontend is supplied
// separately by the host application.
LTV_EXPORT uint32_t ltv_api_version(void);
// The returned static UTF-8 string is library-owned and must not be freed.
LTV_EXPORT const char* ltv_version_string(void);
LTV_EXPORT ltv_error_t ltv_initialize(const ltv_settings_t* settings);
LTV_EXPORT int ltv_run(int argc,
                       const char* const* argv,
                       ltv_ready_callback_t ready_callback,
                       void* user_data);
LTV_EXPORT ltv_view_t ltv_view_create(void);
LTV_EXPORT ltv_view_t
ltv_view_create_with_client(const ltv_view_client_t* client);
// Passing NULL clears the current client.
LTV_EXPORT ltv_error_t ltv_view_set_client(ltv_view_t view,
                                           const ltv_view_client_t* client);
LTV_EXPORT ltv_error_t ltv_view_load_html(ltv_view_t view,
                                          const char* html,
                                          const char* base_url);
LTV_EXPORT ltv_error_t ltv_view_load_url(ltv_view_t view, const char* url);
// Starts navigation and returns without waiting for document completion.
LTV_EXPORT ltv_error_t ltv_view_navigate(ltv_view_t view, const char* url);
LTV_EXPORT ltv_error_t ltv_view_go_back(ltv_view_t view);
LTV_EXPORT ltv_error_t ltv_view_go_forward(ltv_view_t view);
LTV_EXPORT ltv_error_t ltv_view_reload(ltv_view_t view);
LTV_EXPORT ltv_error_t ltv_view_stop(ltv_view_t view);
LTV_EXPORT ltv_error_t
ltv_view_get_navigation_state(ltv_view_t view, ltv_navigation_state_t* state);
// Resizes the logical page viewport. Native hosts that render at a separate
// pixel size should use ltv_win_view_resize instead.
LTV_EXPORT ltv_error_t ltv_view_resize(ltv_view_t view,
                                       int32_t width_dip,
                                       int32_t height_dip);
LTV_EXPORT ltv_error_t ltv_view_get_size(ltv_view_t view,
                                         int32_t* width_dip,
                                         int32_t* height_dip);
LTV_EXPORT ltv_error_t ltv_view_set_focus(ltv_view_t view, int32_t focused);
// Returned UTF-8 strings are caller-owned and must be released with ltv_free.
LTV_EXPORT ltv_error_t ltv_view_get_title(ltv_view_t view, char** title);
LTV_EXPORT ltv_error_t ltv_view_get_url(ltv_view_t view, char** url);
// Returns the WebSocket target URL for inspecting this view. Remote debugging
// must be enabled. The returned string is released with ltv_free.
LTV_EXPORT ltv_error_t
ltv_view_get_devtools_target_url(ltv_view_t view, char** url);
// Returns the serialized current DOM, including changes made after loading.
LTV_EXPORT ltv_error_t ltv_view_get_html(ltv_view_t view, char** html);
LTV_EXPORT ltv_error_t
ltv_view_execute_edit_command(ltv_view_t view, ltv_edit_command_t command);
// Completes one pending JavaScript dialog. prompt_text is used only for an
// accepted prompt; passing NULL preserves the page-provided default value.
// A dialog_id can be completed exactly once.
LTV_EXPORT ltv_error_t
ltv_view_reply_to_javascript_dialog(ltv_view_t view,
                                    uint64_t dialog_id,
                                    int32_t accept,
                                    const char* prompt_text);
// Completes one pending overridable certificate error. Each request_id can be
// completed exactly once. Allowing a non-overridable request is not possible.
LTV_EXPORT ltv_error_t
ltv_view_reply_to_certificate_error(ltv_view_t view,
                                    uint64_t request_id,
                                    ltv_certificate_error_decision_t decision);
// Completes one pending file chooser. path_count zero cancels it. OPEN, SAVE,
// OPEN_DIRECTORY, and UPLOAD_FOLDER require one path; OPEN_MULTIPLE accepts one
// or more. UPLOAD_FOLDER takes a directory and recursively enumerates its
// files. A dialog_id can be completed exactly once.
LTV_EXPORT ltv_error_t
ltv_view_reply_to_file_dialog(ltv_view_t view,
                              uint64_t dialog_id,
                              const char* const* utf8_paths,
                              uint32_t path_count);
// Completes one pending download target request. utf8_path must be an absolute
// path; passing NULL cancels the download. A download_id target can be
// completed exactly once.
LTV_EXPORT ltv_error_t ltv_view_reply_to_download(ltv_view_t view,
                                                  uint64_t download_id,
                                                  const char* utf8_path);
LTV_EXPORT ltv_error_t ltv_view_pause_download(ltv_view_t view,
                                               uint64_t download_id);
LTV_EXPORT ltv_error_t ltv_view_resume_download(ltv_view_t view,
                                                uint64_t download_id);
LTV_EXPORT ltv_error_t ltv_view_cancel_download(ltv_view_t view,
                                                uint64_t download_id);
LTV_EXPORT ltv_eval_result_t ltv_view_eval(ltv_view_t view, const char* script);
// Runs script in the primary page's main JavaScript world. Promise results are
// awaited and returned as JSON. callback is optional; when supplied it runs
// asynchronously on the LitheView UI thread unless the View is destroyed.
LTV_EXPORT ltv_error_t
ltv_view_execute_javascript(ltv_view_t view,
                            const char* script,
                            ltv_javascript_result_callback_t callback,
                            void* user_data);
// Delivers one JSON value to window.litheview.onmessage and registered
// "message" listeners in the primary frame. Invalid JSON is rejected.
LTV_EXPORT ltv_error_t ltv_view_post_javascript_message(ltv_view_t view,
                                                        const char* json_value);
LTV_EXPORT ltv_error_t ltv_view_capture_png(ltv_view_t view,
                                            const char* output_path);
// Captures an encoded PNG into caller-owned memory. The output must be
// released with ltv_free. On failure, data is NULL and size is zero.
LTV_EXPORT ltv_error_t ltv_view_capture_png_to_memory(ltv_view_t view,
                                                       uint8_t** data,
                                                       uint64_t* size);
// Starts continuous GPU output. V1 implements GPU_COPY without CPU readback;
// DIRECT is capability-reserved and returns LTV_ERR_UNSUPPORTED.
LTV_EXPORT ltv_error_t
ltv_view_start_gpu_output(ltv_view_t view,
                          const ltv_gpu_output_settings_t* settings);
// Stops new GPU frame delivery. Already delivered frames remain releasable and
// must be released before the View can be destroyed or the runtime shut down.
LTV_EXPORT ltv_error_t ltv_view_stop_gpu_output(ltv_view_t view);
// Returns one frame lease. A token can be returned exactly once.
LTV_EXPORT ltv_error_t ltv_view_release_gpu_frame(ltv_view_t view,
                                                  uint64_t frame_token);
LTV_EXPORT ltv_error_t ltv_view_destroy(ltv_view_t view);
LTV_EXPORT ltv_error_t ltv_shutdown(void);
LTV_EXPORT void ltv_free(void* memory);

#ifdef __cplusplus
}
#endif

#endif  // LITHEVIEW_PUBLIC_LITHEVIEW_H_
