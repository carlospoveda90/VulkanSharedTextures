cmake_minimum_required(VERSION 3.10)
project(VulkanSharedTextures)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required libraries
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)

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

# Producer
add_executable(vst_producer 
        src/producer/main.cpp
        src/core/vulkan_context.cpp)
target_link_libraries(vst_producer Vulkan::Vulkan glfw glm pthread)
target_include_directories(vst_producer PRIVATE include)
target_sources(vst_producer PRIVATE
    src/producer/main.cpp
    src/core/vulkan_context.cpp
    src/core/vulkan_device.cpp
    src/core/swapchain.cpp
    src/app/producer_app.cpp
    src/shm/shm_writer.cpp
    src/media/image_loader.cpp
    src/media/stb_image.cpp
    src/core/pipeline.cpp
    src/core/descriptor_manager.cpp
    src/core/vertex_definitions.cpp
    src/tools/benchmark.cpp
    src/memory/shm_handler.cpp
    src/memory/download_texture.cpp
    src/ipc/fd_passing.cpp
)

# Consumer
add_executable(vst_consumer
    src/consumer/main.cpp
    src/app/consumer_app.cpp
    src/shm/shm_viewer.cpp
    src/utils/mode_probe.cpp
    src/core/descriptor_manager.cpp
    src/core/pipeline.cpp
    src/core/vertex_definitions.cpp
    src/core/vulkan_context.cpp
    src/core/vulkan_device.cpp
    src/core/swapchain.cpp
    src/ipc/fd_passing.cpp
)
target_include_directories(vst_consumer PRIVATE include)
target_link_libraries(vst_consumer Vulkan::Vulkan SDL3::SDL3 glfw glm pthread)


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
