#!/usr/bin/env python3
"""
Fake "video bitrate samples" at 30 fps on tcp://*:5566
"""
import time, random, zmq
ctx = zmq.Context()
pub = ctx.socket(zmq.PUB)
pub.bind("tcp://*:5566")
# Use the same timing approach as haptic_gen for consistency
t0 = time.monotonic()
while True:
    now = time.monotonic() - t0  # seconds from start
    kbps = random.randint(2000, 8000)
    pub.send_string(f"{now:.5f},{kbps}")
    print(f"Video: {now:.5f},{kbps}")
    time.sleep(1/30)  # 30 fps