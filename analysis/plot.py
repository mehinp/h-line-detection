import pandas as pd 
import matplotlib.pyplot as plt

def main():
    df = pd.read_csv("data/spectrum.csv")
    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(df["frequency(Hz)"] / 1e6, df["power"], linewidth=0.8)
    ax.set_xlabel("Frequency (MHz)")
    ax.set_ylabel("Power (arbitrary units)")
    ax.set_title("Averaged spectrum")
    ax.grid(True, alpha=0.3)

    fig.savefig("data/spectrum.png")
    plt.show()

if __name__ == "__main__":
    main()