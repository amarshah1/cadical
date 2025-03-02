import os

# Directories
input_dir = "benchmarks/satcomp"
output_dir = "benchmarks/satcomp_assumptions"

# Ensure output directory exists
os.makedirs(output_dir, exist_ok=True)

def process_cnf_file(input_path, output_path):
    neighbors_dict = {}  # Dictionary to store neighbors for each literal
    num_literals = 0

    with open(input_path, "r") as infile:
        lines = infile.readlines()

    # Read header to get the number of literals
    for i, line in enumerate(lines):
        if line.startswith("p cnf"):
            _, _, num_literals, _ = line.split()
            num_literals = int(num_literals)
            lines[i] = "p inccnf \n"

            break

    # Initialize neighbor sets for all literals
    for i in range(1, num_literals + 1):
        neighbors_dict[str(i)] = set()
        neighbors_dict[f"-{i}"] = set()

    # Parse the file and collect neighbors
    for line in lines:
        if line.startswith("p") or line.startswith("c"):  # Skip comments and problem line
            continue
        
        # Split the clause into literals
        literals = line.split()
        
        for literal in literals:
            if literal == "0":  # Clause terminator
                continue
            # Add all other literals in the clause as neighbors
            neighbors_dict[literal].update(lit for lit in literals if lit != literal and lit != "0")

    # Write the original file and add assumptions
    with open(output_path, "w") as outfile:
        outfile.writelines(lines)

        # Add assumptions for each literal
        for literal, neighbors in neighbors_dict.items():
            for neighbor in neighbors:
                outfile.write(f"a {literal} {neighbor} 0\n")

# Process all files in the input directory
for filename in os.listdir(input_dir):
    if filename.endswith(".cnf"):
        input_path = os.path.join(input_dir, filename)
        output_path = os.path.join(output_dir, filename)
        process_cnf_file(input_path, output_path)

print(f"Processed all CNF files from {input_dir} and saved results to {output_dir}")
