import os
import glob
import pandas as pd

# Define the directory containing the CSV files
directory = "./logs"  # Replace with your actual directory path

# Use glob to find all .csv files in the directory
csv_files = glob.glob(os.path.join(directory, "*.csv"))
outCsvName = "../result/latency.csv"
outCsvName_err = "../result/latency_err.csv"

expList = pd.Series(["edm", "numa", "rdma"])
ratioList = pd.Series(["10.0", "2.0", "1.0", "0.5", "0.1"])

outDF = pd.DataFrame(index=ratioList, columns=expList)
outDF_err = pd.DataFrame(index=ratioList, columns=expList)


# Loop through each CSV file and process it
for csv_file in csv_files:
    trace = csv_file.split(sep='_')[-1]
    expName = trace.split(sep='-')[0]
    expRatio = trace.split(sep='-')[-1][:-4]

    # Read the CSV into a DataFrame
    df = pd.read_csv(csv_file,header=None)

    # Check if the second column is available
    if df.shape[1] < 2:
        print(f"File '{csv_file}' does not have a second column.")
        continue

    # Extract the second column (assuming columns are indexed from 0)
    second_column = df.iloc[:, 1]

    # Calculate statistics (min, max, and mean)
    min_value = second_column.min()
    max_value = second_column.max()
    mean_value = second_column.mean()

    outDF.loc[expRatio, expName] = mean_value
    outDF_err.loc[expRatio, expName] = max_value-min_value

    # Prepare a DataFrame to hold the results
    # summary = pd.DataFrame({
    #     "Statistic": ["Min", "Max", "Average"],
    #     "Value": [min_value, max_value, mean_value]
    # })

    # Append the summary to the bottom of the original DataFrame
    # result = pd.concat([df, summary], ignore_index=True)

    # Write the updated DataFrame back to the same CSV file
    # result.to_csv(csv_file, index=False)

    # print(
    #     f"Processed '{csv_file}' - Appended summary statistics to the bottom.")
    # print(
    #     f"Processed '{csv_file}' - Inserted mean value to output dataframe.")

print(outDF)
print(outDF_err)
outDF.to_csv(outCsvName)
outDF_err.to_csv(outCsvName_err)
