import argparse
import datetime
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Generator, List, Tuple, TypedDict

import tomli_w


@dataclass
class FlowInfo:
    total_ops: int
    iops: float


@dataclass
class IOCapInfo:
    n_ops_per_lease: int
    sum_time_exposed: int
    max_time_exposed: int
    sum_extra_time_exposed: int
    max_extra_time_exposed: int
    total_lease_ids_required_for_total_availability: int
    total_lease_openings: int
    sum_lease_occupancy_at_close: int


def flow_info(node) -> FlowInfo:
    iops = float(node.find("IOPS").text)
    total_ops = int(node.find("Request_Count").text)
    return FlowInfo(total_ops=total_ops, iops=iops)


def iocap_infos(context) -> Generator[IOCapInfo, None, None]:
    for node in context.iterfind("IOCap.IOCapTrackerContext.IOCapTracker"):
        yield IOCapInfo(
            n_ops_per_lease=int(node.find("N_Ops_Per_Lease").text),
            sum_time_exposed=int(node.find("Sum_Time_Exposed").text),
            max_time_exposed=int(node.find("Max_Time_Exposed").text),
            sum_extra_time_exposed=int(node.find("Sum_Extra_Time_Exposed").text),
            max_extra_time_exposed=int(node.find("Max_Extra_Time_Exposed").text),
            total_lease_ids_required_for_total_availability=int(
                node.find("Total_Lease_Ids_Required_For_Total_Availability").text
            ),
            total_lease_openings=int(node.find("Total_Lease_Openings").text),
            sum_lease_occupancy_at_close=int(
                node.find("Sum_Lease_Occupancy_At_Close").text
            ),
        )


class TomlableTestInfo(TypedDict):
    n_ops: int
    n_ops_per_lease: int
    n_used_lease_ids: int
    iops: float

    avg_l_ns: float
    max_l_ns: int
    avg_extra_l_ns: float
    max_extra_l_ns: int
    delta_max_l_ns: int

    equiv_in_order_n: float


def total_datapoints(
    flow: FlowInfo,
    iocap_scenarios: List[IOCapInfo],
) -> List[TomlableTestInfo]:
    iocap_1per = next(
        t  #
        for t in iocap_scenarios
        if t.n_ops_per_lease == 1
    )
    iocap_others = [
        t  #
        for t in iocap_scenarios
        if t != iocap_1per
    ]

    def parse_test(
        iocaps: IOCapInfo, info_1key: TomlableTestInfo | None
    ) -> TomlableTestInfo:
        avg_l_ns = iocaps.sum_time_exposed / flow.total_ops
        avg_extra_l_ns = iocaps.sum_extra_time_exposed / flow.total_ops
        if info_1key is not None:
            delta_max_l_ns = iocaps.max_time_exposed - info_1key["max_l_ns"]
        else:
            delta_max_l_ns = 0
        equiv_in_order_n = (avg_extra_l_ns * 2 * flow.iops * 1e-9) + 1

        return {
            "n_ops": flow.total_ops,
            "n_ops_per_lease": iocaps.n_ops_per_lease,
            "n_used_lease_ids": iocaps.total_lease_ids_required_for_total_availability,
            "iops": flow.iops,
            "avg_l_ns": avg_l_ns,
            "max_l_ns": iocaps.max_time_exposed,
            "avg_extra_l_ns": avg_extra_l_ns,
            "max_extra_l_ns": iocaps.max_extra_time_exposed,
            "delta_max_l_ns": delta_max_l_ns,
            "equiv_in_order_n": equiv_in_order_n,
        }

    info_1key = parse_test(iocap_1per, None)
    infos = [
        parse_test(i, info_1key)  #
        for i in iocap_others
    ]
    return [info_1key] + infos


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--tsv")
    parser.add_argument("--toml")
    parser.add_argument("xmls_in", nargs="+")

    args = parser.parse_args()

    tests = []
    for xml_path in args.xmls_in:
        try:
            xml = ET.parse(xml_path)
            host_tag = xml.getroot().find("Host")
            assert host_tag is not None
            flows = [flow_info(node) for node in host_tag.iterfind("Host.IO_Flow")]
            iocaps = [
                list(iocap_infos(node))
                for node in host_tag.iterfind("IOCap.IOCapTrackerContext")
            ]
            assert len(flows) == len(iocaps)
            tests.append((Path(xml_path).name, list(zip(flows, iocaps))))
        except Exception as e:
            raise Exception(f"Error in {xml_path}") from e

    all_infos = {}
    for name, flow_to_iocaps in tests:
        infos = [
            sorted(total_datapoints(flow, iocaps), key=lambda t: t["n_ops_per_lease"])
            for flow, iocaps in flow_to_iocaps
        ]
        all_infos[name] = infos

    outputs = {k: all_infos[k] for k in sorted(all_infos.keys())}
    if args.toml:
        toml_out = {
            "timestamp": datetime.datetime.now().isoformat(),
            "generated_by": "MQSim-E-sws35",
            "git_hash": "TODO",
            "tests": outputs,
        }
        with open(args.toml, "wb") as f:
            tomli_w.dump(toml_out, f)
