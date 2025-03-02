import random 

def generate_assumptions(input_file, output_file, p, h):

    # x(i, j) := pigeon i is in hole j
    def x(i, j):
        return (i - 1) * h  + j
    

    # input_file = f"{folder_name}/{file_name}.cnf"
    # output_file = f"{file_name}_assumptions_bigger.cnf"

    try:
        with open(input_file, 'r') as infile:
            # Read the original CNF content
            cnf_content = infile.readlines()

        assumptions = []

        # Update the header line "p cnf ..." to "p inccnf"
        # num_vars = 0
        for i, line in enumerate(cnf_content):
            if line.startswith("p cnf"):
                parts = line.split()
                if len(parts) == 4 and parts[2].isdigit() and parts[3].isdigit():
                    num_vars = int(parts[2])  # Store the first number
                    cnf_content[i] = "p inccnf\n"
                break

        # Generate the assumptions
        assumptions_for_end = []
        for i in range(1, p + 1):
            for j in range(i+1, h+2):
                for k in range(i+1, p):
                    assumptions.append(f"a {x(i,i)} {x(j, k)} 0 \n")
                    # assumptions.append(f"a 0 \n")
                    # assumptions.append(f"a {x(i, k)} {x(j, i)} 0 \n")
                # assumptions.append(f"a {x(j, i)} 0 \n")
            # for l in range(i, h+1):
            #     assumptions.append(f"a -{x(i, l)} 0 \n")
            


        assumptions += assumptions_for_end
        # for t in range(1, p + 1):
        #     for s in range(1, h + 1):
        #         for i in range(1, p + 1): #pigeon
        #             for j in range(1, h + 1): # hole
        #                 # if random.random() < 0.2:
        #                 # Compute literals using the formula
        #                 x_ts = t + h * (s - 1)
        #                 x_ji = j + h * (i - 1)
        #                 # Add the assumption in the specified format
        #                 assumptions.append(f"a {x_ts} {x_ji} 0\n")
                        
        # for j in range(1, num_vars+ 1):
        #     for k in range(j, num_vars + 1):
        #         assumptions.append(f"a {j} {k} 0\n")

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

for k in range(2, 41):
    generate_assumptions(f"pigeonhole/original/pigeonhole_{k}.cnf", f"pigeonhole/assumptions_no_alternating/pigeonhole_{k}.cnf", k+1, k)  # Replace with the actual file name and values for p and h
