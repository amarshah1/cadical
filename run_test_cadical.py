import os
import time
import subprocess
import concurrent.futures

TIMEOUT = 60
GARBAGE_DIR = "garbage"
SATCOMP_DIR = "satcomp_benchmarks"
CADICAL_BINARY = "build/cadical"
MAX_WORKERS = 64

def run_cadical(file_name, with_gbc):
    gbc_flag = "--global=true --globalrecord=true" if with_gbc else ""
    cmd = (
        f'CADICAL_FILENAME="{GARBAGE_DIR}/{file_name}.global_clauses.txt" '
        f'{CADICAL_BINARY} --report=true --chrono=false --inprocessing=true {gbc_flag} '
        f'{SATCOMP_DIR}/{file_name}'
    )
    conflicts_file = f"{GARBAGE_DIR}/{file_name}.global_clauses.txt"
    
    start_time = time.time()
    try:
        subprocess.run(cmd, shell=True, timeout=TIMEOUT, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        elapsed_time = time.time() - start_time
        print(f"{'1' if with_gbc else '2'}. The file {file_name} terminated in time {elapsed_time}!")
    except subprocess.TimeoutExpired:
        elapsed_time = time.time() - start_time
        print(f"{'1' if with_gbc else '2'}. The file {file_name} timed out in time {elapsed_time}!")
    except subprocess.CalledProcessError as result:
        elapsed_time = time.time() - start_time
        if result.returncode in [10, 20]:
            status = "SAT" if result.returncode == 10 else "UNSAT"
            print(f"{'1' if with_gbc else '2'}. The file {file_name} returned {status} in time {elapsed_time}!")
        else:
            print(cmd)
            print(f"{'1' if with_gbc else '2'}. The file {file_name} returned a non-zero exit {result.returncode} status in time {elapsed_time}!")

def process_file(file_name):
    run_cadical(file_name, with_gbc=True)  # First run with --global=true
    run_cadical(file_name, with_gbc=False)  # Second run without gbc

def main():
    os.makedirs(GARBAGE_DIR, exist_ok=True)
    file_names = [f for f in os.listdir(SATCOMP_DIR) if os.path.isfile(os.path.join(SATCOMP_DIR, f))]
    file_names = ["dc7817dfa2817916b266c1cfacd2ee66-constraints_25_4_5_12_12_0_0_0.sanitized.cnf", 
             "0fa9521ff633b27be11525a7b0f7d8b6-jgiraldezlevy.2200.9086.08.40.41.cnf",
             "9cd3acdb765c15163bc239ae3a57f880-FmlaEquivChain_4_6_6.sanitized.cnf",
             "02066c116dbacc40ec5cca2067db26c0-mrpp_4x4#12_12.cnf",
             "7f7109dce621ef361a72b3e8cee9a962-Break_unsat_06_07.xml.cnf"]
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        executor.map(process_file, file_names)

if __name__ == "__main__":
    main()
