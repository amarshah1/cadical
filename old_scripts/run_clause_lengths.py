import os
import random
import subprocess
import shutil
import time
from concurrent.futures import ProcessPoolExecutor
import zipfile
import sys

# Configuration
TIMEOUT = 300  # seconds

def process_file(file_name, input_dir, results_directory, bcp):
    """Function to process a single file in parallel execution."""
    print(f"We are looking at {file_name}")
    file_path = os.path.join(input_dir, file_name)

    # Construct command
    cmd = f'CADICAL_FILENAME="{results_directory}/global_clauses/{file_name}.global_clauses.txt" build/cadical --report=true --chrono=false --inprocessing=true --global=true '
    if bcp:
        cmd += "--globalbcp=true "
    cmd += f'--globalrecord=true --verbose=3 --no-binary {file_path} {results_directory}/proofs/{file_name}.proof.pr'

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
            print(f"The file {file_name} returned a non-zero exit status in time {elapsed_time}!")   

    # Zip the proof file
    file_path = f"{results_directory}/proofs/{file_name}.proof.pr"
    zip_path = f"{results_directory}/proofs/{file_name}.zip"

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        zipf.write(file_path, arcname=f"{file_name}.proof.pr")
    os.remove(file_path)

    print(f"We have finished on file {file_name}")

def run_clause_lengths(results_directory="results", bcp=False):
    """Runs clause length analysis in parallel."""
    input_dir = "satcomp_benchmarks/"
    proofs_dir = f"{results_directory}/proofs"
    clauses_dir = f"{results_directory}/global_clauses"

    os.makedirs(results_directory, exist_ok=True)
    os.makedirs(proofs_dir, exist_ok=True)
    os.makedirs(clauses_dir, exist_ok=True)

    # Select .cnf files
    files = [f for f in os.listdir(input_dir) if f.endswith(".cnf")]

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
    bcp_input = sys.argv[2].lower()

    if bcp_input not in {"true", "false"}:
        print("Error: bcp must be 'true' or 'false'")
        sys.exit(1)

    bcp = bcp_input == "true"  # Convert to boolean
    run_clause_lengths(results_directory, bcp)

if __name__ == "__main__":
    main()
