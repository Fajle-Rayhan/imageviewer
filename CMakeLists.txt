cmake_minimum_required(VERSION 3.20)
project(imageviewer VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Test)

add_executable(imageviewer
  src/main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/imagecanvasview.cpp
  src/imagecanvasview.h
  src/settingsdialog.cpp
  src/settingsdialog.h
  src/appsettings.cpp
  src/appsettings.h
  src/imagedirectorymodel.cpp
  src/imagedirectorymodel.h
  src/editorcommands.cpp
  src/editorcommands.h
  src/openwithservice.cpp
  src/openwithservice.h
  src/trashservice.cpp
  src/trashservice.h
)

target_link_libraries(imageviewer PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)

install(TARGETS imageviewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/imageviewer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/imageviewer-mime.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
install(FILES packaging/icons/imageviewer.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

enable_testing()

add_executable(imageviewer_tests
  tests/test_main.cpp
  tests/test_settings.cpp
  tests/test_model.cpp
  tests/test_editor.cpp
  tests/test_openwith.cpp
  tests/test_viewmode.cpp
  src/appsettings.cpp
  src/appsettings.h
  src/imagedirectorymodel.cpp
  src/imagedirectorymodel.h
  src/editorcommands.cpp
  src/editorcommands.h
  src/openwithservice.cpp
  src/openwithservice.h
)

target_link_libraries(imageviewer_tests PRIVATE Qt6::Test Qt6::Core Qt6::Gui)
add_test(NAME imageviewer_tests COMMAND imageviewer_tests)
