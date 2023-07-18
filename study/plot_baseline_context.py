#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from matplotlib import pyplot as plt
from matplotlib.ticker import MaxNLocator
import pandas as pd
import sys
import argparse
from typing import List, Dict
from plot_utils import plot_to_pdf, subplot, style_setup, bar_with_ci, SINGLE_SCENARIO_LABELS, plot_multi_bar_with_ci


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csvs", nargs="+", help="List of CSV files to process")
    parser.add_argument("--output", "-o", help="Output PDF")
    args = parser.parse_args()

    befores: Dict[str, List[int]] = {}
    afters: Dict[str, List[int]] = {}
    not_afters: Dict[str, List[int]] = {}

    for csv_file in args.csvs:
        csv: pd.DataFrame = pd.read_csv(csv_file, dtype=str, index_col="key")
        csv = csv.drop(index=["age", "gender", "time", "genres", "devices"])
        keys = csv.index.tolist()
        baseline_idx = keys.index("B")
        value = int(csv.loc["B"].value)

        before = keys[baseline_idx - 1][:-1]
        entries = befores.get(before, [])
        entries.append(value)
        befores[before] = entries

        after_idx = baseline_idx + 1
        after = keys[after_idx][:-1]
        after_value = int(csv.iloc[after_idx].value)
        entries = afters.get(after, [])
        entries.append(after_value)
        afters[after] = entries

        for s in (csv.iloc[:baseline_idx], csv.iloc[baseline_idx + 1:]):
            for k, v in s["value"].items():
                k = k[:-1]
                entries = not_afters.get(k, [])
                entries.append(int(v))
                not_afters[k] = entries

    style_setup()
    ax: plt.Axes

    before_df = [ pd.Series(befores.get(scenario, []), dtype=int, name=scenario) for scenario in SINGLE_SCENARIO_LABELS ]
    after_df = [ pd.Series(afters.get(scenario, []), dtype=int, name=scenario) for scenario in SINGLE_SCENARIO_LABELS ]
    notafter_df = [ pd.Series(not_afters.get(scenario, []), dtype=int, name=scenario) for scenario in SINGLE_SCENARIO_LABELS ]
    na_y = 0.1

    with subplot() as (_fig, ax2):
        with subplot() as (_fig, ax):
            color = next(ax._get_lines.prop_cycler)['color']

            for col in before_df:
                if len(col) == 0:
                    ax.text(col.name, na_y, "N/A", horizontalalignment="center")
                bar_with_ci(ax, col, col.name, color=color)
                ax2.bar(col.name, len(col), color=color)

            ax.set_title("Baseline ratings after experiencing a certain scenario")
            ax.set_xlabel("Scenario")
            ax.set_ylabel("Rating")

            ax2.set_title("Frequency of scenarios appearing before Baseline")
            ax2.set_xlabel("Scenario")
            ax2.set_ylabel("Frequency")
            ax2.yaxis.set_major_locator(MaxNLocator(integer=True))

    with subplot() as (_fig, ax):
        color = next(ax._get_lines.prop_cycler)['color']

        for col in after_df:
            ax.bar(col.name, len(col), color=color)

        ax.set_title("Frequency of scenarios appearing after Baseline")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Frequency")
        ax.yaxis.set_major_locator(MaxNLocator(integer=True))

    with subplot(plot_width=10) as (_fig, ax):
        ind = pd.Series(range(len(SINGLE_SCENARIO_LABELS)))
        width = 0.35
        colora = next(ax._get_lines.prop_cycler)['color']
        colorb = next(ax._get_lines.prop_cycler)['color']

        for i, cola, colb in zip(ind, after_df, notafter_df):
            if len(cola) == 0:
                ax.text(i, na_y, "N/A", horizontalalignment="center")
            else:
                bar_with_ci(ax, cola, i, color=colora, width=width)
            bar_with_ci(ax, colb, i + width, color=colorb, width=width)

        ax.set_title("Scenario rating after experiencing Baseline")
        ax.set_xlabel("Scenario")
        ax.set_xticks(ind + width / 2)
        ax.set_xticklabels(SINGLE_SCENARIO_LABELS)
        ax.set_ylabel("Rating")
        ax.legend(["After baseline", "Not after baseline"])

    if args.output:
        plot_to_pdf(args.output)
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main())
