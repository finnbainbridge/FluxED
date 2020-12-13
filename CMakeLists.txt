cmake_minimum_required(VERSION 3.0.0)
project(FluxED VERSION 0.1.0)

# include(CTest)
# enable_testing()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig)
if (!PACKAGES_FOUND)
    message("Could not find PkgConfig")
endif()
pkg_check_modules(GTKMM gtkmm-3.0)
include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
link_libraries(${GTKMM_LIBRARIES})

add_executable(FluxED Src/main.cc 
Include/FluxED/FluxEd.hh 
Include/FluxProj/FluxProj.hh 
Src/BuildEditor.cc Src/Editor3D.cc

Src/FluxProj/Project.cc)

add_subdirectory(Flux/FluxGTK)
target_link_libraries(FluxED PUBLIC FluxGTK)

target_include_directories(FluxED PUBLIC Include)
target_include_directories(FluxED PUBLIC Include/FluxProj)

set(ASSIMP_NO_EXPORT OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
add_subdirectory(ThirdParty/assimp)
target_link_libraries(FluxED PUBLIC assimp)

# Add JSON library
add_subdirectory(ThirdParty/nlohmann_json_cmake_fetchcontent)
target_link_libraries(FluxED PUBLIC nlohmann_json)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
