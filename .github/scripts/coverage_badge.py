#!/usr/bin/env python3
"""Turn a gcovr --json-summary into a shields.io endpoint badge JSON.

    coverage_badge.py coverage-summary.json coverage.json

shields.io renders https://img.shields.io/endpoint?url=<raw URL of coverage.json>.
"""
import json
import sys


def main():
    summary_path, badge_path = sys.argv[1], sys.argv[2]
    with open(summary_path) as f:
        percent = json.load(f)["line_percent"]
    color = "brightgreen" if percent >= 90 else "green" if percent >= 75 else "yellow" if percent >= 60 else "orange" if percent >= 40 else "red"
    badge = {"schemaVersion": 1, "label": "coverage", "message": f"{percent:.0f}%", "color": color}
    with open(badge_path, "w") as f:
        json.dump(badge, f)
    print(f"line coverage {percent:.1f}% -> {badge}")


if __name__ == "__main__":
    main()
