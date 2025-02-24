import csv 
import pandas as pd
import ast
import numpy as np


df = pd.read_csv("useful_clauses2.csv")
total_timeout_clauses = 0
total_unsat_clauses = 0
timout_percentages = []


for i, (index, row) in enumerate(df.iterrows()):
    timeout_num = len(ast.literal_eval(row['timeout_results']))
    unsat_num = len(ast.literal_eval(row['unsat_results']))
    total_num = timeout_num + unsat_num
    timout_percentages.append(timeout_num/total_num)
    total_timeout_clauses += timeout_num
    total_unsat_clauses += unsat_num


print(f"The total number of UNSAT is {total_unsat_clauses}")
print(f"The total number of timeouts is {total_timeout_clauses}")
print(f"The total percentage is {total_timeout_clauses/ (total_timeout_clauses + total_unsat_clauses)}")


print(f"The median percentage {np.median(timout_percentages)}")


