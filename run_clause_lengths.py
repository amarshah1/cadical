import os
import random
import subprocess
import shutil
import time
import csv

# Configuration
N = 5 # Number of random files to process
INPUT_DIR = "benchmarks/satcomp_official/"
PROOFS_DIR = "proofs"
CLAUSES_DIR = "global_clauses"
COMMANDS = [
    ("random_false_learn_true",  "--globalrandom=false --globallearn=true"),
    ("random_false_learn_false", "--globalrandom=false --globallearn=false"),
    ("random_true_learn_true",   "--globalrandom=true --globallearn=true"),
    ("random_true_learn_false",  "--globalrandom=true --globallearn=false")
]
TIMEOUT = 5  # seconds
OUTPUT_FILES = ["global_clauses.txt", "proof.pr"]
CSV_FILE = "clause_lengths.csv"

# Create directories
for base_dir in [PROOFS_DIR, CLAUSES_DIR]:
    os.makedirs(base_dir, exist_ok=True)
    for subfolder, _ in COMMANDS:
        os.makedirs(os.path.join(base_dir, subfolder), exist_ok=True)

# Select random files
files = [f for f in os.listdir(INPUT_DIR) if f.endswith(".cnf")]
random.shuffle(files)
selected_files = files[:N]

# Prepare CSV log
csv_data = []

# Process each file
for file_name in selected_files:
    print(f"We are looking at {file_name}")
    file_path = os.path.join(INPUT_DIR, file_name)
    
    for subfolder, cmd_options in COMMANDS:
        cmd = f"build/cadical --report=true --inprocessing=true --global=true --globalrecord=true {cmd_options} --verbose=3 --no-binary {file_path} proof.pr"
        
        start_time = time.time()
        try:
            subprocess.run(cmd, shell=True, timeout=TIMEOUT, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            elapsed_time = time.time() - start_time
        except subprocess.TimeoutExpired:
            elapsed_time = TIMEOUT  # Mark as hitting timeout
        except subprocess.CalledProcessError:
            elapsed_time = TIMEOUT  # Mark as hitting timeout
            continue  # Skip if the command fails
        
        # Copy output files
        for output_file in OUTPUT_FILES:
            if os.path.exists(output_file):
                dest_folder = CLAUSES_DIR if "global_clauses.txt" == output_file else PROOFS_DIR
                print(f"Moving {output_file} to {dest_folder}/{subfolder}/{file_name}.{output_file}")
                shutil.move(output_file, os.path.join(dest_folder, subfolder, f"{file_name}.{output_file}"))
        
        # Analyze clause lengths
        clause_lengths = []
        global_clauses_path = os.path.join(CLAUSES_DIR, subfolder, f"{file_name}.global_clauses.txt")
        
        if os.path.exists(global_clauses_path):
            with open(global_clauses_path, 'r') as f:
                clause_lengths = [len(line.split()) for line in f if line.strip()]
            # os.remove(global_clauses_path)
        
        csv_data.append([file_name, subfolder, elapsed_time] + clause_lengths)
        
# Write to CSV
with open(CSV_FILE, 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(["File Name", "Command", "Execution Time"] + [f"Clause {i+1}" for i in range(max(map(len, csv_data)) - 3)])
    writer.writerows(csv_data)

print("Processing complete. Results saved in clause_lengths.csv.")
