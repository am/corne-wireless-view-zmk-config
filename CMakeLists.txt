# Simple vertical nice!view status screen, built from this config repo.
# It provides zmk_display_status_screen(), as does the stock nice!view widget,
# so CONFIG_NICE_VIEW_WIDGET_STATUS must be off.
if(CONFIG_NICE_VIEW_SIMPLE)
  if(CONFIG_NICE_VIEW_WIDGET_STATUS)
    message(FATAL_ERROR "CONFIG_NICE_VIEW_SIMPLE replaces the stock nice!view status screen; "
      "set CONFIG_NICE_VIEW_WIDGET_STATUS=n")
  endif()

  # Canvas helpers (rotate_canvas, init_label_dsc, ...) come from ZMK's own
  # nice_view shield, which only compiles them for its stock widget.
  set(nice_view_widgets ${APPLICATION_SOURCE_DIR}/boards/shields/nice_view/widgets)
  target_include_directories(app PRIVATE ${nice_view_widgets} ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_view)
  target_sources(app PRIVATE
    ${nice_view_widgets}/util.c
    ${nice_view_widgets}/bolt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_view/draw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_view/screen.c
  )
endif()
