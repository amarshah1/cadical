import subprocess
import time
import csv
import os
import re

def run_command(command, pigeonhole_number, output_log, proof_path):
    start_time = time.time()

    # Run the command with a timeout of 30 seconds
    try:
        result = subprocess.run(command, shell=True, timeout=600, capture_output=True, text=True)
    except subprocess.TimeoutExpired:
        return None, None, None, None , None, None, None, None, None, None, None, None # Return None for failed execution

    end_time = time.time()
    elapsed_time = end_time - start_time

    # Get proof length
    try:
        with open(proof_path, 'r') as proof_file:
            proof_length = len(proof_file.readlines())
    except FileNotFoundError:
        proof_length = None

    # Write output to the log file
    os.makedirs(os.path.dirname(output_log), exist_ok=True)
    with open(output_log, 'w') as log_file:
        log_file.write(result.stdout)

    # Extract the specific values from the log (time and percentage)
    search_time, search_percentage = extract_value_from_log(result.stdout, 'search')
    unstable_time, unstable_percentage = extract_value_from_log(result.stdout, 'unstable')
    global_time, global_percentage = extract_value_from_log(result.stdout, 'global')
    parse_time, parse_percentage = extract_value_from_log(result.stdout, 'parse')
    simplify_time, simplify_percentage = extract_value_from_log(result.stdout, 'simplify')

    return elapsed_time, proof_length, search_time, search_percentage, unstable_time, unstable_percentage, global_time, global_percentage, parse_time, parse_percentage, simplify_time, simplify_percentage

def extract_value_from_log(log, keyword):
    pattern = re.compile(rf'c\s+(\d+\.\d+)\s+(\d+\.\d+)%\s+.*{keyword}')
    match = pattern.search(log)
    if match:
        time_value = float(match.group(1))
        percentage_value = float(match.group(2))
        return time_value, percentage_value
    return None, None

def main():
    # Create a CSV file to store results
    csv_file = 'pigeonhole_results_10mintimeout_last5.csv'
    with open(csv_file, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['Pigeonhole Number', 'Elapsed Time (Assumptions)', 'Proof Length (Assumptions)', 
                         'Search Time (Assumptions)', 'Search % (Assumptions)', 'Unstable Time (Assumptions)', 'Unstable % (Assumptions)',
                         'Global Time (Assumptions)', 'Global % (Assumptions)', 'Parse Time (Assumptions)', 'Parse % (Assumptions)',
                         'Simplify Time (Assumptions)', 'Simplify % (Assumptions)', 'Elapsed Time (Original)', 
                         'Proof Length (Original)', 'Elapsed Time (Sadical)', 'Proof Length (Sadical)'])
        
        finished_original = True
        finished_cadical = True

        for pigeonhole_number in range(38, 41):  # From 2 to 40
            print(f"Running on pigeonhole {pigeonhole_number}")
            # Prepare file paths and commands
            cadical_command = f"build/cadical --inprocessing=true --global=true --no-binary --maxglobalblock=1 pigeonhole/assumptions/pigeonhole_{pigeonhole_number}.cnf pigeonhole/proofs/assumptions/pigeonhole_{pigeonhole_number}.pr"
            cadical_output_log = f"pigeonhole/output_logs/assumptions/pigeonhole_{pigeonhole_number}.txt"
            cadical_proof_path = f"pigeonhole/proofs/assumptions/pigeonhole_{pigeonhole_number}.pr"

            # Run the Cadical command
            if not finished_cadical:
                print("Running Assumptions")
                cadical_results = run_command(cadical_command, pigeonhole_number, cadical_output_log, cadical_proof_path)
            else:
                cadical_results = None, None, None, None, None, None, None, None, None, None, None, None

            cadical_elapsed_time, cadical_proof_length, cadical_search_time, cadical_search_percentage, cadical_unstable_time, cadical_unstable_percentage, cadical_global_time, cadical_global_percentage, cadical_parse_time, cadical_parse_percentage, cadical_simplify_time, cadical_simplify_percentage = cadical_results

            # Run the original Cadical command
            original_command = f"cadical --no-binary pigeonhole/original/pigeonhole_{pigeonhole_number}.cnf pigeonhole/proofs/original/pigeonhole_{pigeonhole_number}.pr"
            original_output_log = f"pigeonhole/output_logs/original/pigeonhole_{pigeonhole_number}.txt"
            original_proof_path = f"pigeonhole/proofs/original/pigeonhole_{pigeonhole_number}.pr"

            if finished_original:
                original_elapsed_time, original_proof_length = None, None
            else:
                print("Running Assumptions")
                original_elapsed_time, original_proof_length, _, _, _, _, _, _, _, _, _, _ = run_command(original_command, pigeonhole_number, original_output_log, original_proof_path)
                if original_elapsed_time == None:
                    finished_original = True

            # Run Sadical command
            sadical_command = f"../sadical/sadical --no-binary pigeonhole/original/pigeonhole_{pigeonhole_number}.cnf pigeonhole/proofs/sadical/pigeonhole_{pigeonhole_number}.pr -f"
            sadical_output_log = f"pigeonhole/output_logs/sadical/pigeonhole_{pigeonhole_number}.txt"
            sadical_proof_path = f"pigeonhole/proofs/sadical/pigeonhole_{pigeonhole_number}.pr"
            print("Running Sadical")
            sadical_results = run_command(sadical_command, pigeonhole_number, sadical_output_log, sadical_proof_path)

            if sadical_results is None:
                sadical_elapsed_time, sadical_proof_length = None, None
            else:
                sadical_elapsed_time, sadical_proof_length, _, _, _, _, _, _, _, _, _, _ = sadical_results
                # print(sadical_proof_length)

            # Write the results into the CSV
            writer.writerow([pigeonhole_number, cadical_elapsed_time, cadical_proof_length, 
                             cadical_search_time, cadical_search_percentage, cadical_unstable_time, cadical_unstable_percentage, 
                             cadical_global_time, cadical_global_percentage, cadical_parse_time, cadical_parse_percentage, 
                             cadical_simplify_time, cadical_simplify_percentage, 
                             original_elapsed_time, original_proof_length, sadical_elapsed_time, sadical_proof_length])

if __name__ == "__main__":
    main()
