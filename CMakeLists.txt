cmake_minimum_required(VERSION 3.3)

project(FreeMajor VERSION "0.1" LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

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

add_executable(FreeMajor WIN32
  "sources/main.cc"
  "sources/app_i18n.cc"
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
  "sources/ui/eq_display.cc"
  "sources/ui/matrix_display.cc"
  "sources/ui/association.cc"
  "sources/ui/midi_out_queue.cc")
target_compile_definitions(FreeMajor
  PRIVATE "LOCALE_DIRECTORY=\"${CMAKE_INSTALL_FULL_LOCALEDIR}\"")
target_include_directories(FreeMajor
  PRIVATE "sources" "${FLTK_INCLUDE_DIR}")
target_link_libraries(FreeMajor PRIVATE "${FLTK_LIBRARIES}")

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
install(TARGETS FreeMajor DESTINATION "${CMAKE_INSTALL_BINDIR}")

## Translations
if(ENABLE_GETTEXT)
  set(TRANSLATIONS "fr")
  foreach(translation ${TRANSLATIONS})
    gettext_process_po_files("${translation}" ALL INSTALL_DESTINATION "${CMAKE_INSTALL_LOCALEDIR}" PO_FILES
      "po/${translation}/FreeMajor.po")
  endforeach()
endif()

## Packaging
include(CPackLists.txt)
