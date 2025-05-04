cmake_minimum_required(VERSION 3.10)
project(VulkanSharedTextures)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required libraries
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(OpenCV REQUIRED)

# stb_image for image loading
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)
include_directories(${stb_SOURCE_DIR})
    
# Includes
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale
)

# Define common source files
set(COMMON_SOURCES
    src/core/vulkan_context.cpp
    src/core/vulkan_device.cpp
    src/core/vulkan_utils.cpp
    src/core/swapchain.cpp
    src/app/producer_app.cpp
    src/app/consumer_app.cpp
    src/shm/shm_writer.cpp
    src/shm/shm_viewer.cpp
    src/media/texture_image.cpp
    src/media/texture_video.cpp
    src/media/image_loader.cpp
    src/media/stb_image.cpp
    src/media/video_loader.cpp
    src/media/frame_queue.cpp
    src/core/pipeline.cpp
    src/core/descriptor_manager.cpp
    src/core/vertex_definitions.cpp
    src/tools/benchmark.cpp
    src/utils/file_utils.cpp
    src/utils/mode_probe.cpp
    src/memory/shm_handler.cpp
    src/memory/shm_video_handler.cpp
    src/memory/download_texture.cpp
    src/ipc/fd_passing.cpp
    src/window/glfw_window.cpp
    src/window/sdl_window.cpp
)

# Create shared library
add_library(VulkanSharedTextures SHARED ${COMMON_SOURCES})
target_include_directories(VulkanSharedTextures PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(VulkanSharedTextures
    Vulkan::Vulkan 
    SDL3::SDL3 
    SDL3_image::SDL3_image 
    glfw 
    glm 
    pthread 
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_videoio
)

# Producer
add_executable(vst_producer 
        src/producer/main.cpp
        src/core/vulkan_context.cpp)
target_link_libraries(vst_producer 
    Vulkan::Vulkan 
    SDL3::SDL3 
    SDL3_image::SDL3_image 
    glfw 
    glm 
    pthread 
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_videoio
)
target_include_directories(vst_producer PRIVATE include)
target_sources(vst_producer PRIVATE
    src/producer/main.cpp
    src/core/vulkan_context.cpp
    src/core/vulkan_device.cpp
    src/core/vulkan_utils.cpp
    src/core/swapchain.cpp
    src/app/producer_app.cpp
    src/shm/shm_writer.cpp
    src/shm/shm_viewer.cpp
    src/media/texture_image.cpp
    src/media/texture_video.cpp
    src/media/image_loader.cpp
    src/media/stb_image.cpp
    src/media/video_loader.cpp
    src/media/frame_queue.cpp
    src/core/pipeline.cpp
    src/core/descriptor_manager.cpp
    src/core/vertex_definitions.cpp
    src/tools/benchmark.cpp
    src/utils/file_utils.cpp
    src/memory/shm_handler.cpp
    src/memory/shm_video_handler.cpp
    src/memory/download_texture.cpp
    src/ipc/fd_passing.cpp
    src/window/glfw_window.cpp
    src/window/sdl_window.cpp
)

# Consumer
add_executable(vst_consumer
    src/consumer/main.cpp
    src/app/consumer_app.cpp
    src/shm/shm_writer.cpp
    src/shm/shm_viewer.cpp
    src/memory/shm_video_handler.cpp
    src/utils/file_utils.cpp
    src/media/texture_image.cpp
    src/media/texture_video.cpp
    src/media/image_loader.cpp
    src/media/stb_image.cpp
    src/utils/mode_probe.cpp
    src/core/descriptor_manager.cpp
    src/core/pipeline.cpp
    src/core/vertex_definitions.cpp
    src/core/vulkan_context.cpp
    src/core/vulkan_device.cpp
    src/core/vulkan_utils.cpp
    src/core/swapchain.cpp
    src/ipc/fd_passing.cpp
)
target_include_directories(vst_consumer PRIVATE include)
target_link_libraries(vst_consumer Vulkan::Vulkan 
    SDL3::SDL3
    SDL3_image::SDL3_image 
    glfw 
    glm 
    pthread
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_videoio
)

# Find glslangValidator
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/bin)

function(compile_shader SHADER_NAME SHADER_TYPE TARGET_NAME)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders/
        COMMAND ${GLSL_VALIDATOR} -V ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.${SHADER_TYPE}
                -o ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.${SHADER_TYPE}
        COMMENT "Compiling: ${SHADER_NAME}.${SHADER_TYPE}"
    )
    add_custom_target(${TARGET_NAME} DEPENDS ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
endfunction()

# Compile shaders
compile_shader(vertex vert vertex_shader)
compile_shader(fragment frag fragment_shader)

# Ensure shaders are built before the app
add_dependencies(vst_producer vertex_shader fragment_shader)

# Installation
install(TARGETS VulkanSharedTextures vst_producer vst_consumer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install header files
install(DIRECTORY include/ DESTINATION include/VulkanSharedTextures)
