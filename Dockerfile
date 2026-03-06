# syntax=docker/dockerfile:1.7

FROM alpine:3.20 AS builder

RUN apk add --no-cache \
    bash \
    build-base \
    cmake \
    git \
    linux-headers \
    pkgconf \
    py3-pip \
    python3

RUN pip install --no-cache-dir "conan==2.26.2"

WORKDIR /src

COPY CMakeLists.txt conanfile.py ./
COPY *.cpp ./
COPY *.h ./
COPY cmake ./cmake

RUN conan profile detect --force
RUN conan install . --output-folder=build-release -s build_type=Release --build=missing
RUN cmake -B build-release \
    -DCMAKE_TOOLCHAIN_FILE=build-release/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build-release --parallel

FROM alpine:3.20

RUN apk add --no-cache ca-certificates libstdc++

COPY --from=builder /src/build-release/karmabotcpp /usr/local/bin/karmabotcpp

ENTRYPOINT ["/usr/local/bin/karmabotcpp"]
