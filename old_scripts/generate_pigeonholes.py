import os

def generate_pigeonhole_instance(k):
    """Generate a pigeonhole instance with k + 1 pigeons and k holes."""
    pigeons = k + 1
    holes = k
    clauses = []

    # At least one hole per pigeon
    for i in range(1, pigeons + 1):
        clauses.append(" ".join(str((i - 1) * holes + j) for j in range(1, holes + 1)) + " 0")

    # At most one pigeon per hole (pairwise constraints)
    for j in range(1, holes + 1):
        for i1 in range(1, pigeons + 1):
            for i2 in range(i1 + 1, pigeons + 1):
                clauses.append(f"-{(i1 - 1) * holes + j} -{(i2 - 1) * holes + j} 0")

    return clauses

def generate_encoding_comments(k):
    """Generate comments describing the encoding."""
    pigeons = k + 1
    holes = k
    comments = [f"c at-most-one of {pigeons} pigeons in {holes} holes"]
    variable = 1
    for i in range(1, pigeons + 1):
        for j in range(1, holes + 1):
            comments.append(f"c p{i}@{j} {variable}")
            variable += 1
    return comments

def save_pigeonhole_instance(k, directory):
    """Save the pigeonhole instance to a file."""
    pigeons = k + 1
    holes = k
    filename = os.path.join(directory, f"pigeonhole_{k}.cnf")
    clauses = generate_pigeonhole_instance(k)
    comments = generate_encoding_comments(k)

    # Calculate number of variables and clauses
    num_vars = pigeons * holes
    num_clauses = len(clauses)

    with open(filename, "w") as f:
        # Write comments at the start
        f.write("\n".join(comments) + "\n")
        # Write problem line
        f.write(f"p cnf {num_vars} {num_clauses}\n")
        # Write clauses
        f.write("\n".join(clauses) + "\n")
    print(f"Generated {filename}")

def main():
    # Create the output directory
    directory = "pigeonhole/original"
    os.makedirs(directory, exist_ok=True)

    # Generate instances for k = 2 to k = 40
    for k in range(2, 41):
        save_pigeonhole_instance(k, directory)

if __name__ == "__main__":
    main()
