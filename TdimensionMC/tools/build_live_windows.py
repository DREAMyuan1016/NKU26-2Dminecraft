import argparse
import csv
from collections import defaultdict
from pathlib import Path

import pandas as pd


TIME_MIN = 1704067200000
TIME_MAX = 1900000000000
WINDOW_MS = 60000
CHUNK_SIZE = 500_000

EVENT_FILES = {
    "click": "click.csv",
    "comment": "comment.csv",
    "gift": "gift.csv",
    "like": "like.csv",
    "negative": "negative.csv",
}


def normalize_chunk(chunk: pd.DataFrame, behavior_type: str) -> pd.DataFrame:
    chunk = chunk.copy()
    chunk["behavior_type"] = behavior_type
    chunk["timestamp"] = pd.to_numeric(chunk["timestamp"], errors="coerce")
    chunk["live_id"] = pd.to_numeric(chunk["live_id"], errors="coerce")
    chunk = chunk.dropna(subset=["live_id", "timestamp"])
    chunk["timestamp"] = chunk["timestamp"].astype("int64")
    chunk["live_id"] = chunk["live_id"].astype("int64")
    chunk = chunk[(chunk["timestamp"] >= TIME_MIN) & (chunk["timestamp"] < TIME_MAX)]
    if chunk.empty:
        return chunk

    if "watch_live_time" not in chunk.columns:
        chunk["watch_live_time"] = 0
    if "gift_price" not in chunk.columns:
        chunk["gift_price"] = 0

    chunk["watch_live_time"] = pd.to_numeric(chunk["watch_live_time"], errors="coerce").fillna(0).astype("int64")
    chunk["gift_price"] = pd.to_numeric(chunk["gift_price"], errors="coerce").fillna(0).astype("int64")
    chunk["window_start"] = (chunk["timestamp"] // WINDOW_MS) * WINDOW_MS
    return chunk[[
        "user_id", "live_id", "streamer_id", "timestamp", "window_start", "behavior_type", "watch_live_time", "gift_price"
    ]]


def append_merged_csv(chunk: pd.DataFrame, output_file: Path, write_header: bool) -> None:
    chunk.sort_values(["live_id", "timestamp", "behavior_type"]).to_csv(
        output_file,
        mode="a",
        header=write_header,
        index=False,
    )


def update_aggregates(chunk: pd.DataFrame, live_ranges, window_stats) -> None:
    if chunk.empty:
        return

    range_stats = chunk.groupby("live_id").agg(first_timestamp=("timestamp", "min"), last_timestamp=("timestamp", "max"))
    for live_id, row in range_stats.iterrows():
        first_ts = int(row["first_timestamp"])
        last_ts = int(row["last_timestamp"])
        if live_id not in live_ranges:
            live_ranges[live_id] = [first_ts, last_ts]
        else:
            live_ranges[live_id][0] = min(live_ranges[live_id][0], first_ts)
            live_ranges[live_id][1] = max(live_ranges[live_id][1], last_ts)

    grouped = chunk.groupby(["live_id", "window_start", "behavior_type"], sort=False).agg(
        behavior_count=("behavior_type", "size"),
        total_watch_live_time=("watch_live_time", "sum"),
        total_gift_price=("gift_price", "sum"),
    )

    for (live_id, window_start, behavior_type), row in grouped.iterrows():
        key = (int(live_id), int(window_start))
        stats = window_stats[key]
        stats["behavior_count"] += int(row["behavior_count"])
        stats[f"{behavior_type}_count"] += int(row["behavior_count"])
        stats["total_watch_live_time"] += int(row["total_watch_live_time"])
        stats["total_gift_price"] += int(row["total_gift_price"])


def write_window_csv(output_file: Path, live_ranges, window_stats) -> int:
    fieldnames = [
        "live_id",
        "first_timestamp",
        "last_timestamp",
        "window_start",
        "window_end",
        "behavior_count",
        "click_count",
        "comment_count",
        "gift_count",
        "like_count",
        "negative_count",
        "total_watch_live_time",
        "total_gift_price",
    ]
    row_count = 0

    with output_file.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()

        for live_id in sorted(live_ranges):
            first_ts, last_ts = live_ranges[live_id]
            window_start = (first_ts // WINDOW_MS) * WINDOW_MS
            window_max = (last_ts // WINDOW_MS) * WINDOW_MS

            while window_start <= window_max:
                stats = window_stats.get((live_id, window_start), {})
                writer.writerow(
                    {
                        "live_id": live_id,
                        "first_timestamp": first_ts,
                        "last_timestamp": last_ts,
                        "window_start": window_start,
                        "window_end": window_start + WINDOW_MS,
                        "behavior_count": int(stats.get("behavior_count", 0)),
                        "click_count": int(stats.get("click_count", 0)),
                        "comment_count": int(stats.get("comment_count", 0)),
                        "gift_count": int(stats.get("gift_count", 0)),
                        "like_count": int(stats.get("like_count", 0)),
                        "negative_count": int(stats.get("negative_count", 0)),
                        "total_watch_live_time": int(stats.get("total_watch_live_time", 0)),
                        "total_gift_price": int(stats.get("total_gift_price", 0)),
                    }
                )
                row_count += 1
                window_start += WINDOW_MS

    return row_count


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default=r"D:\\")
    parser.add_argument("--output-dir", default=r"D:\\processed_live_windows")
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    merged_output = output_dir / "merged_live_events.csv"
    windows_output = output_dir / "live_windows_1min.csv"
    summary_output = output_dir / "summary.csv"
    skipped_output = output_dir / "skipped_files.csv"

    if merged_output.exists():
        merged_output.unlink()

    live_ranges = {}
    window_stats = defaultdict(lambda: defaultdict(int))
    total_event_rows = 0
    write_header = True

    for behavior_type, filename in EVENT_FILES.items():
        file_path = data_dir / filename
        for chunk in pd.read_csv(file_path, chunksize=CHUNK_SIZE):
            normalized = normalize_chunk(chunk, behavior_type)
            if normalized.empty:
                continue

            total_event_rows += len(normalized)
            append_merged_csv(normalized, merged_output, write_header)
            write_header = False
            update_aggregates(normalized, live_ranges, window_stats)

    window_rows = write_window_csv(windows_output, live_ranges, window_stats)

    pd.DataFrame(
        [
            {"file_name": "room.csv", "reason": "metadata_only_no_behavior_timestamp_column"},
            {"file_name": "streamer.csv", "reason": "profile_only_no_live_id_timestamp_behavior"},
            {"file_name": "user.csv", "reason": "profile_only_no_live_id_timestamp_behavior"},
        ]
    ).to_csv(skipped_output, index=False)

    pd.DataFrame(
        [
            {
                "event_rows": total_event_rows,
                "live_count": len(live_ranges),
                "window_rows": window_rows,
                "time_min": TIME_MIN,
                "time_max": TIME_MAX,
                "window_ms": WINDOW_MS,
            }
        ]
    ).to_csv(summary_output, index=False)


if __name__ == "__main__":
    main()
