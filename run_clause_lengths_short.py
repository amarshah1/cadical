import os
import random
import subprocess
import shutil
import time
from concurrent.futures import ProcessPoolExecutor
import zipfile
import sys
# from run_add_gbc_as_assumptions import *

# Configuration
TIMEOUT = 30  # seconds
TIMEOUT_GBC = 5 # timeout for checking if a gbc is trival
NUM_SCRAMBLES = 10


def filter_gbc(original_formula, gbc_file, output_folder, file_name):
    # Read the global clauses file
    print(f"original_formula: {original_formula}")
    print(f"gbc_file: {gbc_file}")
    print(f"output_folder: {output_folder}")
    print(f"file_name: {file_name}")

    with open(gbc_file, "r") as f:
        lines = f.readlines()
    
    print("here1")

    with open(original_formula, "r") as f:
        original_lines = f.readlines()

    # Process each line in the file
    print("here2")
    with open(f"{gbc_file}.filtered", "w") as gbc_file_filtered:
        print("here3")
        for line_number, line in enumerate(lines, start=1):
            print("here4")
            numbers = list(map(int, line.strip().split()))
            print(numbers)
            
            # Ensure the last number is 0
            # if numbers[-1] != 0:
            #     print(f"Warning: Line {line_number} in {file_name} does not end with 0.")
            #     break
            #     continue
            
            # Compute clause_len: count non-zero numbers
            clause_numbers = numbers  # Exclude the last zero
            clause_len = len(clause_numbers)

            # Create directory for this file inside checking_clauses
            # file_output_folder = os.path.join(checking_clauses_folder, file_name_without_cnf)
            os.makedirs(output_folder, exist_ok=True)

            # Define output file path
            print(f"file_name : {file_name}")
            new_file_name = file_name.replace(".cnf", f"_{line_number}.cnf")
            output_file_path = os.path.join(output_folder, new_file_name)
            print("here5")


            # Modify the first line
            first_line_parts = original_lines[6].strip().split()
            print("here6")
            print(first_line_parts)
            num_one = first_line_parts[2]  # Extract num_one
            num_two = first_line_parts[3]  # Extract num_two

            # Replace the first line
            modified_first_line = f"p cnf {num_one} {int(num_two) + clause_len}\n"


            # Write the modified file
            with open(output_file_path, "w") as f:
                print("here7")
                

                f.write(modified_first_line)  # Write modified first line
                f.writelines(original_lines[7:])  # Write remaining lines

                # Append the negated assumptions at the end
                for i in clause_numbers:
                    f.write(f"{-1 * i} 0\n")

            print(f"Processed {file_name}, line {line_number}, saved to {output_file_path}.")


            os.makedirs(f"{output_folder}/global_clauses/", exist_ok = True)
            # os.makedirs(f"{output_folder}/proofs/", exist_ok = True)

            cmd = f'CADICAL_FILENAME="{output_folder}/global_clauses/{file_name}_{i}.global_clauses.txt" build/cadical --report=true --chrono=false --inprocessing=true {output_file_path}'# {output_folder}/proofs/{file_name}_{i}.proof.pr'
            
            start_time = time.time()
            num_conflicts = None
            conflicts_file = f"{output_folder}/global_clauses/{file_name}_{i}.global_clauses.txt_conflicts"
            print("here8", cmd)
            try:
                result = subprocess.run(cmd, shell=True, timeout=TIMEOUT_GBC, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                elapsed_time = time.time() - start_time
                print(f"1. The file {file_name} terminated in time {elapsed_time}!")
            except subprocess.TimeoutExpired:
                elapsed_time = time.time() - start_time
                print(f"1. The file {file_name} timed out in time {elapsed_time}!")
            except subprocess.CalledProcessError as result:
                elapsed_time = time.time() - start_time
                if result.returncode == 10:
                    try:
                        with open(conflicts_file, "r") as f:
                            num_conflicts = int(f.read().strip())
                    except (FileNotFoundError, ValueError) as e:
                        print(f"Warning: Could not read conflicts from {conflicts_file} ({e})")
                    print(f"The file {file_name} returned SAT in time {elapsed_time}!")   
                elif result.returncode == 20:
                    print("found one that is unsat!")
                    try:
                        with open(conflicts_file, "r") as f:
                            num_conflicts = int(f.read().strip())
                    except (FileNotFoundError, ValueError) as e:
                        print(f"Warning: Could not read conflicts from {conflicts_file} ({e})")
                    print(f"1. The file {file_name} returned UNSAT in time {elapsed_time}!")   
                else:
                    print(f"1.The file {file_name} returned a non-zero exit {result.returncode} status in time {elapsed_time}!")   

            print(f"the num_conflicts is {num_conflicts}")
            if num_conflicts == 0:
                print(f"We are writing in this line into filtered gbc {line}")
                gbc_file_filtered.write(line)
                # gbc_file_filtered.write("\n")
                gbc_file_filtered.flush()
                print("right after a write")
            else:
                print(f"The num_conflicts is {num_conflicts}")



def process_file(file_name, input_dir, results_directory, mode):
    """Function to process a single file in parallel execution."""
    print(f"We are looking at {file_name}")
    file_path = os.path.join(input_dir, file_name)
    new_file_name = file_name.replace('.cnf', '')
    # file_directory = f"{results_directory}/{file_name}/"
    # print(file_directory)
    # os.makedirs(file_directory, exist_ok=True)
    file_directory = f"{results_directory}/{new_file_name}/"
    os.makedirs(file_directory, exist_ok=True)
    # os.makedirs(f"{file_directory}/global_clauses/", exist_ok=True)
    # os.makedirs(f"{file_directory}/proofs/", exist_ok=True)
    for i in range(NUM_SCRAMBLES):

        sub_directory = f"{results_directory}/{new_file_name}/{new_file_name}_scramble{i}"
        # print(results_directory)
        os.makedirs(sub_directory, exist_ok=True)
        os.makedirs(f"{sub_directory}/global_clauses/", exist_ok=True)
        os.makedirs(f"{sub_directory}/proofs/", exist_ok=True)


        new_file_path = f'{sub_directory}/{new_file_name}_scramble{i}.cnf'
        scramble_cmd = f'../scranfilize/scranfilize -c 1 -f 0 -v 0 --force {file_path} {new_file_path}'

        print(scramble_cmd)
        try:
            subprocess.run(scramble_cmd, shell = True)
        except Exception as e:
            print(e)
        print("here")

        # Construct command
        # output_folder = f"{results_directory}/global_clauses/{file_name}"
        # gbc_file_path = f"{output_folder}/{file_name}_{i}.global_clauses.txt"

        gbc_file_path = f"{sub_directory}/global_clauses/{new_file_name}_{i}.global_clauses.txt"
        proof_path = f"{sub_directory}/proofs/{new_file_name}_{i}.proof.pr"
        cmd = f'CADICAL_FILENAME="{gbc_file_path}" build/cadical --report=true --chrono=false --inprocessing=true '
        if mode != "cadical":
            cmd += '--global=true '
            if mode  == "bcp":
                cmd += "--globalbcp=true "
            if mode == "no_shrink":
                cmd += "--globalnoshrink=true "
            cmd += "--globalrecord=true "
        cmd += f'--verbose=3 --no-binary {new_file_path} {proof_path}'

        print(cmd)

        start_time = time.time()
        try:
            result = subprocess.run(cmd, shell=True, timeout=TIMEOUT, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            elapsed_time = time.time() - start_time
            print(f"The file {file_name} terminated in time {elapsed_time}!")
        except subprocess.TimeoutExpired:
            elapsed_time = time.time() - start_time
            print(f"The file {file_name} timed out in time {elapsed_time}!")
        except subprocess.CalledProcessError as result:
            elapsed_time = time.time() - start_time
            if result.returncode == 10:
                print(f"The file {file_name} returned SAT in time {elapsed_time}!")   
            elif result.returncode == 20:
                print(f"The file {file_name} returned UNSAT in time {elapsed_time}!")   
            else:
                print(f"The file {file_name} returned a non-zero exit status {result.returncode} in time {elapsed_time}!")   

        # Zip the proof file
        # proof_path = f"{results_directory}/proofs/{file_name}/{file_name}_{i}.proof.pr"
        zip_proof_path = f"{sub_directory}/proofs/{new_file_name}_{i}.zip"

        with zipfile.ZipFile(zip_proof_path, "w", zipfile.ZIP_DEFLATED) as zipf:
            zipf.write(file_path, arcname=proof_path)
        os.remove(proof_path)

        # creating a filter version of the gbc
        filter_gbc(new_file_path, gbc_file_path, f"{sub_directory}/test_files", file_name)

        os.rmdir(f"{sub_directory}/test_files",)


        print(f"We have finished on file {file_name}")

def run_clause_lengths(results_directory="results", bcp=False):
    """Runs clause length analysis in parallel."""
    # todo: note i added the gbc one instead
    # input_dir = "satcomp_benchmarks/"

    input_dir = "satcomp_benchmarks/"
    # proofs_dir = f"{results_directory}/proofs"
    # clauses_dir = f"{results_directory}/global_clauses"
    # scramble_dir = f"{results_directory}/scramble"


    os.makedirs(results_directory, exist_ok=True)
    # os.makedirs(proofs_dir, exist_ok=True)
    # os.makedirs(clauses_dir, exist_ok=True)
    # os.makedirs(scramble_dir, exist_ok=True)


    # Select .cnf files
    files = [f for f in os.listdir(input_dir) if f.endswith(".cnf")][:40]
    print(files)

    # Parallel execution
    proc_num = 64
    with ProcessPoolExecutor(max_workers=proc_num) as executor:
        executor.map(process_file, files, [input_dir] * len(files), [results_directory] * len(files), [bcp] * len(files))

def main():
    """Main function to parse arguments and execute processing."""
    if len(sys.argv) != 3:
        print("Usage: python script.py <results_directory> <bcp>")
        sys.exit(1)
    
    results_directory = sys.argv[1]
    mode = sys.argv[2].lower()

    modes = {"original", "bcp", "no_shrink", "cadical"}

    if mode not in modes:
        print("Error: mode must be one of", modes)
        sys.exit(1)

    run_clause_lengths(results_directory, mode)

if __name__ == "__main__":
    main()