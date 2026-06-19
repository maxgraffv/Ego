import json
import os

from launch import LaunchDescription
from launch_ros.actions import Node


def _load_network_config() -> dict:
    # Priorytet 1: zmienna środowiskowa EGO_NETWORK_CONFIG wskazuje bezpośrednio na plik
    path = os.environ.get('EGO_NETWORK_CONFIG')
    if path and os.path.exists(path):
        with open(path) as f:
            return json.load(f)

    # Priorytet 2: config/network.json w katalogu workspace (wyżej od install/)
    prefix = os.environ.get('COLCON_PREFIX_PATH', '')
    if prefix:
        ws_root = os.path.dirname(prefix.split(':')[0])
        candidate = os.path.join(ws_root, 'config', 'network.json')
        if os.path.exists(candidate):
            with open(candidate) as f:
                return json.load(f)

    raise FileNotFoundError(
        "Brak pliku konfiguracji sieci.\n"
        "  Skopiuj:  config/network.example.json  →  config/network.json\n"
        "  Uzupełnij pole 'laptop_ips' listą adresów IP odbiorców.\n"
        "  Lub ustaw: export EGO_NETWORK_CONFIG=/sciezka/do/network.json"
    )


def generate_launch_description():
    net = _load_network_config()

    return LaunchDescription([
        Node(
            package='robot_pipeline',
            node_executable='camera_node',
            node_name='camera',
            output='screen',
            parameters=[{
                'jpeg_quality': 60,
            }],
        ),
        Node(
            package='robot_pipeline',
            node_executable='microphone_node',
            node_name='microphone',
            output='screen',
            parameters=[{
                'device': 'respeaker',
            }],
        ),
        Node(
            package='robot_pipeline',
            node_executable='compute_node.py',
            node_name='compute',
            output='screen',
        ),
        Node(
            package='robot_pipeline',
            node_executable='comms_node.py',
            node_name='comms',
            output='screen',
            parameters=[{
                'hosts':         net['laptop_ips'],
                'port':          net.get('laptop_port', 5005),
                'depth_every_n': net.get('depth_every_n', 3),
            }],
        ),
    ])
