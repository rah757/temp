#!/usr/bin/env python3
import time, csv, zmq, argparse, sys, os

def main():
    parser = argparse.ArgumentParser(description="Loop and PUB video CSV")
    parser.add_argument("--run",   default="run01", help="Subfolder under /SimData")
    parser.add_argument("--port",  type=int,   default=5566,  help="PUB port")
    parser.add_argument("--fps",   type=float, default=30.0,  help="Stream rate (fps)")
    args = parser.parse_args()

    filepath = os.path.join("/SimData", args.run, "video.csv")
    if not os.path.isfile(filepath):
        sys.stderr.write(f"[stream_video] File not found: {filepath}\n")
        sys.exit(1)

    ctx = zmq.Context()
    pub = ctx.socket(zmq.PUB)
    pub.bind(f"tcp://0.0.0.0:{args.port}")
    print(f"[stream_video] Bound to tcp://*:{args.port}, streaming {filepath}", flush=True)
    time.sleep(0.2)

    interval = 1.0 / args.fps
    while True:
        with open(filepath, newline="") as f:
            reader = csv.reader(f)
            next(reader, None)
            for row in reader:
                line = ",".join(row)
                print(f"[stream_video] → {line}", flush=True)
                pub.send_string(line)
                time.sleep(interval)

if __name__ == "__main__":
    main()