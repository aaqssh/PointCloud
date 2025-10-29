cmake_minimum_required(VERSION 3.16)

project(RollingBallSim VERSION 1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    OpenGLWidgets
)

# Source files - ONLY the files that exist
set(PROJECT_SOURCES
    main.cpp
    renderwindow.cpp
    renderwindow.h
    trianglesurface.cpp
    trianglesurface.h
    rollingball.cpp
    rollingball.h
    camera.cpp
    camera.h
    vector3d.h
    vertex.h
)

# Create executable
add_executable(RollingBallSim ${PROJECT_SOURCES})

# Link Qt libraries
target_link_libraries(RollingBallSim PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Windows-specific OpenGL linking
if(WIN32)
    target_link_libraries(RollingBallSim PRIVATE
        opengl32.lib
        gdi32.lib
    )
endif()

# macOS-specific
if(APPLE)
    target_link_libraries(RollingBallSim PRIVATE
        "-framework OpenGL"
    )
endif()

# Linux-specific
if(UNIX AND NOT APPLE)
    target_link_libraries(RollingBallSim PRIVATE
        GL
    )
endif()

# Include directories
target_include_directories(RollingBallSim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Copy surface.txt to build directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/surface.txt")
    add_custom_command(TARGET RollingBallSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.txt"
        "$<TARGET_FILE_DIR:RollingBallSim>/surface.txt"
        COMMENT "Copying surface.txt to build directory"
    )
endif()

# Print build info
message(STATUS "")
message(STATUS "=== RollingBallSim Configuration ===")
message(STATUS "Qt6 path: ${Qt6_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "====================================")
message(STATUS "")
