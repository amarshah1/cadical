import os

# Define folder paths
satcomp_benchmarks_folder = "satcomp_benchmarks/"
# global_clauses_folder = "results_original_short/global_clauses/"
checking_clauses_folder = "checking_clauses_filtercorrect/"
results_folder = "results_scranfilized"
num_shuffles = 10

print("STARTING ADD_GBC_TO_END")


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
        global_clauses_file_path = os.path.join(file_path, "global_clauses/", file_name_without_cnf + f"_{i}.global_clauses.txt.filteredcorrect")
        if not os.path.exists(file_path):
            print(f"File path {file_path} not found. Skipping {file_name}")

            continue
        # Check if the global clauses file exists
        if not os.path.exists(global_clauses_file_path):
            print(f"Global clauses file {global_clauses_file_path} not found. Skipping {file_name}")
            continue

        # Read the global clauses file
        with open(global_clauses_file_path , "r") as f:
            global_clauses_lines = f.readlines()

        with open(file_path + f"{file_name_without_cnf}_scramble{i}.cnf", "r") as f:
            original_lines = f.readlines()


        
        num_clauses = len(global_clauses_lines)
        # print(f"The num_lines are {num_lines}")
        # print(global_clauses_lines)
        # print(original_lines[6:])
        original_lines = original_lines[6:]


        if num_clauses != 0:
        # if True:
            os.makedirs(file_output_folder, exist_ok = True)
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
                    f.write(f"{line.strip()} 0\n")




        # # Process each line in the file
        # for line_number, line in enumerate(lines, start=1):
    
     
            
        #     numbers = list(map(int, line.strip().split()))
            
            
        #     # Ensure the last number is 0
        #     # if numbers[-1] != 0:
        #     #     print(f"Warning: Line {line_number} in {file_name} does not end with 0.")
        #     #     break
        #     #     continue
            
        #     # Compute clause_len: count non-zero numbers
        #     clause_numbers = numbers[:-1]  # Exclude the last zero
        #     clause_len = len(clause_numbers)

        #     # Create directory for this file inside checking_clauses
        #     file_output_folder = os.path.join(checking_clauses_folder, file_name_without_cnf)
        #     os.makedirs(file_output_folder, exist_ok=True)

        #     # Define output file path
        #     output_file_path = os.path.join(file_output_folder, f"{file_name_without_cnf}_{line_number}.cnf")


        #     # Modify the first line
        #     # original_lines = original_lines
        #     first_line_parts = original_lines[6].strip().split()
        #     original_lines_rest = original_lines[7:]
        #     print(first_line_parts)
        #     num_one = first_line_parts[2]  # Extract num_one
        #     num_two = first_line_parts[3]  # Extract num_two

        #     # Replace the first line
        #     modified_first_line = f"p cnf {num_one} {int(num_two) + clause_len}\n"


        #     # Write the modified file
        #     with open(output_file_path, "w") as f:
                

        #         f.write(modified_first_line)  # Write modified first line
        #         f.writelines(original_lines_rest)  # Write remaining lines

        #         # Append the negated assumptions at the end
        #         for i in clause_numbers:
        #             f.write(f"{-1 * i} 0\n")

        #     print(f"Processed {file_name}, line {line_number}, saved to {output_file_path}.")
