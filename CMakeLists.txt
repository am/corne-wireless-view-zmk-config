# Simple vertical nice!view status screen, built from this config repo.
# It provides zmk_display_status_screen(), as does the stock nice!view widget,
# so CONFIG_NICE_VIEW_WIDGET_STATUS must be off.
if(CONFIG_NICE_VIEW_SIMPLE)
  if(CONFIG_NICE_VIEW_WIDGET_STATUS)
    message(FATAL_ERROR "CONFIG_NICE_VIEW_SIMPLE replaces the stock nice!view status screen; "
      "set CONFIG_NICE_VIEW_WIDGET_STATUS=n")
  endif()

  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_view/assets.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_view/draw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simple_view/screen.c
  )
endif()
