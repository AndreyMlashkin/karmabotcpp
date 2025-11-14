conan install . --output-folder=build-debug -s build_type=Debug --build=missing && \
cmake -B build-debug -DCMAKE_TOOLCHAIN_FILE=build-debug/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug && \
cmake --build build-debug

