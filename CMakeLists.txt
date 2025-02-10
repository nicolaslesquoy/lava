cmake_minimum_required(VERSION 3.16)
project(vulkan_lib)

# Find Vulkan and glslc compiler
find_package(Vulkan REQUIRED)

# Add shader compilation
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/add.comp.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders/
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/shaders/add.comp -o ${CMAKE_BINARY_DIR}/shaders/add.comp.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/add.comp
    COMMENT "Compiling add.comp shader"
)
add_custom_target(shaders DEPENDS ${CMAKE_BINARY_DIR}/shaders/add.comp.spv)

# Add vk-bootstrap
add_subdirectory(vk-bootstrap)

# Add VulkanMemoryAllocator as header-only library
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include)

# Project executable
add_executable(vulkan_lib
    src/main.cpp
    src/lvinit.cpp
    src/lvrun.cpp
    src/lvcleanup.cpp
    src/vma.cpp
)
add_dependencies(vulkan_lib shaders)

target_include_directories(vulkan_lib PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(vulkan_lib PRIVATE
    vk-bootstrap
    VulkanMemoryAllocator
    Vulkan::Vulkan
)