#!/usr/bin/env python3
import zmq
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()

    ctx = zmq.Context()
    sub = ctx.socket(zmq.SUB)
    sub.connect(f"tcp://127.0.0.1:{args.port}")
    sub.setsockopt_string(zmq.SUBSCRIBE, "")
    print(f"[debug_sub] Connected to tcp://127.0.0.1:{args.port}")

    try:
        while True:
            msg = sub.recv_string()
            print(f"[debug_sub] → {msg}")
    except KeyboardInterrupt:
        print("\n[debug_sub] Stopped.")

if __name__ == "__main__":
    main()