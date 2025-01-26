format:
	find . -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" \) \
		-not -path "./build/*" \
		-not -path "./external/*" \
		-not -path "./references/*" \
		-exec clang-format -i {} +