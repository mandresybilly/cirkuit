include_directories(${CMAKE_CURRENT_BINARY_DIR} ..)
set (cirkuitwidgets_SRCS templatechoosedialog.cpp 
    imageview.cpp
    previewwidget.cpp
    logviewwidget.cpp
    backendchoosedialog.cpp
    widgetfloater.cpp
)

kde4_add_ui_files( cirkuitwidgets_SRCS backendchooser.ui templatechooser.ui)
kde4_add_library( cirkuitwidgets STATIC ${cirkuitwidgets_SRCS} )
