cmake_minimum_required(VERSION 3.21)

project(litheview_demo LANGUAGES CXX)

if(NOT TARGET LitheView::LitheView)
  find_package(LitheView CONFIG REQUIRED)
endif()

set(litheview_demo_sources
  demo_support.cc
  demo_pages.cc
  native_dialogs.cc
  shell.cc
  shell_host_callbacks.cc
  shell_host_file_drop.cc
  shell_host_input.cc
  shell_host_page.cc
  shell_host_ui.cc
  shell_host_window.cc
  shared_texture_presenter.cc
)

set(litheview_demo_headers
  demo_support.h
  demo_pages.h
  devtools_resource_server.h
  native_dialogs.h
  shell.h
  shell_app.h
  shell_host.h
  shared_texture_presenter.h
)
list(TRANSFORM litheview_demo_headers
  PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/"
  OUTPUT_VARIABLE litheview_demo_header_paths)

add_executable(litheview_demo WIN32
  main.cc
  ${litheview_demo_sources}
  ${litheview_demo_header_paths}
)

set(devtools_resource_dir
  "${CMAKE_CURRENT_SOURCE_DIR}/../../share/litheview/inspector")
set(devtools_pak "${devtools_resource_dir}/devtools_resources.pak")
set(devtools_header
  "${devtools_resource_dir}/devtools_resources.h")
set(devtools_map
  "${devtools_resource_dir}/devtools_resources_map.cc")
foreach(devtools_input IN ITEMS
    "${devtools_pak}" "${devtools_header}" "${devtools_map}")
  if(NOT EXISTS "${devtools_input}")
    message(FATAL_ERROR "Inspector resource input is missing: ${devtools_input}")
  endif()
endforeach()

set(devtools_resource_index
  "${CMAKE_CURRENT_BINARY_DIR}/devtools_resource_index.inc")
execute_process(
  COMMAND powershell -NoProfile -ExecutionPolicy Bypass
          -File "${CMAKE_CURRENT_SOURCE_DIR}/generate_devtools_resource_index.ps1"
          -Header "${devtools_header}"
          -Map "${devtools_map}"
          -Output "${devtools_resource_index}"
  COMMAND_ERROR_IS_FATAL ANY
)
file(TO_CMAKE_PATH "${devtools_pak}" DEVTOOLS_PAK_RC_PATH)
configure_file(devtools_resources.rc.in
  "${CMAKE_CURRENT_BINARY_DIR}/devtools_resources.rc" @ONLY)

add_executable(litheview_inspector WIN32
  inspector_main.cc
  devtools_resource_server.cc
  "${CMAKE_CURRENT_BINARY_DIR}/devtools_resources.rc"
  ${litheview_demo_sources}
  ${litheview_demo_header_paths}
)
set_property(SOURCE devtools_resource_server.cc APPEND PROPERTY
  OBJECT_DEPENDS "${devtools_resource_index};${devtools_pak}")
target_include_directories(litheview_inspector PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")

# Some localized MSVC installations emit /showIncludes text that Ninja cannot
# decode, leaving its discovered dependency list empty. Keep the standalone
# SDK example correct by declaring its small header surface explicitly.
get_target_property(litheview_include_dirs
  LitheView::LitheView INTERFACE_INCLUDE_DIRECTORIES)
set(litheview_sdk_headers)
foreach(litheview_include_dir IN LISTS litheview_include_dirs)
  file(GLOB litheview_headers_in_dir CONFIGURE_DEPENDS
    "${litheview_include_dir}/litheview/*.h")
  list(APPEND litheview_sdk_headers ${litheview_headers_in_dir})
endforeach()
set_property(SOURCE ${litheview_demo_sources} main.cc inspector_main.cc
  APPEND PROPERTY OBJECT_DEPENDS
  "${litheview_demo_header_paths};${litheview_sdk_headers}")

foreach(litheview_target IN ITEMS litheview_demo litheview_inspector)
  target_compile_features(${litheview_target} PRIVATE cxx_std_20)
  target_compile_definitions(${litheview_target} PRIVATE
    NOMINMAX
    UNICODE
    WIN32_LEAN_AND_MEAN
    _UNICODE
  )
  target_link_libraries(${litheview_target} PRIVATE
    LitheView::LitheView
    comctl32
    comdlg32
    d3d11
    dxgi
    imm32
    ole32
    shell32
    ws2_32
  )
  if(MSVC)
    target_compile_options(${litheview_target} PRIVATE /W4 /utf-8)
    target_link_options(${litheview_target} PRIVATE
      /STACK:0x800000
      "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/litheview_demo.manifest"
    )
  endif()
  add_custom_command(TARGET ${litheview_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LitheView::LitheView>
            $<TARGET_FILE_DIR:${litheview_target}>
  )
endforeach()
