import csv

def fix_csv_spacing(input_file, output_file, delimiter=','):
    with open(input_file, 'r', newline='', encoding='utf-8') as infile, \
         open(output_file, 'w', newline='', encoding='utf-8') as outfile:
        
        reader = csv.reader(infile, skipinitialspace=True)  # Skip extra spaces
        writer = csv.writer(outfile, delimiter=delimiter)

        for row in reader:
            cleaned_row = [cell.strip() for cell in row]  # Trim spaces from each cell
            writer.writerow(cleaned_row)

# Example usage
input_csv = "useful_clauses.csv"
output_csv = "useful_clauses2.csv"
fix_csv_spacing(input_csv, output_csv)

print(f"Cleaned CSV saved as {output_csv}")