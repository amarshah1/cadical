import os
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Process SAT benchmark files.")
parser.add_argument("--gbc_appendix", type=str, required=True, help="GBC appendix filename")
parser.add_argument("--results_folder", type=str, required=True, help="Results folder path")
parser.add_argument("--checking_clauses_folder", type=str, required=True, help="Checking clauses folder path")
args = parser.parse_args()

# Assign command-line arguments to variables
satcomp_benchmarks_folder = "satcomp_benchmarks/"
checking_clauses_folder = args.checking_clauses_folder
results_folder = args.results_folder
gbc_appendix = args.gbc_appendix
num_shuffles = 10
sorting = True

print(f"STARTING ADD_GBC_TO_END with sorting: {sorting}")

# Ensure the checking_clauses folder exists
os.makedirs(checking_clauses_folder, exist_ok=True)

# Maximum file size in bytes (5MB = 5 * 1024 * 1024)
MAX_FILE_SIZE = 5 * 1024 * 1024  

# List all files in satcomp_benchmarks/ that are less than 5MB
file_names = [
    f for f in os.listdir(satcomp_benchmarks_folder) 
    if os.path.getsize(os.path.join(satcomp_benchmarks_folder, f)) < MAX_FILE_SIZE
]

for file_name in file_names:
    file_name_without_cnf = file_name.replace(".cnf", "")
    file_output_folder = os.path.join(checking_clauses_folder, file_name_without_cnf)

    for i in range(num_shuffles):
        file_path = f"{results_folder}/{file_name_without_cnf}/{file_name_without_cnf}_scramble{i}/"
        global_clauses_file_path = os.path.join(file_path, "global_clauses/", f"{file_name_without_cnf}_{i}_{gbc_appendix}.txt")
        
        if not os.path.exists(file_path):
            print(f"File path {file_path} not found. Skipping {file_name}")
            continue

        if not os.path.exists(global_clauses_file_path):
            print(f"Global clauses file {global_clauses_file_path} not found. Skipping {file_name}")
            continue

        with open(global_clauses_file_path, "r") as f:
            global_clauses_lines = f.readlines()

        with open(os.path.join(satcomp_benchmarks_folder, file_name), "r") as f:
            original_lines = f.readlines()

        num_clauses = len(global_clauses_lines)
        
        if num_clauses != 0:
            os.makedirs(file_output_folder, exist_ok=True)
            output_file_path = os.path.join(file_output_folder, f"{file_name_without_cnf}_scramble{i}.cnf")
            
            with open(output_file_path, "w") as f:
                first_line_parts = original_lines[0].strip().split()
                num_one = first_line_parts[2]  # Extract num_one
                num_two = first_line_parts[3]  # Extract num_two
                modified_first_line = f"p cnf {num_one} {int(num_two) + num_clauses}\n"
                f.write(modified_first_line)
                
                for line in original_lines[1:]:
                    f.write(line)
                
                for line in global_clauses_lines:
                    if sorting:
                        sorted_numbers = sorted(map(int, line.strip().split()), key=lambda x: (x == 0, x))
                        f.write(f"{' '.join(map(str, sorted_numbers))} 0\n")  
                    else:
                        f.write(f"{line.strip()} 0\n")
