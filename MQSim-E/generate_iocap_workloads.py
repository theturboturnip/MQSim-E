import argparse
import itertools
from pathlib import Path
from typing import *

import jinja2

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--queues", type=int, default=8)
    parser.add_argument("--total-ops", type=int, default=4000000)
    parser.add_argument("--template", type=str, default="iocap_workload_template.jinja")
    parser.add_argument("workload_dir")
    args = parser.parse_args()

    workload_dir = Path(args.workload_dir)
    assert workload_dir.is_dir()

    """
    Workload Structure:
    - As in Haas, total queue depth of 512 (rounded up from 375).
    - 4KiB accesses
    - Haas found 32 threads in an all-to-all configuration got best random-read performance, but this sim caps out at 8 queues, so 8 flows and 8 iocap trackers it is - 512/8 = 64
    - l_n=1..64 (this is done automatically)
    - vary r/w mix from 0..50..100
    - Samsung PM9A3 1.92TB model was used as a base - PCIe Gen4x4
    """
    params = {
        "len_queue_ids": [args.queues],
        "queue_ids": [list(range(args.queues))],
        "queue_depth": [512 // args.queues],
        "initial_occupancy": [0, 50, 70, 100],
        "working_set": [100],
        "read_percentage": [0, 50, 100],
        "total_requests": [args.total_ops],
    }
    changing_params = ["len_queue_ids", "read_percentage", "initial_occupancy"]

    env = jinja2.Environment(loader=jinja2.FileSystemLoader("."))
    template = env.get_template(args.template)

    for param_list in itertools.product(*params.values()):
        param_dict = {k: v for k, v in zip(params.keys(), param_list)}
        name = (
            "synth_"
            + "_".join([f"{key}_{param_dict[key]}" for key in changing_params])
            + ".xml"
        )
        with open(workload_dir / name, "w", encoding="utf-8") as f:
            f.write(template.render(**param_dict))
