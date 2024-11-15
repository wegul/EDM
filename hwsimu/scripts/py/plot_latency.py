import pandas as pd
import matplotlib.pyplot as plt

# Read the data
data = pd.read_csv("../result/latency.csv")
error = pd.read_csv("../result/latency_err.csv")

# Create bar plot
plt.figure(figsize=(10, 6))

# Set width and positions for bars
bar_width = 0.25
ratios = data.iloc[:, 0]
x = range(len(ratios))

# Create bars
bars1 = plt.bar(x, data['edm'],  bar_width,
                label='EDM', color='blue', yerr=error['edm'])
bars2 = plt.bar([i+bar_width for i in x], data['numa'],
                bar_width, label='NUMA', color='green', yerr=error['numa'])
bars3 = plt.bar([i+2*bar_width for i in x], data['rdma'],
                bar_width, label='RDMA', color='red', yerr=error['rdma'])


def autolabel(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                 f'{height:.1f}',
                 ha='center', va='bottom')


autolabel(bars1)
autolabel(bars2)
autolabel(bars3)

# Customize plot
plt.xlabel('Local : Remote')
plt.ylabel('Latency (ns)')
plt.xticks([i+bar_width for i in x], ratios)
plt.legend()

plt.tight_layout()
# plt.show()
plt.savefig('../result/latency.png', dpi=300, bbox_inches='tight')
