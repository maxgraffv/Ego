#!/usr/bin/env python3
"""
Laptop-side UDP receiver — reconstructs RGBD + audio streamed from Jetson.

Usage:
    python3 laptop_receiver.py [--port 5005] [--show]

Dependencies:
    pip install opencv-python numpy
"""
import argparse
import struct
import socket
import zlib
from collections import defaultdict

import cv2
import numpy as np

# --- protocol (must match comms_node.py) ---
MAGIC        = 0x524F5343
STREAM_COLOR = 0
STREAM_DEPTH = 1
STREAM_AUDIO = 2

HEADER_FMT  = '!IBIHHH'
HEADER_SIZE = struct.calcsize(HEADER_FMT)
RECV_BUF    = 65536

# fragment buffer: (stream_id, frame_id) → {frag_id: bytes}
_frags: dict = defaultdict(dict)


def reassemble(stream_id: int, frame_id: int,
               frag_id: int, frag_total: int,
               payload: bytes):
    """Returns complete payload when all fragments arrived, else None."""
    key = (stream_id, frame_id)
    _frags[key][frag_id] = payload

    if len(_frags[key]) == frag_total:
        full = b''.join(_frags[key][i] for i in range(frag_total))
        del _frags[key]
        return full
    return None


def decode_color(data: bytes):
    arr = np.frombuffer(data, dtype=np.uint8)
    return cv2.imdecode(arr, cv2.IMREAD_COLOR)


def decode_depth(data: bytes):
    h, w = struct.unpack('!HH', data[:4])
    raw  = zlib.decompress(data[4:])
    return np.frombuffer(raw, dtype=np.uint16).reshape(h, w)


def depth_colormap(depth: np.ndarray) -> np.ndarray:
    """Normalize and apply colormap for display."""
    valid = depth[depth > 0]
    if valid.size == 0:
        return np.zeros((*depth.shape, 3), dtype=np.uint8)
    norm = np.clip((depth.astype(np.float32) - valid.min()) /
                   (valid.max() - valid.min() + 1e-6), 0, 1)
    grey = (norm * 255).astype(np.uint8)
    return cv2.applyColorMap(grey, cv2.COLORMAP_TURBO)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', type=int, default=5005)
    parser.add_argument('--show', action='store_true',
                        help='Display color and depth windows')
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', args.port))
    sock.settimeout(1.0)
    print(f'Listening on :{args.port}  (Ctrl+C to stop)')

    fps_counter = [0, 0, 0]

    while True:
        try:
            raw, addr = sock.recvfrom(RECV_BUF)
        except socket.timeout:
            continue
        except KeyboardInterrupt:
            break

        if len(raw) < HEADER_SIZE:
            continue

        magic, stream_id, frame_id, frag_id, frag_total, payload_len = \
            struct.unpack(HEADER_FMT, raw[:HEADER_SIZE])

        if magic != MAGIC:
            continue

        payload = raw[HEADER_SIZE:HEADER_SIZE + payload_len]
        full    = reassemble(stream_id, frame_id, frag_id, frag_total, payload)

        if full is None:
            continue

        fps_counter[stream_id] += 1

        if stream_id == STREAM_COLOR:
            frame = decode_color(full)
            if frame is not None and args.show:
                cv2.imshow('color', frame)

        elif stream_id == STREAM_DEPTH:
            depth = decode_depth(full)
            if args.show:
                cv2.imshow('depth', depth_colormap(depth))

        elif stream_id == STREAM_AUDIO:
            samples = np.frombuffer(full, dtype=np.int16)
            # samples.reshape(-1, 6) → (frames, channels)
            # TODO: feed to speech recognition / RL model

        if args.show:
            if cv2.waitKey(1) == ord('q'):
                break

    sock.close()
    cv2.destroyAllWindows()
    print('Frames received — color:', fps_counter[0],
          ' depth:', fps_counter[1],
          ' audio:', fps_counter[2])


if __name__ == '__main__':
    main()
