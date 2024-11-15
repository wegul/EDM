import pandas as pd
import re
import matplotlib.pyplot as plt

# Create data from CSV strings
data = pd.read_csv("../result/goodp.csv", header=None)

# Extract data using regex
letters = []
rawLetters = data.iloc[:, 0].values
edm_values = data.iloc[:, 1].values
rdma_values = data.iloc[:, 2].values

for line in rawLetters:
    print(line)
    # Extract letter
    letter = line[-5]
    letters.append(letter)

# Create DataFrame
df = pd.DataFrame({
    'EDM': edm_values,
    'RDMA': rdma_values
}, index=letters)


# Create bar plot
plt.figure(figsize=(8, 6))
bar_width = 0.35
x = range(len(df))
# Create bars
bars1 = plt.bar(x, df['EDM'], bar_width, label='EDM', color='blue')
bars2 = plt.bar([i+bar_width for i in x], df['RDMA'],
                bar_width, label='RDMA', color='red')

# Add value labels on top of each bar


def autolabel(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                 f'{height:.1f}',
                 ha='center', va='bottom')


autolabel(bars1)
autolabel(bars2)


# Customize plot
plt.xlabel('YCSB workload')
plt.ylabel('Million requests per second')
plt.xticks([i+bar_width/2 for i in x], df.index)
plt.legend()

plt.tight_layout()
plt.savefig('../result/goodp.png', dpi=300, bbox_inches='tight')
