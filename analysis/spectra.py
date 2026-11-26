from pathlib import Path
import pandas as pd

LIGHT_SPEED = 299792.458 # in km/s
F_ZERO = 1420405751.768 # in Hz
DATA_DIR = Path("data")


def latest_csv(folder: Path) -> Path | None:
    return max(folder.glob("*.csv"), key=lambda p: p.stat().st_mtime, default=None)

def trim_edges(freq, power, half_width=0.8e6):
    center = freq.median()
    keep = (freq - center).abs() < half_width
    return freq[keep], power[keep]

def on_off_ratio(on_path, off_path, trim: bool):
    on = pd.read_csv(on_path, comment="#")
    off = pd.read_csv(off_path, comment="#")
    freq = on["frequency(Hz)"]
    ratio = on["power"] / off["power"]
    if trim:
        freq, ratio = trim_edges(freq, ratio)
    ratio = ratio / ratio.median()
    ratio = ratio.rolling(7, center=True).median()
    return ratio

def freq_to_velocity(freq_series: pd.Series):
    velocity = LIGHT_SPEED * (F_ZERO - freq_series) / F_ZERO
    return velocity