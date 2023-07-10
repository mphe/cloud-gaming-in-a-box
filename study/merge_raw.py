#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pandas as pd
import sys
import argparse
from typing import List


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csvs", nargs="+", help="List of CSV files to process")
    parser.add_argument("--output", "-o", help="Output CSV file")
    args = parser.parse_args()

    dfs: List[pd.DataFrame] = []

    for csv_file in args.csvs:
        csv: pd.DataFrame = pd.read_csv(csv_file, dtype=str, index_col="key")
        csv = csv.T.reset_index().drop(columns="index").rename_axis(index="", columns="")
        dfs.append(csv)

    pd.concat(dfs).to_csv(args.output, index=False)

    return 0


if __name__ == "__main__":
    sys.exit(main())
