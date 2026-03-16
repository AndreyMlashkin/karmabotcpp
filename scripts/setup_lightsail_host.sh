#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ENV_FILE="${ENV_FILE:-$REPO_DIR/.env}"
DATA_DIR="${DATA_DIR:-$REPO_DIR/data}"
IMAGE_TAG="${IMAGE_TAG:-karmabotcpp:latest}"
CONTAINER_NAME="${CONTAINER_NAME:-karmabotcpp}"
SWAPFILE="${SWAPFILE:-/swapfile}"
SWAP_SIZE_MB="${SWAP_SIZE_MB:-2048}"

log() {
    printf '[setup] %s\n' "$*"
}

require_file() {
    local path="$1"
    if [[ ! -f "$path" ]]; then
        printf 'Missing required file: %s\n' "$path" >&2
        exit 1
    fi
}

ensure_swap() {
    if swapon --show=NAME --noheadings | grep -qx "$SWAPFILE"; then
        log "Swap already active at $SWAPFILE"
        return
    fi

    if [[ ! -f "$SWAPFILE" ]]; then
        log "Creating ${SWAP_SIZE_MB}MB swap file"
        sudo fallocate -l "${SWAP_SIZE_MB}M" "$SWAPFILE" || \
            sudo dd if=/dev/zero of="$SWAPFILE" bs=1M count="$SWAP_SIZE_MB" status=progress
        sudo chmod 600 "$SWAPFILE"
        sudo mkswap "$SWAPFILE"
    fi

    sudo swapon "$SWAPFILE"
    if ! grep -qE "^${SWAPFILE//\//\\/} " /etc/fstab; then
        echo "$SWAPFILE none swap sw 0 0" | sudo tee -a /etc/fstab >/dev/null
    fi
}

install_packages() {
    log "Updating apt metadata"
    sudo apt-get update

    # Debian's "docker" package is not Docker Engine. Remove it if it was installed by mistake.
    if dpkg -s docker >/dev/null 2>&1 || dpkg -s wmdocker >/dev/null 2>&1; then
        log "Removing incorrect docker packages"
        sudo apt-get purge -y docker wmdocker || true
    fi

    log "Installing host dependencies"
    sudo apt-get install -y ca-certificates git docker.io
    sudo systemctl enable --now docker

    if ! id -nG "$USER" | grep -qw docker; then
        sudo usermod -aG docker "$USER"
        log "Added $USER to docker group. New login required to use docker without sudo."
    fi
}

load_env() {
    require_file "$ENV_FILE"

    log "Loading environment from $ENV_FILE"
    set -a
    # shellcheck disable=SC1090
    source "$ENV_FILE"
    set +a

    : "${TELEGRAM_BOT_TOKEN:?TELEGRAM_BOT_TOKEN is not set in $ENV_FILE}"
    : "${CHAT_IDS:?CHAT_IDS is not set in $ENV_FILE}"
    : "${ENABLE_EXCHANGE_RATES:=1}"
}

build_image() {
    log "Building Docker image $IMAGE_TAG"
    sudo docker build -t "$IMAGE_TAG" "$REPO_DIR"
}

run_container() {
    mkdir -p "$DATA_DIR"

    if sudo docker ps -a --format '{{.Names}}' | grep -qx "$CONTAINER_NAME"; then
        log "Removing existing container $CONTAINER_NAME"
        sudo docker rm -f "$CONTAINER_NAME" >/dev/null
    fi

    log "Starting container $CONTAINER_NAME"
    sudo docker run -d \
        --name "$CONTAINER_NAME" \
        --restart unless-stopped \
        --network host \
        -w /data \
        -v "$DATA_DIR:/data" \
        -e TELEGRAM_BOT_TOKEN="$TELEGRAM_BOT_TOKEN" \
        -e CHAT_IDS="$CHAT_IDS" \
        -e ENABLE_EXCHANGE_RATES="$ENABLE_EXCHANGE_RATES" \
        "$IMAGE_TAG" >/dev/null
}

show_status() {
    log "Container status"
    sudo docker ps --filter "name=$CONTAINER_NAME"
    echo
    log "Recent logs"
    sudo docker logs --tail 50 "$CONTAINER_NAME"
}

main() {
    require_file "$REPO_DIR/Dockerfile"

    ensure_swap
    install_packages
    load_env
    build_image
    run_container
    show_status
}

main "$@"
