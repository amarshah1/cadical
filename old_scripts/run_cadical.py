import argparse
import subprocess
import time
import os

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Run the CaDiCaL solver with specified parameters.")
    parser.add_argument("pigeonhole_number", type=int, help="The pigeonhole number (e.g., 10).")
    parser.add_argument("--folder", default="original", help="The folder name (default: 'original').")
    parser.add_argument("--timeout", type=int, default=5, help="Timeout in seconds (default: 10).")
    parser.add_argument("--maxglobalblock", type=int, default=1, help="Maximum global block (default: 1).")
    parser.add_argument("--log", type=bool, default=False, help="whether we should record a log")

    args = parser.parse_args()

    # Prepare variables
    pigeonhole_number = args.pigeonhole_number
    folder = args.folder
    timeout = args.timeout
    maxglobalblock = args.maxglobalblock
    log = args.log

    # File paths
    cnf_file = f"pigeonhole/{folder}/pigeonhole_{pigeonhole_number}.cnf"
    pr_file = f"pigeonhole/proofs/{folder}/pigeonhole_{pigeonhole_number}.pr"
    log_file = f"pigeonhole/output_logs/{folder}/pigeonhole_{pigeonhole_number}.txt"

    # Ensure output folder exists
    os.makedirs(os.path.dirname(log_file), exist_ok=True)

    # Command to run
    command = [
        "build/cadical",
        "--report=true",
        "--inprocessing=true",
        "--global=true",
        "--verbose=3",
        "--no-binary",
        f"--maxglobalblock={maxglobalblock}",
        cnf_file,
        pr_file
    ] + (["--log"] if log else [])

    try:
        # Measure runtime
        start_time = time.time()
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout
        )
        end_time = time.time()

        # Write stdout to log file
        with open(log_file, "w") as f:
            f.write(result.stdout.decode())

        # Print runtime
        runtime = end_time - start_time
        print(f"Total runtime: {runtime:.2f} seconds")
    except subprocess.TimeoutExpired as e:
        with open(log_file, "w") as f:
            if e.stdout:
                f.write(e.stdout.decode())
            if e.stderr:
                f.write("\n--- STDERR ---\n")
                f.write(e.stderr.decode())
        print(f"Command timed out after {timeout} seconds.")
    except Exception as e:
        # with open(log_file, "w") as f:
        #     f.write(result.stdout.decode())
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
