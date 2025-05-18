#!/usr/bin/env python3
"""
Broadcasts (timestamp,x,y,z) at 100 Hz on tcp://*:5555
"""
import time, random, zmq
ctx = zmq.Context()
pub = ctx.socket(zmq.PUB)
pub.bind("tcp://*:5555")
# Use monotonic time for more reliable relative timing
t0 = time.monotonic()
while True:
    now = time.monotonic() - t0  # seconds from start
    xyz = [round(random.uniform(-1,1),3) for _ in range(3)]
    pub.send_string(f"{now:.5f},{xyz[0]},{xyz[1]},{xyz[2]}")
    print(f"Published: {now:.5f},{xyz[0]},{xyz[1]},{xyz[2]}")
    time.sleep(0.01)  # 100 Hz