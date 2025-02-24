import os
import random
import subprocess
import shutil
import time
from concurrent.futures import ProcessPoolExecutor
import zipfile
import sys
import csv

# Configuration
TIMEOUT = 20  # seconds
input_dir = "checking_clauses/"


def process_file(subdir, results_directory):
    """Function to process a single file in parallel execution."""
    sat_results = []
    unsat_results = []
    error_results = []
    timeout_results = []
    print(os.listdir(input_dir + subdir))
    for file_name in os.listdir(input_dir + subdir):
        print(f"We are looking at {input_dir}{subdir}/{file_name}")
        file_path = os.path.join(input_dir, subdir, file_name)

        # Construct command
        cmd = f'CADICAL_FILENAME="{results_directory}/{file_name}" build/cadical {file_path}'

        start_time = time.time()
        try:
            result = subprocess.run(cmd, shell=True, timeout=TIMEOUT, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            elapsed_time = time.time() - start_time
            print(f"The file {file_name} terminated in time {elapsed_time}!")
        except subprocess.TimeoutExpired:
            elapsed_time = time.time() - start_time
            print(f"The file {file_name} timed out in time {elapsed_time}!")
            timeout_results.append((file_name, None))
        except subprocess.CalledProcessError as result:
            elapsed_time = time.time() - start_time
            if result.returncode == 10:
                onflicts_file = f"{results_directory}/{file_name}_conflicts"
                try:
                    with open(conflicts_file, "r") as f:
                        num_conflicts = int(f.read().strip())
                except (FileNotFoundError, ValueError) as e:
                    print(f"Warning: Could not read conflicts from {conflicts_file} ({e})")
                    num_conflicts = None  # Default value if reading fails

                print(f"The file {file_name} returned SAT in time {elapsed_time}!")
                sat_results.append(file_name)
            elif result.returncode == 20:
                conflicts_file = f"{results_directory}/{file_name}_conflicts"
                try:
                    with open(conflicts_file, "r") as f:
                        num_conflicts = int(f.read().strip())
                except (FileNotFoundError, ValueError) as e:
                    print(f"Warning: Could not read conflicts from {conflicts_file} ({e})")
                    num_conflicts = None  # Default value if reading fails

                print(f"The file {file_name} returned UNSAT in time {elapsed_time}!")   
                unsat_results.append((file_name, num_conflicts))
            else:
                print(f"The file {file_name} returned a non-zero exit status in time {elapsed_time}!")   
                error_results.append(file_name)


        print(f"We have finished on file {file_name}")
    
    print("We have finished on folder ", subdir)
    print("We have sat_results: ", sat_results)
    print("We have unsat_results: ", unsat_results)
    print("We have error_results: ", error_results)
    print("We have timeout_results: ", timeout_results)

    return {"subdir": subdir, "sat_results": sat_results, "unsat_results": unsat_results, "error_results": error_results, "timeout_results": timeout_results}

def process_results(results):
    output_dir = "results_gbc_csvs"
    os.makedirs(output_dir, exist_ok=True)
    
    for result in results:
        subdir = result["subdir"]
        csv_path = os.path.join(output_dir, f"{subdir}.csv")
        clause_file = f"results_original_short/global_clauses/{subdir}.cnf.global_clauses.txt"
        
        # Read global clauses file
        try:
            with open(clause_file, "r") as f:
                clauses = [line.strip() for line in f.readlines()]
        except FileNotFoundError:
            print(f"Warning: Clause file not found for {subdir}")
            continue
        
        # Collect rows for the CSV
        rows = []
        for category, label in [(result["sat_results"], "SAT"), 
                                (result["unsat_results"], "UNSAT"), 
                                (result["timeout_results"], "TIMEOUT")]:
            for item1, item2 in category:
                try:
                    i = int(item1.split("_")[-1].split(".")[0])  # Extract index from filename
                    clause = clauses[i - 1]  # 1-indexed
                    num_conflicts = item2 if label in ["SAT", "UNSAT"] else ""
                    rows.append([clause, label, num_conflicts])
                except (IndexError, ValueError):
                    print(f"Warning: Could not process {item1} in {subdir}")
        
        # Write to CSV
        with open(csv_path, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["clause", "result", "num_conflicts"])
            writer.writerows(rows)
        
        print(f"CSV written: {csv_path}")

def run_clause_lengths():
    """Runs clause length analysis in parallel."""
    clauses_dir = f"garbage_global_clauses/"

    os.makedirs(clauses_dir, exist_ok=True)

    # Select .cnf files
    files = [f for f in os.listdir(input_dir)]

    # Parallel execution
    proc_num = 64
    with ProcessPoolExecutor(max_workers=proc_num) as executor:
        results = list(executor.map(process_file, files, [clauses_dir] * len(files)))

    # Write results to a CSV file
    csv_filename = "useful_clauses3.csv"
    process_results(results)

    # Extract column names from the first dictionary
    fieldnames = results[0].keys()  

    with open(csv_filename, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    print(f"CSV saved as {csv_filename}")

def main():
    """Main function to parse arguments and execute processing."""
    
    run_clause_lengths()

if __name__ == "__main__":
    main()
