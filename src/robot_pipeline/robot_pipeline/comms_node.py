#!/usr/bin/env python3
import struct
import socket
import threading
import zlib

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from sensor_msgs.msg import Image, CompressedImage
from std_msgs.msg import Int16MultiArray

# --- protocol constants ---
MAGIC        = 0x524F5343  # 'ROSC'
STREAM_COLOR = 0
STREAM_DEPTH = 1
STREAM_AUDIO = 2

# header layout: magic(4) stream(1) frame_id(4) frag_id(2) frag_total(2) payload_len(2) = 15 bytes
HEADER_FMT  = '!IBIHHH'
HEADER_SIZE = struct.calcsize(HEADER_FMT)
MAX_PAYLOAD = 60_000


class CommsNode(Node):
    def __init__(self):
        super().__init__('comms_node')

        self.declare_parameter('host', '127.0.0.1')
        self.declare_parameter('port', 5005)
        self.declare_parameter('depth_every_n', 3)

        host                = self.get_parameter('host').value
        port                = self.get_parameter('port').value
        self._depth_every_n = self.get_parameter('depth_every_n').value

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4 * 1024 * 1024)
        self._addr = (host, port)

        self._fid       = [0, 0, 0]
        self._send_lock = threading.Lock()

        # latest-frame slots: callback writes, worker reads — always newest frame, never queued
        self._color_slot  = None
        self._color_lock  = threading.Lock()
        self._color_event = threading.Event()

        self._depth_slot    = None
        self._depth_lock    = threading.Lock()
        self._depth_event   = threading.Event()
        self._depth_counter = 0

        self._running = True
        threading.Thread(target=self._color_worker, daemon=True, name='color_enc').start()
        threading.Thread(target=self._depth_worker, daemon=True, name='depth_enc').start()

        img_qos = QoSProfile(depth=1,
                             reliability=ReliabilityPolicy.BEST_EFFORT,
                             history=HistoryPolicy.KEEP_LAST)
        # JPEG already encoded in C++ camera_node — no Python encoding needed
        self.create_subscription(CompressedImage, '/camera/color/compressed', self._on_color, img_qos)
        self.create_subscription(Image,           '/camera/depth',            self._on_depth, img_qos)
        self.create_subscription(Int16MultiArray, '/audio',                   self._on_audio, 10)

        self.get_logger().info(f'CommsNode streaming → {host}:{port}')

    def destroy_node(self):
        self._running = False
        self._color_event.set()
        self._depth_event.set()
        super().destroy_node()

    # --- send: memoryview slices + sendmsg scatter-gather (zero extra copies) ---

    def _send(self, stream_id: int, payload) -> None:
        mv    = memoryview(payload).cast('B')  # byte-granular slicing for any buffer type
        n     = len(mv)
        total = max((n + MAX_PAYLOAD - 1) // MAX_PAYLOAD, 1)

        with self._send_lock:
            fid = self._fid[stream_id]
            self._fid[stream_id] = (fid + 1) & 0xFFFFFFFF

        for idx in range(total):
            chunk  = mv[idx * MAX_PAYLOAD : (idx + 1) * MAX_PAYLOAD]
            header = struct.pack(HEADER_FMT, MAGIC, stream_id, fid, idx, total, len(chunk))
            self._sock.sendmsg([header, chunk], [], 0, self._addr)

    # --- ROS callbacks: just swap the slot and signal — never block the executor ---

    def _on_color(self, msg: CompressedImage) -> None:
        with self._color_lock:
            self._color_slot = msg
        self._color_event.set()

    def _on_depth(self, msg: Image) -> None:
        self._depth_counter += 1
        if self._depth_counter % self._depth_every_n != 0:
            return
        with self._depth_lock:
            self._depth_slot = msg
        self._depth_event.set()

    def _on_audio(self, msg: Int16MultiArray) -> None:
        # msg.data is already a little-endian int16 array.array on this platform — send raw
        self._send(STREAM_AUDIO, msg.data)

    # --- encoding workers: cv2/zlib release GIL → real parallelism on ARM cores ---

    def _color_worker(self):
        while self._running:
            self._color_event.wait()
            self._color_event.clear()
            with self._color_lock:
                msg = self._color_slot
            if msg is None:
                continue
            # JPEG already encoded in C++ — just forward the bytes
            self._send(STREAM_COLOR, msg.data)

    def _depth_worker(self):
        while self._running:
            self._depth_event.wait()
            self._depth_event.clear()
            with self._depth_lock:
                msg = self._depth_slot
            if msg is None:
                continue
            # depth is raw little-endian uint16 bytes already — compress the ROS buffer directly
            meta       = struct.pack('!HH', msg.height, msg.width)
            compressed = zlib.compress(msg.data, level=1)
            self._send(STREAM_DEPTH, meta + compressed)


def main(args=None):
    rclpy.init(args=args)
    node = CommsNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
