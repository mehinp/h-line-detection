import argparse
import pandas as pd 
import matplotlib.pyplot as plt
from pathlib import Path
from spectra import LIGHT_SPEED, F_ZERO, DATA_DIR, latest_csv, trim_edges, on_off_ratio, freq_to_velocity

def get_csv_path(args) -> Path:
    csv_path = args.csv_path
    if csv_path is None:
        csv_path = latest_csv(DATA_DIR)
        if csv_path is None:
            raise RuntimeError(f"No CSVs found in {DATA_DIR}")
        print(f"Using newest data: {csv_path.name}")
    elif not csv_path.exists():
        raise RuntimeError(f"File not found: {csv_path}")
    return csv_path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv_path", nargs="?", type=Path)
    parser.add_argument("--off_path", nargs="?", type=Path)
    parser.add_argument("--trim", action="store_true")
    parser.add_argument("--velocity", action="store_true")

    args = parser.parse_args()
    fig, ax = plt.subplots(figsize=(10, 5))
    csv_path = get_csv_path(args)

    df = pd.read_csv(csv_path, comment="#")
    freq = df["frequency(Hz)"]
    power = df["power"] 
    if args.trim:
        freq, power = trim_edges(freq, power, half_width=0.8e6)

    x_value, x_label = (freq / 1e6, "Frequency (MHz)") if not args.velocity else (freq_to_velocity(freq), "Velocity (km/s)")
    y_value, y_label = (power, "Power (arbitray units)") if args.off_path is None else (on_off_ratio(csv_path, args.off_path, args.trim), "On / off")
    ax.axvline(0 if args.velocity else F_ZERO / 1e6, color="red", linestyle="--", linewidth=0.7)
    ax.plot(x_value, y_value)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)

    suffix = ("_velocity" if args.velocity else "") + ("_trim" if args.trim else "") + ("_ratio" if args.off_path else "")
    if args.off_path is None:
        ax.set_title("Averaged spectrum")
    else:
        ax.set_title(f"{csv_path.stem} / {args.off_path.stem}")

    out_path = csv_path.with_name(csv_path.stem + suffix + ".png")
    ax.grid(True, alpha=0.4)
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"Saved {out_path}")
    plt.show()

if __name__ == "__main__":
    main()