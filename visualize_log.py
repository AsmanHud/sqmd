import sys
from io import StringIO

import pandas as pd
import matplotlib.pyplot as plt


def extract_csv_from_dump(text: str) -> str:
    start_marker = "===== LOG DUMP START ====="
    end_marker = "===== LOG DUMP END ====="

    start = text.find(start_marker)
    end = text.find(end_marker)

    if start == -1:
        raise ValueError("Start marker not found.")
    if end == -1:
        raise ValueError("End marker not found.")

    csv_text = text[start + len(start_marker):end].strip()

    if not csv_text:
        raise ValueError("No CSV data found between dump markers.")

    return csv_text


def main():
    if len(sys.argv) != 2:
        print("Usage: python visualize_log.py dump.txt")
        return

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        raw_text = f.read()

    csv_text = extract_csv_from_dump(raw_text)
    df = pd.read_csv(StringIO(csv_text))

    if df.empty:
        print("No records found.")
        return

    df["time_s"] = df["timestamp_ms"] / 1000.0

    th_df = df[df["type"] == "TH"].copy()
    accel_df = df[df["type"] == "ACCEL_PEAK"].copy()

    print("\nTemperature / humidity records:")
    if th_df.empty:
        print("None")
    else:
        print(th_df[["timestamp_ms", "time_s", "value1", "value2"]].rename(columns={
            "value1": "temperature_C",
            "value2": "humidity_percent"
        }).to_string(index=False))

    print("\nShock event records:")
    if accel_df.empty:
        print("None")
    else:
        print(accel_df[["timestamp_ms", "time_s", "value1", "value2"]].rename(columns={
            "value1": "peak_delta_g",
            "value2": "sample_count"
        }).to_string(index=False))

    if not th_df.empty:
        plt.figure()

        plt.plot(
            th_df["time_s"],
            th_df["value1"],
            marker="o",
            label="Temperature [°C]"
        )

        plt.plot(
            th_df["time_s"],
            th_df["value2"],
            marker="o",
            label="Relative humidity [%]"
        )

        for _, row in accel_df.iterrows():
            plt.axvline(
                x=row["time_s"],
                linestyle="--",
                linewidth=1,
                label="Shock event" if row.name == accel_df.index[0] else None
            )

        plt.xlabel("Time since boot [s]")
        plt.ylabel("Measured value")
        plt.title("Temperature / Humidity Log with Shock Event Markers")
        plt.grid(True)
        plt.legend()
        plt.tight_layout()
        plt.show()

    if not accel_df.empty:
        plt.figure()
        plt.bar(accel_df["time_s"], accel_df["value1"], width=0.5)

        plt.xlabel("Time since boot [s]")
        plt.ylabel("Peak delta magnitude [g]")
        plt.title("Logged Shock Events")
        plt.grid(True)
        plt.tight_layout()
        plt.show()


if __name__ == "__main__":
    main()