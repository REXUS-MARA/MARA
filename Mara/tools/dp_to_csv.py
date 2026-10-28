#!/usr/bin/env python3
"""Turn MARA data product files (.fdp) into one CSV per record type.

Each .fdp file is decoded with F Prime's `fprime-dp-write` (run it with the fprime venv active),
records are matched to their names in the dictionary, and rows of the same record type from all
files are merged into one CSV, sorted by time. Nested struct fields become columns such as
`data.pressure`.

BMP280 readings get two extra columns, `recomputed_pressure` and `recomputed_temperature`: the
datasheet compensation applied to the raw ADC values with the calibration stored in the same file,
without Bosch's clamp to 300..1100 hPa. Use them when `status` is 3 (pressure below 300 hPa, above
~9 km); within the sensor range they match the stored values.

Usage:
    python3 Mara/tools/dp_to_csv.py <dictionary.json> <output dir> <.fdp file or directory>...

See DATA_PRODUCTS.md for where the files come from.
"""

import argparse
import csv
import json
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

BMP_READING = ".bmp280Manager.ReadingRecord"
BMP_CALIB = ".bmp280Manager.CalibRecord"


def flatten(value, prefix=""):
    """fprime-dp-write writes structs as lists of one-key dicts; flatten them to {"a.b": value}."""
    if isinstance(value, list) and all(isinstance(item, dict) and len(item) == 1 for item in value):
        flat = {}
        for item in value:
            (key, inner), = item.items()
            flat.update(flatten(inner, f"{prefix}{key}."))
        return flat
    return {prefix.rstrip("."): value}


def decode(fdp: Path, dictionary: Path, workdir: Path) -> list:
    """Run fprime-dp-write (it writes <name>.json into its working directory) and load the result."""
    result = subprocess.run(
        ["fprime-dp-write", str(fdp.resolve()), str(dictionary.resolve())],
        cwd=workdir, capture_output=True, text=True,
    )
    output = workdir / (fdp.stem + ".json")
    if result.returncode != 0 or not output.exists():
        raise RuntimeError(f"fprime-dp-write failed on {fdp}:\n{result.stdout}{result.stderr}")
    return json.loads(output.read_text())


def bmp280_compensate(calib: dict, adc_t: int, adc_p: int):
    """BMP280 datasheet double-precision compensation, without the range clamp. Returns (°C, Pa)."""
    v1 = (adc_t / 16384.0 - calib["t1"] / 1024.0) * calib["t2"]
    v2 = (adc_t / 131072.0 - calib["t1"] / 8192.0) ** 2 * calib["t3"]
    t_fine = v1 + v2
    v1 = t_fine / 2.0 - 64000.0
    v2 = v1 * v1 * calib["p6"] / 32768.0 + v1 * calib["p5"] * 2.0
    v2 = v2 / 4.0 + calib["p4"] * 65536.0
    v1 = (calib["p3"] * v1 * v1 / 524288.0 + calib["p2"] * v1) / 524288.0
    v1 = (1.0 + v1 / 32768.0) * calib["p1"]
    if v1 == 0:
        return t_fine / 5120.0, float("nan")
    p = (1048576.0 - adc_p - v2 / 4096.0) * 6250.0 / v1
    p += (calib["p9"] * p * p / 2147483648.0 + p * calib["p8"] / 32768.0 + calib["p7"]) / 16.0
    return t_fine / 5120.0, p


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dictionary", type=Path, help="MaraRPiUARTTopologyDictionary.json of the flown build")
    parser.add_argument("output", type=Path, help="directory for the CSV files")
    parser.add_argument("inputs", type=Path, nargs="+", help=".fdp files or directories containing them")
    args = parser.parse_args()

    record_names = {r["id"]: r["name"] for r in json.loads(args.dictionary.read_text())["records"]}
    files = sorted({f for p in args.inputs for f in ([p] if p.is_file() else p.rglob("*.fdp"))})
    if not files:
        print("no .fdp files found", file=sys.stderr)
        return 1

    rows = defaultdict(list)
    with tempfile.TemporaryDirectory() as tmp:
        for fdp in files:
            header, *records = decode(fdp, args.dictionary, Path(tmp))
            container_time = header["Seconds"] + header["USeconds"] / 1e6
            calib = None
            for record in records:
                name = record_names.get(record["dataId"], f"unknown_{record['dataId']}")
                row = {"file": fdp.name, "container_time": container_time, **flatten(record["data"])}
                if name.endswith(BMP_CALIB):
                    calib = row
                elif name.endswith(BMP_READING) and calib is not None:
                    temperature, pressure = bmp280_compensate(calib, row["rawTemperature"], row["rawPressure"])
                    row["recomputed_temperature"] = temperature
                    row["recomputed_pressure"] = pressure
                rows[name].append(row)

    args.output.mkdir(parents=True, exist_ok=True)
    for name, name_rows in rows.items():
        name_rows.sort(key=lambda r: (r.get("time_stamp.seconds", r["container_time"]),
                                      r.get("time_stamp.useconds", 0)))
        columns = list(dict.fromkeys(key for row in name_rows for key in row))
        path = args.output / f"{name}.csv"
        with path.open("w", newline="") as out:
            writer = csv.DictWriter(out, fieldnames=columns)
            writer.writeheader()
            writer.writerows(name_rows)
        print(f"{path}: {len(name_rows)} rows")
    return 0


if __name__ == "__main__":
    sys.exit(main())
