import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
file_name = "pigeonhole_results_5minfinal.csv"
data = pd.read_csv(file_name)

# Extract x-axis values
x_axis = data["Pigeonhole Number"]

# Plot 1: Elapsed Time
plt.figure(figsize=(10, 6))
plt.plot(x_axis, data["Elapsed Time (Assumptions)"], label="CAutiCal (our Solver)", marker="o", markersize=3)
plt.plot(x_axis, data["Elapsed Time (Original)"], label="CaDiCal", marker="o",markersize=3)
plt.plot(x_axis, data["Elapsed Time (Sadical)"], label="SaDiCal", marker="o", markersize=3)
plt.xlabel("Pigeonhole Number")
plt.ylabel("Elapsed Time (seconds)")
plt.title("Elapsed Time vs. Pigeonhole Number")
plt.legend()
plt.grid()
plt.savefig("graphs/elapsed_time_graph.png", dpi=300)
#plt.show()

# Plot 2: Proof Length
plt.figure(figsize=(10, 6))
plt.plot(x_axis, data["Proof Length (Assumptions)"], label="CAutiCal (Our Solver)", marker="o",markersize=3)
plt.plot(x_axis, data["Proof Length (Original)"], label="CaDiCal", marker="o", markersize=3)
plt.plot(x_axis, data["Proof Length (Sadical)"], label="SaDiCal", marker="o", markersize=3, color="green")
plt.xlabel("Pigeonhole Number")
plt.ylabel("Proof Length")
plt.title("Length of Proof")
plt.legend()
plt.grid()
plt.yscale("log")
plt.savefig("graphs/proof_length_graph_log.png", dpi=300)
#plt.show()

# Plot 3: Global % (Assumptions)
plt.figure(figsize=(10, 6))
plt.plot(x_axis, data["Global % (Assumptions)"], label="Global % (Assumptions)", color="purple", marker="o",markersize=3)
plt.xlabel("Pigeonhole Number")
plt.ylabel("% of Time ")
plt.title("% of Time CAutiCal Spends Finding Globally Blocked Clauses")
plt.legend()
plt.grid()
plt.savefig("graphs/global_percentage_graph.png", dpi=300)
#plt.show()
