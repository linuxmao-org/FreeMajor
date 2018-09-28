cmake_minimum_required(VERSION 3.3)

project(gmajctl LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

find_package(FLTK REQUIRED)

add_executable(gmajctl
  "sources/main.cc"
  "sources/utility/misc.cc"
  "sources/device/midi.cc"
  "sources/model/parameter.cc"
  "sources/model/patch_loader.cc"
  "sources/model/patch_writer.cc"
  "sources/model/patch.cc"
  "sources/ui/main_window.cc"
  "sources/ui/main_component.cxx"
  "sources/ui/main_component_impl.cc"
  "sources/ui/patch_chooser.cxx"
  "sources/ui/association.cc"
  "sources/ui/midi_out_queue.cc")
target_include_directories(gmajctl
  PRIVATE "sources" "${FLTK_INCLUDE_DIR}")
target_link_libraries(gmajctl PRIVATE "${FLTK_LIBRARIES}")

include(RtMidi)
target_link_libraries(gmajctl PRIVATE RtMidi)

find_package(Threads REQUIRED)
target_link_libraries(gmajctl PRIVATE "${CMAKE_THREAD_LIBS_INIT}")

target_sources(gmajctl PRIVATE "thirdparty/Fl_Knob/Fl_Knob/Fl_Knob.cxx")
target_include_directories(gmajctl PRIVATE "thirdparty/Fl_Knob/Fl_Knob")
