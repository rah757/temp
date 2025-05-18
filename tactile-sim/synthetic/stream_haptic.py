#!/usr/bin/env python3
import time, csv, zmq, argparse, sys, os

def main():
    parser = argparse.ArgumentParser(description="Loop and PUB tactile CSV")
    parser.add_argument("--run",   default="run01", help="Subfolder under /SimData")
    parser.add_argument("--port",  type=int,   default=5555,   help="PUB port")
    parser.add_argument("--hz",    type=float, default=100.0,  help="Stream rate (Hz)")
    args = parser.parse_args()

    filepath = os.path.join("/SimData", args.run, "tactile.csv")
    if not os.path.isfile(filepath):
        sys.stderr.write(f"[stream_haptic] File not found: {filepath}\n")
        sys.exit(1)

    ctx = zmq.Context()
    pub = ctx.socket(zmq.PUB)
    pub.bind(f"tcp://0.0.0.0:{args.port}")
    print(f"[stream_haptic] Bound to tcp://*:{args.port}, streaming {filepath}", flush=True)
    time.sleep(0.2)  # allow subscribers to connect

    interval = 1.0 / args.hz
    while True:
        with open(filepath, newline="") as f:
            reader = csv.reader(f)
            next(reader, None)  # skip header
            for row in reader:
                line = ",".join(row)
                print(f"[stream_haptic] → {line}", flush=True)
                pub.send_string(line)
                time.sleep(interval)

if __name__ == "__main__":
    main()