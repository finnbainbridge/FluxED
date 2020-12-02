cmake_minimum_required(VERSION 3.0.0)
project(FluxED VERSION 0.1.0)

# include(CTest)
# enable_testing()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
if (!PACKAGES_FOUND)
    message("Could not find PkgConfig")
endif()
pkg_check_modules(GTKMM gtkmm-3.0)
include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
link_libraries(${GTKMM_LIBRARIES})

add_executable(FluxED src/main.cpp)

add_subdirectory(Flux/FluxGTK)
target_link_libraries(FluxED PUBLIC FluxGTK)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
