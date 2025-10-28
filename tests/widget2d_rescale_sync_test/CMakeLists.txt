cmake_minimum_required(VERSION 3.21)
project(widget2d_rescale_sync_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_LIST_DIR}/../../deps/Qt/6.7.2/msvc2019_64"
    "${CMAKE_CURRENT_LIST_DIR}/../../deps/vtk-9.3.0/install"
    ${CMAKE_PREFIX_PATH}
)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Concurrent)
find_package(VTK REQUIRED COMPONENTS CommonCore CommonDataModel)

add_executable(widget2d_rescale_sync_test widget2d_rescale_sync_test.cpp)
target_sources(widget2d_rescale_sync_test PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../src/gui/widget2dframebuilder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/gui/widget2dframecache.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/gui/widget2dframerenderer.cpp"
)
target_include_directories(widget2d_rescale_sync_test PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../src"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/core"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/gui"
)

target_link_libraries(widget2d_rescale_sync_test PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    ${VTK_LIBRARIES}
)

if (VTK_LIBRARIES)
    vtk_module_autoinit(
        TARGETS widget2d_rescale_sync_test
        MODULES ${VTK_LIBRARIES}
    )
endif()
