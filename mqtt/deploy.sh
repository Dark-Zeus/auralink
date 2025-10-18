#!/usr/bin/env bash
set -euo pipefail

MOSQ_DIR="./mosquitto"
WEB_DIR="./mqttx-web"
MOSQ_IMAGE="auralink-mosquitto:latest"
WEB_IMAGE="auralink-mqttxweb:latest"
MOSQ_CONT="auralink-mosquitto"
WEB_CONT="auralink-mqttxweb"
USER="admin"

if command -v cygpath >/dev/null 2>&1; then
  ROOT_WIN=$(cygpath -w "$PWD")
  MOSQ_CONF_MNT="$ROOT_WIN\\mosquitto\\mosquitto.conf:/mosquitto/config/mosquitto.conf:ro"
  PASS_FILE_MNT="$ROOT_WIN\\mosquitto\\passwords:/mosquitto/config/passwords:ro"
  DATA_DIR_MNT="$ROOT_WIN\\mosquitto\\data:/mosquitto/data"
  LOG_DIR_MNT="$ROOT_WIN\\mosquitto\\log:/mosquitto/log"
else
  # Linux/macOS fallback
  ROOT_UNIX="$PWD"
  MOSQ_CONF_MNT="$ROOT_UNIX/mosquitto/mosquitto.conf:/mosquitto/config/mosquitto.conf:ro"
  PASS_FILE_MNT="$ROOT_UNIX/mosquitto/passwords:/mosquitto/config/passwords:ro"
  DATA_DIR_MNT="$ROOT_UNIX/mosquitto/data:/mosquitto/data"
  LOG_DIR_MNT="$ROOT_UNIX/mosquitto/log:/mosquitto/log"
fi

read -sp "Enter password for MQTT user '${USER}': " PASSWORD
echo ""

echo "==> Cleaning old containers..."
docker rm -f "$MOSQ_CONT" "$WEB_CONT" >/dev/null 2>&1 || true

mkdir -p "$MOSQ_DIR/data" "$MOSQ_DIR/log"

sed -i 's/\r$//' "$MOSQ_DIR/mosquitto.conf" || true

echo "==> Building Mosquitto..."
docker build -t "$MOSQ_IMAGE" "$MOSQ_DIR"

echo "==> Building MQTTX Web..."
docker build -t "$WEB_IMAGE" "$WEB_DIR"

PASS_FILE="$MOSQ_DIR/passwords"
echo "==> Generating hashed password file..."
docker run --rm eclipse-mosquitto:2 sh -lc \
  "mosquitto_passwd -b -c /tmp/passwd '$USER' '$PASSWORD' && cat /tmp/passwd" \
  > "$PASS_FILE"

if ! grep -q '^admin:\$' "$PASS_FILE"; then
  echo "ERROR: password file not created correctly." >&2
  exit 1
fi

echo "==> Starting Mosquitto..."
docker run -d --name "$MOSQ_CONT" \
  -p 1883:1883 -p 9001:9001 \
  -v "$MOSQ_CONF_MNT" \
  -v "$PASS_FILE_MNT" \
  -v "$DATA_DIR_MNT" \
  -v "$LOG_DIR_MNT" \
  "$MOSQ_IMAGE"

echo "==> Starting MQTTX Web..."
docker run -d --name "$WEB_CONT" \
  -p 8080:80 \
  "$WEB_IMAGE"

echo ""
echo "Mosquitto running on:"
echo "   - MQTT:      localhost:1883"
echo "   - WebSocket: localhost:9001"
echo ""
echo "MQTTX Web UI running at:"
echo "   - http://localhost:8080"
echo ""
echo "Login credentials:"
echo "   Username: $USER"
echo "   Password: (the one you entered)"
echo ""
echo "To stop both containers: docker rm -f $MOSQ_CONT $WEB_CONT"
