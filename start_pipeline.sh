#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ROS_SETUP="/opt/ros/${ROS_DISTRO:-eloquent}/setup.bash"
if [ ! -f "$ROS_SETUP" ]; then
    echo "[robot-pipeline] ERROR: ROS setup nie znaleziony: $ROS_SETUP"
    echo "  Ustaw: export ROS_DISTRO=<twoja_dystrybucja>  (np. eloquent, foxy, humble)"
    exit 1
fi
source "$ROS_SETUP"

if [ ! -f "$SCRIPT_DIR/config/network.json" ]; then
    echo "[robot-pipeline] ERROR: Brak pliku config/network.json"
    echo "  Skopiuj: cp config/network.example.json config/network.json"
    echo "  Następnie uzupełnij 'laptop_ip' adresem IP swojego laptopa."
    exit 1
fi

cd "$SCRIPT_DIR"
echo "[robot-pipeline] Building..."
colcon build --packages-select robot_pipeline

echo "[robot-pipeline] Launching..."
source "$SCRIPT_DIR/install/setup.bash"
exec ros2 launch robot_pipeline system.launch.py
