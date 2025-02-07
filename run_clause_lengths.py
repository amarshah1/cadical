import os
import random
import subprocess
import shutil
import time
# import csv
from concurrent.futures import ProcessPoolExecutor
import zipfile

# Configuration
# N = 5 # Number of random files to process
INPUT_DIR = "satcomp_benchmarks/"
PROOFS_DIR = "results/proofs"
CLAUSES_DIR = "results/global_clauses"

os.makedirs("results/", exist_ok = True)
os.makedirs(PROOFS_DIR, exist_ok=True)
os.makedirs(CLAUSES_DIR, exist_ok=True)

TIMEOUT = 20 # seconds
OUTPUT_FILES = ["global_clauses.txt", "proof.pr"]


# Select random files
files = ["7f7109dce621ef361a72b3e8cee9a962-Break_unsat_06_07.xml.cnf"] + [f for f in os.listdir(INPUT_DIR) if f.endswith(".cnf") and f != "f7109dce621ef361a72b3e8cee9a962-Break_unsat_06_07.xml.cnf"]
# random.shuffle(files)
# selected_files = files[:N]


# Define function to process each file
def process_file(file_name):
    print(f"We are looking at {file_name}")
    file_path = os.path.join(INPUT_DIR, file_name)

    cmd = f'CADICAL_FILENAME="results/global_clauses/{file_name}.global_clauses.txt" build/cadical --report=true --chrono=false --inprocessing=true --global=true --globalrecord=true --verbose=3 --no-binary {file_path} results/proofs/{file_name}.proof.pr'
    
    start_time = time.time()
    try:
        subprocess.run(cmd, shell=True, timeout=TIMEOUT, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        elapsed_time = time.time() - start_time
        print(f"The file {file_name} terminated!")
    except subprocess.TimeoutExpired:
        print(f"The file {file_name} timed out!")
    except subprocess.CalledProcessError:
        print(f"The file {file_name} returned a non-zero exit status!")   

        # return [file_name, elapsed_time]  # Return immediately in case of failure
    file_path = f"results/proofs/{file_name}.proof.pr"
    zip_path = f"results/proofs/{file_name}.zip"

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        zipf.write(file_path, arcname=f"{file_name}.proof.pr")
    os.remove(file_path)


    print(f"We have finished on file {file_name}")

    # Copy output files
    # for output_file in OUTPUT_FILES:
    #     output_file = file_name + "." + output_file
    #     print(f"We have the file: {output_file}")
    #     if os.path.exists(output_file):
    #         dest_folder = CLAUSES_DIR if output_file.endswith("global_clauses.txt") else PROOFS_DIR
    #         print(f"Moving {output_file} to {dest_folder}/{file_name}.{output_file}")
    #         shutil.move(output_file, os.path.join(dest_folder, f"{file_name}.{output_file}"))

    # # Analyze clause lengths
    # clause_lengths = []
    # global_clauses_path = os.path.join(CLAUSES_DIR, f"{file_name}.global_clauses.txt")

    # if os.path.exists(global_clauses_path):
    #     with open(global_clauses_path, 'r') as f:
    #         clause_lengths = [len(line.split()) for line in f if line.strip()]
    
    # return [file_name, elapsed_time] + clause_lengths

# Parallel execution
proc_num = 20
with ProcessPoolExecutor(max_workers=proc_num) as executor:
    results = list(executor.map(process_file, files))
