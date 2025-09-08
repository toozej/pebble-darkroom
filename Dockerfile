# Latest stable Debian slim
FROM debian:stable-slim AS base

# Prevent tz/locale prompts during apt operations
ENV DEBIAN_FRONTEND=noninteractive

# Pebble/tooling environment
ENV \
  # Ensure curl-based installers work without TTY prompts
  PIP_DISABLE_PIP_VERSION_CHECK=1 \
  # pebble-tool analytics off to keep builds non-interactive
  PEBBLE_TOOL_NO_ANALYTICS=1 \
  # Add uv-installed tools and shims to PATH for all users
  PATH="/home/pebble/.local/bin:/home/pebble/.local/share/uv/tools/bin:${PATH}"

# Install required runtime dependencies for Pebble SDK and tooling.
# - Keep to a single apt layer, use --no-install-recommends, and clean lists afterwards.
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
      python3 \
      python3-venv \
      python3-pip \
      nodejs \
      npm \
      libsdl1.2debian \
      libfdt1 \
      ca-certificates \
      curl \
      git \
      xz-utils \
      bzip2 \
      unzip \
      build-essential \
      tini; \
    rm -rf /var/lib/apt/lists/*

# Create an unprivileged user to run builds.
# Use a fixed UID/GID for predictable file ownership in bind mounts.
ARG USER=pebble
ARG UID=1000
ARG GID=1000
RUN set -eux; \
    groupadd -g "${GID}" "${USER}"; \
    useradd -l -m -u "${UID}" -g "${GID}" -s /bin/bash "${USER}"

# Install uv for the non-root user (recommended install script).
# This places `uv` under ~/.local/bin and sets up shims under ~/.local/share/uv/tools/bin.
USER ${USER}
WORKDIR /home/${USER}

# Install uv
RUN set -eux; \
    curl -LsSf https://astral.sh/uv/install.sh | sh

# Install pebble-tool via uv (user-local, no root required)
RUN set -eux; \
    uv tool install pebble-tool

# Preinstall the latest Pebble SDK into the image to avoid downloads at runtime.
# This will populate the user's ~/.pebble-sdk directory.
RUN set -eux; \
    pebble --version; \
    pebble sdk install latest


FROM base AS test
# Build and run unit tests for pebble-darkroom
USER root
COPY app/pebble-darkroom/ /workspace/pebble-darkroom/
RUN chown -R pebble:pebble /workspace && chown -R pebble:pebble /workspace/pebble-darkroom
USER pebble
WORKDIR /workspace
RUN pebble new-project pebble-test && \
    cp -r pebble-darkroom/src pebble-test/ && \
    cp -r pebble-darkroom/tests pebble-test/
WORKDIR /workspace/pebble-test
RUN gcc -I./tests -Isrc/c -c tests/unity.c -o unity.o && \
    gcc -I./tests -Isrc/c -c tests/test_runner.c -o test_runner.o && \
    gcc -I./tests -Isrc/c -c tests/settings.c -o settings.o && \
    gcc -I./tests -Isrc/c -c tests/test_settings.c -o test_settings.o && \
    gcc -I./tests -Isrc/c -c tests/test_timer.c -o test_timer.o && \
    gcc -I./tests -Isrc/c -c tests/test_display.c -o test_display.o && \
    gcc unity.o test_runner.o settings.o test_settings.o test_timer.o test_display.o -lm -o test_runner && \
    ./test_runner


FROM base AS final

# Define mountable volume for projects
VOLUME /workspace/

# Set default working directory
WORKDIR /workspace/

# Use Tini as PID 1 for signal handling; drop into a shell by default.
# (Tini was installed from Debian above.)
ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/bin/bash"]


