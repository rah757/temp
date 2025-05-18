#!/usr/bin/env python3
import zmq, argparse, time, sys

def main():
    parser = argparse.ArgumentParser(description="SUB tcp://vm1:5555 → PUB tcp://0.0.0.0:5556")
    parser.add_argument("--in-host", default="vm1",         help="Hostname of VM1")
    parser.add_argument("--in-port", type=int, default=5555, help="VM1 PUB port")
    parser.add_argument("--out-port",type=int, default=5556, help="Filtered PUB port")
    args = parser.parse_args()

    ctx = zmq.Context()

    # subscribe to the raw haptic stream coming from vm1:5555
    sub = ctx.socket(zmq.SUB)
    sub.connect(f"tcp://{args.in_host}:{args.in_port}")
    sub.setsockopt_string(zmq.SUBSCRIBE, "")
    print(f"[filter_haptic] SUB → tcp://{args.in_host}:{args.in_port}", flush=True)

    # re-publish on all interfaces so host:5556 sees it
    pub = ctx.socket(zmq.PUB)
    pub.bind(f"tcp://0.0.0.0:{args.out_port}")
    print(f"[filter_haptic] PUB → tcp://0.0.0.0:{args.out_port}", flush=True)

    # tiny pause so subscribers have time to connect
    time.sleep(0.2)

    while True:
        msg = sub.recv_string()
        print(f"[filter_haptic] ← {msg}", flush=True)
        # baseline: forward every sample
        pub.send_string(msg)

if __name__ == "__main__":
    main()