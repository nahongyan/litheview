cmake_minimum_required(VERSION 3.21)

project(litheview_qt_demo LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "The Qt SDK example currently requires Windows.")
endif()

find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Widgets)
if(NOT TARGET LitheView::LitheView)
  find_package(LitheView CONFIG REQUIRED)
endif()

qt_standard_project_setup()

set(litheview_qt_demo_sources
  browser_widget.cc
  browser_window.cc
  main.cc
  native_browser_surface.cc
)
set(litheview_qt_demo_headers
  browser_widget.h
  browser_window.h
  demo_config.h
  native_browser_surface.h
)
list(TRANSFORM litheview_qt_demo_headers
  PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/"
  OUTPUT_VARIABLE litheview_qt_demo_header_paths)

add_executable(litheview_qt_demo WIN32
  ${litheview_qt_demo_sources}
  ${litheview_qt_demo_header_paths}
)

# Localized MSVC /showIncludes output is not always decoded by Ninja. Declare
# the small public dependency surface so header edits invalidate every host
# translation unit in incremental SDK builds.
get_target_property(litheview_include_dirs
  LitheView::LitheView INTERFACE_INCLUDE_DIRECTORIES)
set(litheview_sdk_headers)
foreach(litheview_include_dir IN LISTS litheview_include_dirs)
  file(GLOB litheview_headers_in_dir CONFIGURE_DEPENDS
    "${litheview_include_dir}/litheview/*.h")
  list(APPEND litheview_sdk_headers ${litheview_headers_in_dir})
endforeach()
set_property(SOURCE ${litheview_qt_demo_sources} APPEND PROPERTY OBJECT_DEPENDS
  "${litheview_qt_demo_header_paths};${litheview_sdk_headers}")
target_compile_features(litheview_qt_demo PRIVATE cxx_std_20)
target_compile_definitions(litheview_qt_demo PRIVATE
  NOMINMAX
  UNICODE
  WIN32_LEAN_AND_MEAN
  _UNICODE
)
target_link_libraries(litheview_qt_demo PRIVATE
  Qt6::Widgets
  LitheView::LitheView
  comctl32
  dwmapi
)

if(MSVC)
  target_compile_options(litheview_qt_demo PRIVATE /W4 /utf-8)
endif()

add_custom_command(TARGET litheview_qt_demo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:LitheView::LitheView>
          $<TARGET_FILE_DIR:litheview_qt_demo>
)

get_target_property(qt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(qt_bin_dir "${qt_qmake_executable}" DIRECTORY)
find_program(windeployqt_executable windeployqt HINTS "${qt_bin_dir}" REQUIRED)
add_custom_command(TARGET litheview_qt_demo POST_BUILD
  COMMAND "${windeployqt_executable}"
          --release
          --no-compiler-runtime
          --no-opengl-sw
          --no-translations
          $<TARGET_FILE:litheview_qt_demo>
  COMMENT "Deploying the Qt 6.8.3 runtime"
)
