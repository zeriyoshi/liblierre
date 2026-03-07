ARG PLATFORM=${BUILDPLATFORM:-linux/amd64}
ARG DEBIAN_VERSION=trixie
ARG ENABLE_VALGRIND=1
ARG ENABLE_CLANG=1
ARG ENABLE_EMSDK=1
ARG EMSDK_VERSION=0

FROM --platform=${PLATFORM} debian:${DEBIAN_VERSION} AS base

ENV TZ="Asia/Tokyo"

ENV LANG="C.UTF-8"
ENV LC_ALL="C.UTF-8"

ENV DEBIAN_FRONTEND="noninteractive"
RUN rm -f /etc/apt/apt.conf.d/docker-clean; \
    echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > "/etc/apt/apt.conf.d/keep-cache"

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    apt-get update && \
    apt-get --no-install-recommends install -y \
      "ca-certificates" "tzdata" "lsb-release" "curl" \
      "bash" \
      "git" \
      "xz-utils" \
      "gcc" "g++" \
      "make" "cmake" \
      "nasm" \
      "libc6-dev" "libc6-dbg" "libstdc++-14-dev" \
      "pkg-config" \
      "python3" \
      "lcov"

SHELL [ "/bin/bash", "-c" ]
CMD [ "/bin/bash" ]

# Clang/LLVM
ARG ENABLE_CLANG
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    if test "${ENABLE_CLANG}" != "0"; then \
      apt-get update && \
      echo "deb [trusted=yes] http://apt.llvm.org/trixie/ llvm-toolchain-trixie main" > "/etc/apt/sources.list.d/llvm.list" && \
      echo "deb [trusted=yes] http://apt.llvm.org/trixie/ llvm-toolchain-trixie-21 main" >> "/etc/apt/sources.list.d/llvm.list" && \
      apt-get update && \
      apt-get install --no-install-recommends -y \
        "clang-21" "clang-tools-21" "clang-format-21" "clang-tidy-21" \
        "libclang-rt-21-dev" "lld-21" "lldb-21" \
        "libc++-21-dev" "libc++abi-21-dev" \
        "llvm-21" "llvm-21-dev" "llvm-21-runtime" && \
      update-alternatives --install "/usr/bin/clang" clang "/usr/bin/clang-21" 100 && \
      update-alternatives --install "/usr/bin/clang++" clang++ "/usr/bin/clang++-21" 100 && \
      update-alternatives --install "/usr/bin/clang-format" clang-format "/usr/bin/clang-format-21" 100 && \
      update-alternatives --install "/usr/bin/clang-tidy" clang-tidy "/usr/bin/clang-tidy-21" 100 && \
      update-alternatives --install "/usr/bin/lldb" lldb "/usr/bin/lldb-21" 100 && \
      update-alternatives --install "/usr/bin/ld.lld" ld.lld "/usr/bin/ld.lld-21" 100 && \
      update-alternatives --install "/usr/bin/llvm-symbolizer" llvm-symbolizer "/usr/bin/llvm-symbolizer-21" 100 && \
      update-alternatives --install "/usr/bin/llvm-config" llvm-config "/usr/bin/llvm-config-21" 100; \
    fi

COPY . "/project"

# Emscripten SDK
ARG ENABLE_EMSDK
ARG EMSDK_VERSION
RUN set -e; \
    if test "${ENABLE_EMSDK}" != "0"; then \
      cd "/project/third_party/emsdk" && \
        if test "${EMSDK_VERSION}" = "0"; then \
          EM_GIT_TAG="$(git describe --tags --abbrev=0)"; \
        else \
          EM_GIT_TAG="${EMSDK_VERSION}"; \
        fi && \
        ./emsdk install "${EM_GIT_TAG}" && \
        ./emsdk activate "${EM_GIT_TAG}" && \
        echo ". \"$(pwd)/emsdk_env.sh\"" >> "/etc/profile.d/emsdk.sh" && \
      cd -; \
    fi

ENV EMSDK_QUIET=1

# Valgrind
ARG ENABLE_VALGRIND
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    if test "${ENABLE_VALGRIND}" != "0"; then \
      apt-get update && \
      apt-get --no-install-recommends install -y \
        "build-essential" "autotools-dev" "automake" "autoconf" "libtool" \
        "libc6-dev" "linux-libc-dev" \
        "libxml2-dev" && \
      cd "/project/third_party/valgrind" && \
      ./autogen.sh && \
      ./configure && \
      make -j"$(nproc)" && \
      make install && \
      cd -; \
    fi

WORKDIR "/project"

# CI target
FROM --platform=${PLATFORM} base AS ci

WORKDIR "/project"
CMD [ "/project/ci.sh" ]

# Development container
FROM --platform=${PLATFORM} base AS devcontainer

ENV DEBIAN_FRONTEND=""

ENV PIP_BREAK_SYSTEM_PACKAGES=1
ENV PIP_ROOT_USER_ACTION="ignore"

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    apt-get update && \
    apt-get --no-install-recommends install -y \
      "ssh" "gdb" "xxd" "bash" \
      "vim" "ripgrep" \
      "guetzli" "imagemagick" "python3" "python3-pip" \
      "exiftool" "pngtools" && \
    update-alternatives --install "/usr/bin/python" python "$(command -v "python3")" 100 && \
    update-alternatives --install "/usr/bin/pip" pip "$(command -v "pip3")" 100 && \
    pip install --resume-retries=5 --no-cache-dir "numpy" "opencv-python" "Pillow" "scikit-image" "flake8" "black"

ENV EDITOR="vim"
ENV VISUAL="vim"
