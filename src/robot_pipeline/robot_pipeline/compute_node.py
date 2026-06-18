#!/usr/bin/env python3
from typing import Optional

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from sensor_msgs.msg import Image
from std_msgs.msg import Int16MultiArray


class ComputeNode(Node):
    def __init__(self):
        super().__init__('compute_node')

        self._latest_color: Optional[Image] = None
        self._latest_depth: Optional[Image] = None
        self._latest_audio: Optional[Int16MultiArray] = None

        # match camera_node's best-effort QoS
        img_qos = QoSProfile(depth=1,
                             reliability=ReliabilityPolicy.BEST_EFFORT,
                             history=HistoryPolicy.KEEP_LAST)
        self.create_subscription(Image, '/camera/color', self._on_color, img_qos)
        self.create_subscription(Image, '/camera/depth', self._on_depth, img_qos)
        self.create_subscription(Int16MultiArray, '/audio', self._on_audio, 10)

        self.get_logger().info('Compute node started')

    # --- callbacks: just store the latest frame ---

    def _on_color(self, msg: Image) -> None:
        self._latest_color = msg

    def _on_depth(self, msg: Image) -> None:
        self._latest_depth = msg

    def _on_audio(self, msg: Int16MultiArray) -> None:
        self._latest_audio = msg

    # --- accessors (call from a timer / future processing logic) ---

    @property
    def color_frame(self) -> Optional[Image]:
        return self._latest_color

    @property
    def depth_frame(self) -> Optional[Image]:
        return self._latest_depth

    @property
    def audio_frame(self) -> Optional[Int16MultiArray]:
        return self._latest_audio


def main(args=None):
    rclpy.init(args=args)
    node = ComputeNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
