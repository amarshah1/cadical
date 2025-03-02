import os
import shutil

# Define folder paths
satcomp_benchmarks_folder = "satcomp_benchmarks/"
results_folder = "results_original_short"
new_folder = "satcomp_benchmarks_gbc/"

# Create the new directory if it doesn't exist
os.makedirs(new_folder, exist_ok=True)

# List all files in the satcomp_benchmarks folder that end in .cnf
file_names = [f for f in os.listdir(satcomp_benchmarks_folder) if f.endswith(".cnf")]

# Initialize a counter for the total number of files
num_files = len(file_names)

for file_name in file_names:

    file_name_without_cnf = file_name[:-4]
    # Define paths
    input_file_path = os.path.join(satcomp_benchmarks_folder, file_name)
    global_clauses_file_path = os.path.join(results_folder, "global_clauses", f"{file_name}.global_clauses.txt")
    output_file_path = os.path.join(new_folder, file_name)

    # Count the number of lines in the global_clauses file
    try:
        with open(global_clauses_file_path, "r") as f:
            num_lines = sum(1 for line in f)
    except FileNotFoundError:
        print(f"Global clauses file {global_clauses_file_path} not found. Skipping {file_name}.")
        continue

    # Read the input SAT file
    with open(input_file_path, "r") as f:
        lines = f.readlines()

    # Modify the first line of the SAT file
    first_line = lines[0].strip().split()
    num_one = first_line[2]  # num_one is the first number in the first line
    num_two = first_line[3]  # num_two is the second number in the first line

    # Replace the first line with the new format
    lines[0] = f"p cnf {num_one} {int(num_two) + num_lines}\n"

    # Write the modified file to the new directory
    with open(output_file_path, "w") as f:
        f.writelines(lines)

        # Append the content of the global_clauses file with a "0" at the end of each line
        with open(global_clauses_file_path, "r") as global_clauses_file:
            for line in global_clauses_file:
                f.write(f"{line.strip()} 0\n")

    print(f"Processed {file_name}, updated and moved to {new_folder}.")
