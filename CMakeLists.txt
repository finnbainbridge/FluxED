cmake_minimum_required(VERSION 3.0.0)
project(FluxED VERSION 0.1.0)

# include(CTest)
# enable_testing()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

# Enable hell
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic") # -Wextra -Weverything")

find_package(PkgConfig)
if (!PACKAGES_FOUND)
    message("Could not find PkgConfig")
endif()
pkg_check_modules(GTKMM gtkmm-3.0)
include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
link_libraries(${GTKMM_LIBRARIES})

add_executable(FluxED Src/main.cc 

# Headers
Include/FluxED/FluxEd.hh 
Include/FluxProj/FluxProj.hh 
Include/FluxED/3DView.hh

# 3D Editor
Src/BuildEditor.cc 
Src/Editor3D.cc
Src/3DView/OrbitCamera.cc

# 3D Editor components
Src/Components/TransformCom.cc

# FluxProj
Src/Importers/AssimpImporter.cc
Src/FluxProj/Project.cc)

set(FLUX_GCC_PROFILE ON)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=bounds")

if (FLUX_GCC_PROFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fstack-protector-strong")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

add_subdirectory(Flux/FluxGTK)
target_link_libraries(FluxED PUBLIC FluxGTK)

target_include_directories(FluxED PUBLIC Include)
target_include_directories(FluxED PUBLIC Include/FluxProj)

# Apparently, no export breaks assimp
set(ASSIMP_NO_EXPORT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/assimp)
target_link_libraries(FluxED PUBLIC assimp)

# Add JSON library
add_subdirectory(ThirdParty/nlohmann_json_cmake_fetchcontent)
target_link_libraries(FluxED PUBLIC nlohmann_json)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# ============================================================
# Playground
# ============================================================
# Big hack
add_executable(FluxPlayground Src/Playground/Playground.cc Flux/FluxGTK/FluxEngine/Src/OpenGL/GLFW/GLFW.cc)
# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(Flux/FluxGTK/FluxEngine/ThirdParty/glfw)
target_link_libraries(FluxPlayground PUBLIC glfw)
target_link_libraries(FluxPlayground PUBLIC FluxEngine)