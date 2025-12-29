cmake_minimum_required(VERSION 3.20)
project(ObscuraRT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Verbose output for debugging
set(CMAKE_VERBOSE_MAKEFILE ON)

# Find Vulkan (required)
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan found at: ${Vulkan_LIBRARIES}")

# Find OpenCV for frame input (optional but needed for V4L2/FFmpeg)
find_package(OpenCV REQUIRED COMPONENTS core videoio)
message(STATUS "OpenCV found: ${OpenCV_INCLUDE_DIRS}")

# Find GLFW for display window
find_package(glfw3 REQUIRED)

# Shader compilation target
add_custom_target(shaders ALL)

# Function to compile GLSL shaders to SPIR-V
function(compile_shader shader_file)
  get_filename_component(shader_name ${shader_file} NAME)
  set(spv_file "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.spv")
  
  add_custom_command(
    OUTPUT ${spv_file}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND Vulkan::glslc -o ${spv_file} ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file}
    VERBATIM
  )
  
  add_custom_target(shader_${shader_name} ALL DEPENDS ${spv_file})
  add_dependencies(shaders shader_${shader_name})
endfunction()

# Compile compute shader
compile_shader(shaders/pixelation.comp)
compile_shader(shaders/display.vert)
compile_shader(shaders/display.frag)

# Main executable
add_executable(obscura_rt
  src/main.cpp
  src/vulkan_context.cpp
  src/frame_grabber.cpp
  src/compute_pipeline.cpp
  src/display_pipeline.cpp
)

# Include directories
target_include_directories(obscura_rt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm
  ${OpenCV_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(obscura_rt PRIVATE
  Vulkan::Vulkan
  ${OpenCV_LIBRARIES}
  glfw
)

# Add dependencies
add_dependencies(obscura_rt shaders)

# Copy shader SPIR-V files to binary directory at runtime
add_custom_command(TARGET obscura_rt POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_BINARY_DIR}/shaders
  $<TARGET_FILE_DIR:obscura_rt>/shaders
)
