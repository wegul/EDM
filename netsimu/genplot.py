import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path


def plot_grouped_bars(csv_file: str):
    # Read CSV file
    df = pd.read_csv(csv_file)

    # Plot settings
    apps = df.iloc[:, 0]  # First column is 'app'
    schemes = df.columns[1:]  # Rest are schemes
    n_apps = len(apps)
    n_schemes = len(schemes)
    width = 0.8 / n_schemes

    # Create figure
    fig, ax = plt.subplots(figsize=(12, 6))

    # Plot bars for each scheme
    for i, scheme in enumerate(schemes):
        x = np.arange(n_apps) + (i - n_schemes/2 + 0.5) * width
        bars = ax.bar(x, df[scheme], width, label=scheme)

        # Add value labels
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2, height,
                    f'{height:.2f}',
                    ha='center', va='bottom')

    # Customize plot
    ax.set_ylabel('Normalized Slowdown')
    ax.set_xticks(np.arange(n_apps))
    ax.set_xticklabels(apps, rotation=45)
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left')

    # Add grid
    ax.grid(True, axis='y', linestyle='--', alpha=0.7)

    # Use log scale (due to large value differences)
    ax.set_yscale('log')

    # Adjust layout
    plt.tight_layout()
    plt.savefig(str(csv_file)+".png")


# Usage
directory = Path('./results')
resFiles = directory.glob(r'*result.csv')
for result in resFiles:
    plot_grouped_bars(result)
