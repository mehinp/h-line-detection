import argparse
from pathlib import Path
import pandas as pd 
import matplotlib.pyplot as plt

DATA_DIR = Path("data")

def latest_csv(folder: Path) -> Path | None:
    return max(folder.glob("*.csv"), key=lambda p: p.stat().st_mtime, default=None)

def trim_edges(freq, power, half_width=0.8):
    center = freq.median()
    keep = (freq - center).abs() < half_width
    return freq[keep], power[keep]

def on_off_ratio(on_path, off_path):
    on = pd.read_csv(on_path, comment="#")
    off = pd.read_csv(off_path, comment="#")
    freq = on["frequency(Hz)"] / 1e6
    ratio = on["power"] / off["power"]
    freq, ratio = trim_edges(freq, ratio)
    ratio = ratio / ratio.median()
    ratio = ratio.rolling(7, center=True).median()
    return freq, ratio

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_path", nargs="?", type=Path)
    parser.add_argument("off_path", nargs="?", type=Path)
    parser.add_argument("--full", action="store_true")
    args = parser.parse_args()

    csv_path = args.csv_path
    if csv_path is None:
        csv_path = latest_csv(DATA_DIR)
        if csv_path is None:
            parser.error(f"No CSVs found in {DATA_DIR}")
        print(f"Using newest data: {csv_path.name}")
    elif not csv_path.exists():
        parser.error(f"File not found: {csv_path}")

    fig, ax = plt.subplots(figsize=(10, 5))

    if args.off_path is not None:
        freq, ratio = on_off_ratio(csv_path, args.off_path)
        ax.plot(freq, ratio, linewidth=0.8)
        ax.axhline(1.0, color="gray", linewidth=0.5)
        ax.axvline(1420.405751, color="red", linestyle="--", linewidth=0.7)
        ax.set_ylabel("On / off")
        ax.set_title(f"{csv_path.stem} / {args.off_path.stem}")
        out_path = csv_path.with_name(csv_path.stem + "_ratio.png")
    else:
        df = pd.read_csv(csv_path, comment="#")
        freq = df["frequency(Hz)"] / 1e6
        power = df["power"]
        if not args.full:
            freq, power = trim_edges(freq, power, half_width=0.8)
        ax.plot(freq, power, linewidth=0.8)
        ax.set_ylabel("Power (arbitrary units)")
        ax.set_title("Averaged spectrum")
        suffix = "" if args.full else "_trim"
        out_path = csv_path.with_name(csv_path.stem + suffix + ".png")

    ax.set_xlabel("Frequency (MHz)")
    ax.grid(True, alpha=0.3)

    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"Saved {out_path}")
    plt.show()

if __name__ == "__main__":
    main()