cmake_minimum_required(VERSION 3.16)
project(vulkan_example)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Add vk-bootstrap
add_subdirectory(vk-bootstrap)

# Add VulkanMemoryAllocator as header-only library
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include)

# Project executable
add_executable(vulkan_example 
    src/example.cpp
)

target_include_directories(vulkan_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(vulkan_example PRIVATE
    vk-bootstrap
    VulkanMemoryAllocator
    Vulkan::Vulkan
)