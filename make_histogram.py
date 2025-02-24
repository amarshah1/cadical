import os
import re
import matplotlib.pyplot as plt

def count_numbers_in_line(line):
    """Count the number of numbers (integers or floats) in a line."""
    return len(re.findall(r'-?\d+\.?\d*', line))

def process_files_in_folder(folder_path):
    """Process each file in the folder and return the combined list of numbers."""
    all_numbers = []

    # Iterate over all files in the folder
    clause_lengths = []
    for filename in os.listdir(folder_path):
        file_path = os.path.join(folder_path, filename)

        # Only process text files
        if os.path.isfile(file_path):
            with open(file_path, 'r') as file:
                for line in file:
                    # Count numbers in each line
                    if line.strip():
                        clause_lengths += [len(line.split())]
                    

    return clause_lengths

def plot_histogram(numbers, output_file):
    """Plot a histogram of the numbers and save it to a file."""
    plt.figure(figsize=(10, 6))
    plt.hist(numbers, bins=100, range=(0,120), edgecolor='black')
    plt.title(f'Histogram of {output_file}')
    plt.xlabel('Value')
    plt.ylabel('Frequency')
    plt.savefig(output_file)
    print(f'Histogram saved to {output_file}')

def main():
    folder_path = 'results/global_clauses'  # Change to your folder path
    # folder_paths = ['global_clauses/random_false_learn_false', 'global_clauses/random_true_learn_false', 'global_clauses/random_false_learn_true', 'global_clauses/random_true_learn_true']
    # for folder_path in folder_paths:
    all_numbers = process_files_in_folder(folder_path)
    print(f'Total numbers found: {len(all_numbers)}')
    output_file = folder_path[7:] + "_histogram.png"
    plot_histogram(all_numbers, output_file)


if __name__ == "__main__":
    main()
