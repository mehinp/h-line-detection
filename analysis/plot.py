import argparse
from pathlib import Path
import pandas as pd 
import matplotlib.pyplot as plt

DATA_DIR = Path("data")

def latest_csv(folder: Path) -> Path | None:
    return max(folder.glob("*.csv"), key=lambda p: p.stat().st_mtime, default=None)

def main():
    parser = argparse.ArgumentParse()
    parser.add_argument("csv_path", nargs="?", type=Path)
    args = parser.parse_args()

    csv_path = args.csv_path
    if csv_path is None:
        csv_path = latest_csv(DATA_DIR)
        if csv_path is None:
            parser.error(f"No CSVs found in {DATA_DIR}")
        print(f"Using newest data: {csv_path.name}")
    elif not csv_path.exists():
        parser.error(f"File not found: {csv_path}")

    df = pd.read_csv("data/spectrum.csv", comment="#")
    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(df["frequency(Hz)"] / 1e6, df["power"], linewidth=0.8)
    ax.set_xlabel("Frequency (MHz)")
    ax.set_ylabel("Power (arbitrary units)")
    ax.set_title("Averaged spectrum")
    ax.grid(True, alpha=0.3)

    out_path = csv_path.with_suffix(".png")
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"Saved {out_path}")
    plt.show()

if __name__ == "__main__":
    main()