# Format all source files with clang-format.
# TODO(nlesquoy): Check if it should be integrated with the build system?

format:
	find . -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" \) \
		-not -path "./build/*" \
		-not -path "./external/*" \
		-not -path "./references/*" \
		-not -path "./vk-bootstrap/*" \
		-not -path "./VulkanMemoryAllocator/*" \
		-exec clang-format -i {} +