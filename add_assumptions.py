def generate_assumptions(file_name, p, h):
    input_file = f"{file_name}.cnf"
    output_file = f"{file_name}_assumptions.cnf"

    try:
        with open(input_file, 'r') as infile:
            # Read the original CNF content
            cnf_content = infile.readlines()

        assumptions = []

        # Update the header line "p cnf ..." to "p inccnf"
        for i, line in enumerate(cnf_content):
            if line.startswith("p cnf"):
                cnf_content[i] = "p inccnf\n"
                break

        # Generate the assumptions
        for r in range(1, p + 1):
            for l in range(r + 1, p + 1):
                for m in range(1, h + 1):
                    if r <= h:
                        # Compute literals using the formula
                        x_rr = r + p * (r - 1)
                        x_lm = l + p * (m - 1)
                        # Add the assumption in the specified format
                        assumptions.append(f"a {x_rr} {x_lm} 0\n")

        # Write the new CNF file
        with open(output_file, 'w') as outfile:
            # Write modified CNF content
            outfile.writelines(cnf_content)
            # Write assumptions
            outfile.writelines(assumptions)

        print(f"Header updated and assumptions added. Output written to {output_file}")
    except FileNotFoundError:
        print(f"Error: File {input_file} not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Example usage
generate_assumptions("benchmarks/pigeonhole/php9", 10, 9)  # Replace with the actual file name and values for p and h
