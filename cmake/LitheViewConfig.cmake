if(TARGET LitheView::LitheView)
  return()
endif()

get_filename_component(_LITHEVIEW_PREFIX
  "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

add_library(LitheView::LitheView SHARED IMPORTED)
set_target_properties(LitheView::LitheView PROPERTIES
  IMPORTED_IMPLIB "${_LITHEVIEW_PREFIX}/lib/litheview.lib"
  IMPORTED_LOCATION "${_LITHEVIEW_PREFIX}/bin/litheview.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${_LITHEVIEW_PREFIX}/include"
)

unset(_LITHEVIEW_PREFIX)
