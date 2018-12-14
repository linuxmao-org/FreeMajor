cmake_minimum_required(VERSION 3.3)

project(FreeMajor VERSION "0.1" LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

include(GNUInstallDirs)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(ENABLE_GETTEXT_INIT FALSE)
else()
  set(ENABLE_GETTEXT_INIT TRUE)
endif()

option(ENABLE_GETTEXT "Enable internationalization with gettext" "${ENABLE_GETTEXT_INIT}")

if(ENABLE_GETTEXT)
  find_package(Intl REQUIRED)
  find_package(Gettext REQUIRED)
endif()

set(FLTK_SKIP_OPENGL ON)
set(FLTK_SKIP_FORMS ON)
set(FLTK_SKIP_IMAGES ON)
set(FLTK_SKIP_FLUID ON)
find_package(FLTK REQUIRED)

set(MACOSX_BUNDLE_ICON_FILE "FreeMajor.icns")

add_executable(FreeMajor WIN32 MACOSX_BUNDLE
  "sources/main.cc"
  "sources/app_i18n.cc"
  "sources/app_win32.rc"
  "sources/utility/misc.cc"
  "sources/device/midi.cc"
  "sources/device/midi_apis.cc"
  "sources/model/parameter.cc"
  "sources/model/patch_loader.cc"
  "sources/model/patch_writer.cc"
  "sources/model/patch.cc"
  "sources/ui/main_window.cc"
  "sources/ui/main_component.cxx"
  "sources/ui/main_component_impl.cc"
  "sources/ui/patch_chooser.cxx"
  "sources/ui/modifiers_editor.cxx"
  "sources/ui/modifiers_editor_impl.cc"
  "sources/ui/singlemod_editor.cxx"
  "sources/ui/receive_dialog.cxx"
  "sources/ui/receive_dialog_impl.cc"
  "sources/ui/eq_display.cc"
  "sources/ui/matrix_display.cc"
  "sources/ui/hyperlink_button.cc"
  "sources/ui/association.cc"
  "sources/ui/midi_out_queue.cc")
target_compile_definitions(FreeMajor
  PRIVATE "LOCALE_DIRECTORY=\"${CMAKE_INSTALL_FULL_LOCALEDIR}\"")
target_include_directories(FreeMajor
  PRIVATE "sources" "${FLTK_INCLUDE_DIR}")
target_link_libraries(FreeMajor PRIVATE "${FLTK_LIBRARIES}")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  add_custom_command(TARGET FreeMajor POST_BUILD
    COMMAND "${CMAKE_INSTALL_NAME_TOOL}" -add_rpath "@executable_path/../Frameworks"
    $<TARGET_FILE:FreeMajor>)
endif()

include(RtMidi)
target_link_libraries(FreeMajor PRIVATE RtMidi)

find_package(Threads REQUIRED)
target_link_libraries(FreeMajor PRIVATE "${CMAKE_THREAD_LIBS_INIT}")

if(ENABLE_GETTEXT)
  target_compile_definitions(FreeMajor PRIVATE "ENABLE_NLS=1")
  target_include_directories(FreeMajor PRIVATE ${Intl_INCLUDE_DIRS})
  target_link_libraries(FreeMajor PRIVATE ${Intl_LIBRARIES})
  target_include_directories(FreeMajor PRIVATE "thirdparty/gettext/include")
endif()

# target_sources(FreeMajor PRIVATE "thirdparty/Fl_Knob/Fl_Knob/Fl_Knob.cxx")
# target_include_directories(FreeMajor PRIVATE "thirdparty/Fl_Knob/Fl_Knob")

## Installation
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  install(TARGETS FreeMajor DESTINATION "${CMAKE_INSTALL_BINDIR}")
else()
  install(TARGETS FreeMajor DESTINATION ".")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  install(FILES "resources/application/FreeMajor.icns" DESTINATION "${PROJECT_NAME}.app/Contents/Resources")
elseif(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  install(FILES "resources/application/FreeMajor.png" "resources/application/FreeMajor.svg" DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps")
  install(FILES "resources/application/FreeMajor.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
  install(FILES
    "resources/mime/freemajor-realmajor.xml"
    "resources/mime/freemajor-realpatch.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")
endif()

## Translations
if(ENABLE_GETTEXT)
  set(TRANSLATIONS "fr")
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MO_DESTINATION "${CMAKE_INSTALL_LOCALEDIR}")
  else()
    set(MO_DESTINATION "${PROJECT_NAME}.app/Contents/Resources/locale")
  endif()
  foreach(translation ${TRANSLATIONS})
    gettext_process_po_files("${translation}" ALL INSTALL_DESTINATION "${MO_DESTINATION}" PO_FILES
      "po/${translation}/FreeMajor.po")
  endforeach()
endif()

## Packaging
include(CPackLists.txt)
