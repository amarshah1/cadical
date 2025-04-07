Cautical is a modified version of Cadical 

We include a few scripts for testing Cautical. First you want to generate PR clauses using:

```shell
python3 run_clause_lengths_short.py results_scranfilized global-preprocess preprocessing
```

You can then add the clauses to the end:

```shell
python run_add_gbc_to_end.py --gbc_appendix preprocessing --results_folder results-scranfilized --checking_clauses_folder checking_clauses
```

This does not need to be run with a job. Finally, you can run the formulas with the PR clauses appended. Below will run all of the formulas corresponding to the 0 scranfilize from the folder `checking_clauses`

```shell
python3 run_queries_with_gbc.py 0 checking_clauses
```

The best way to do this is to run a job using `run_queries_with_gbc{n}.job` where you replace `n` with a number from 0-9. The results can be gotten from the slurm output.