import pandas as pd

# Load the CSV file
def compute_derivatives(csv_file, column_name, max_derivative=5):
    # Read the CSV file
    df = pd.read_csv(csv_file)

    if column_name not in df.columns:
        raise ValueError(f"Column '{column_name}' not found in the CSV file.")

    # Extract the column values
    values = df[column_name].astype(float).tolist()#[:34]

    # Compute derivatives
    for derivative_order in range(0, max_derivative + 1):
        average = sum(values) / len(values) if values else 0
        print(f"{derivative_order}th derivative:")
        print(values)
        print(f"Average of {derivative_order}th derivative: {average}\n")
        values = [values[i + 1] - values[i] for i in range(len(values) - 1)]

# Specify the input parameters
csv_file = "pigeonhole_results_5mintimeout_to_38.csv"
column_name = "Elapsed Time (Sadical)"

# Compute and print derivatives
compute_derivatives(csv_file, column_name)
