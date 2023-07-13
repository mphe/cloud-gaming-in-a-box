#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib import pyplot as plt
from matplotlib.ticker import MaxNLocator
import pandas as pd
import sys
from contextlib import contextmanager
from typing import Sequence, Tuple, cast, Union
import math

META_COLUMNS = ["age", "gender", "time", "genres", "devices"]
NUM_SCENARIOS = 9
ROUND_A_LABELS = [ "D1a", "D2a", "D3a", "L1a", "L2a", "L3a", "M1a", "M2a", "M3a" ]
ROUND_B_LABELS = [ "D1b", "D2b", "D3b", "L1b", "L2b", "L3b", "M1b", "M2b", "M3b" ]
BASELINE_LABEL = "B"
SORTED_SCENARIO_LABELS = [ BASELINE_LABEL ] + list(sum(zip(ROUND_A_LABELS, ROUND_B_LABELS), ()))
SINGLE_SCENARIO_LABELS = [ "D1", "D2", "D3", "L1", "L2", "L3", "M1", "M2", "M3" ]
COLUMN_RENAME_MAPPING = { i: i[:-1] for i in ROUND_A_LABELS + ROUND_B_LABELS }

GENDERS = (
    "Female",
    "Male",
    "Diverse",
)
TIME_SPENT = (
    "0-4 hours",
    "5-8 hours",
    "9-12 hours",
    "13-16 hours",
    "17+ hours",
)
GENRES = (
    "Action",
    "Adventure",
    "Battle royale",
    "Bullet hell",
    "Card game",
    "Construction and management",
    "Dating sim",
    "Tabletop",
    "Dungeon crawl",
    "Fighting",
    "Horror",
    "MMO",
    "Metroidvania",
    "MOBA",
    "Multiplayer",
    "Platform",
    "Puzzle",
    "Racing",
    "Rhythm",
    "Roguelike",
    "RPG",
    "Sandbox",
    "Shooter",
    "Soulslike",
    "Sports",
    "Stealth",
    "Strategy",
    "Survival",
    "Visual novel",
    "Walking simulator",
    "Other",
)
DEVICES = (
    "PC",
    "Console",
    "Hand-held",
    "Mobile",
)


def plot_to_pdf(outfile: str):
    with PdfPages(outfile) as pdf:
        for i in range(1, plt.figure().number):
            pdf.savefig(i)


@contextmanager
def subplot(*args, nrows: int = 1, ncols: int = 1, plot_width: float = 6, plot_height: float = 4.5, **kwargs):
    fig, axs = plt.subplots(*args, nrows=nrows, ncols=ncols, squeeze=True, **kwargs)
    yield fig, axs
    fig.set_size_inches(plot_width * ncols, plot_height * nrows)
    plt.tight_layout()


def plot_bar_frequency(ax: plt.Axes, col: pd.Series, labels: Sequence[str]):
    counts: pd.Series = col.value_counts().sort_index()
    values = [ counts.get(i + 1) or 0 for i in range(len(labels)) ]
    ax.bar(labels, values)
    ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel("Frequency")


def plot_bar_bitflags(ax: plt.Axes, col: pd.Series, labels: Sequence[str]):
    values = cast(pd.Series, sum(map(lambda flags: parse_bitflags(flags, len(labels)), col)))
    ax.bar(labels, values)
    ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel("Frequency")


def plot_multi_bar(ax: plt.Axes, a: pd.Series, b: pd.Series, labels: Sequence[str], width: float = 0.35, a_yerr=None, b_yerr=None, **kwargs) -> None:
    ind = pd.Series(range(len(a)))
    ax.bar(ind, a, width, yerr=a_yerr, **kwargs)
    ax.bar(ind + width, b, width, yerr=b_yerr, **kwargs)
    ax.set_xticks(ind + width / 2)
    ax.set_xticklabels(labels)


def parse_bitflags(flags: int, num_labels: int) -> pd.Series:
    return pd.Series([ int(bool(flags & (1 << i))) for i in range(num_labels) ])


def confidence_interval(y: Union[pd.Series, pd.DataFrame]):
    return 1.96 * y.std() / math.sqrt(len(y))


def annotate(ax: plt.Axes, text: str, pos: Tuple[float, float], xytext: Tuple[float, float], *args, **kwargs):
    ax.annotate(text, pos, *args, xytext=xytext, textcoords="offset points", arrowprops=dict(linestyle=":", arrowstyle="-"), **kwargs)


def boxplot(ax: plt.Axes, data, labels: Sequence[str], *args, **kwargs):
    ax.boxplot(data, *args, patch_artist=True, labels=labels, showmeans=True, meanline=True, meanprops=dict(linestyle="-", color="violet"), **kwargs)


def plot_meta(df: pd.DataFrame) -> None:
    # _fig: plt.Figure
    # axs: Tuple[Tuple[plt.Axes, ...], ...]
    ax: plt.Axes

    with subplot() as (_fig, ax):
        ax.hist(df["age"], bins=len(df["age"]))
        ax.yaxis.set_major_locator(MaxNLocator(integer=True))
        ax.set_xlabel("Age")
        ax.set_ylabel("Frequency")
        ax.set_title("Age distribution")

    with subplot() as (_fig, ax):
        plot_bar_frequency(ax, df["gender"], GENDERS)
        ax.set_title("Gender distribution")

    with subplot() as (_fig, ax):
        plot_bar_frequency(ax, df["time"], TIME_SPENT)
        ax.set_title("Average time spent on gaming per week")

    with subplot(plot_width=10, plot_height=5) as (_fig, ax):
        plot_bar_bitflags(ax, df["genres"], GENRES)
        ax.set_title("Distribution of preferred game genres")
        plt.setp(ax.get_xticklabels(), rotation=30, horizontalalignment='right')  # type: ignore

    with subplot() as (_fig, ax):
        plot_bar_bitflags(ax, df["devices"], DEVICES)
        ax.set_title("Distribution of most used devices for gaming")


def plot_scenarios(df: pd.DataFrame) -> None:  # pylint: disable=too-many-statements
    df = df.drop(columns=META_COLUMNS)

    baseline = df[BASELINE_LABEL]
    rounda: pd.DataFrame = df[ROUND_A_LABELS].rename(columns=COLUMN_RENAME_MAPPING)
    roundb: pd.DataFrame = df[ROUND_B_LABELS].rename(columns=COLUMN_RENAME_MAPPING)

    all_rounds: pd.DataFrame = pd.concat([ rounda, roundb ]).reset_index(drop=True)
    all_rounds.insert(0, BASELINE_LABEL, pd.concat([ baseline, baseline ]).reset_index(drop=True))
    all_means: pd.Series = all_rounds.mean()
    all_medians: pd.Series = all_rounds.median()

    ax: plt.Axes

    with subplot() as (_fig, ax):
        boxplot(ax, all_rounds, all_rounds.columns)
        ax.set_title("Distribution of ratings")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Rating")

    with subplot(plot_width=10) as (_fig, ax):
        boxplot(ax, df[SORTED_SCENARIO_LABELS], SORTED_SCENARIO_LABELS)
        ax.set_title("Distribution of ratings per round")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Rating")

    with subplot(ncols=2) as (_fig, (ax, ax2)):
        # Mean
        ax.bar(all_means.index, all_means, yerr=confidence_interval(all_rounds), capsize=3)
        ax.set_title("Average scenario ratings")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Average rating")
        ax.set_yticks(range(6))

        # Median
        ax2.bar(all_medians.index, all_medians)
        ax2.set_title("Median scenario ratings")
        ax2.set_xlabel("Scenario")
        ax2.set_ylabel("Median rating")
        ax2.set_yticks(range(6))

    with subplot(ncols=2) as (_fig, (ax, ax2)):
        # Mean
        plot_multi_bar(ax, rounda.mean(), roundb.mean(), SINGLE_SCENARIO_LABELS, a_yerr=confidence_interval(rounda), b_yerr=confidence_interval(roundb), capsize=3)
        ax.set_title("Mean rating - Comparison first vs second round")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Mean rating")
        ax.set_yticks(range(6))
        ax.legend(["First round", "Second round"])

        # Median
        plot_multi_bar(ax2, rounda.median(), roundb.median(), SINGLE_SCENARIO_LABELS)
        ax2.set_title("Median rating - Comparison first vs second round")
        ax2.set_xlabel("Scenario")
        ax2.set_ylabel("Median rating")
        ax2.set_yticks(range(6))
        ax2.legend(["First round", "Second round"])

    with subplot() as (_fig, ax):
        delays = [ 0, 50, 100, 200 ]
        ax.plot(delays, all_means[[ "B", "D1", "D2", "D3" ]], marker="o", label="0% Loss")
        ax.plot(delays, all_means[[ "B", "M1", "M2", "M3" ]], marker="o", label="0.1% Loss")
        ax.set_title("Average delay rating")
        ax.set_xlabel("Delay (ms)")
        ax.set_ylabel("Average rating")
        ax.legend()

    with subplot() as (_fig, ax):
        loss = [ 0, 0.05, 0.1, 1 ]
        ax.plot(loss, all_means[[ "B", "L1", "L2", "L3" ]], marker="o")
        ax.plot([ 0.1, 0.1, 0.1 ], all_means[[ "M1", "M2", "M3" ]], "x")
        annotate(ax, "M1", (0.1, all_means["M1"]), (-40, -20))
        annotate(ax, "M2", (0.1, all_means["M2"]), (+30, -30))
        annotate(ax, "M3", (0.1, all_means["M3"]), (-40, -20))
        ax.set_title("Average packet loss rating")
        ax.set_xlabel("Packet loss probability (%)")
        ax.set_ylabel("Average rating")


def main() -> int:
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", help="Input CSV")
    parser.add_argument("--output", "-o", help="Output PDF")
    args = parser.parse_args()

    # df: pd.DataFrame = pd.read_csv(args.csv)
    df: pd.DataFrame = pd.read_csv("./data.csv")

    matplotlib.rcParams.update({
        "axes.grid": True,
        "axes.axisbelow": True,
        "grid.alpha": 0.5,
        "grid.linestyle": ":",
    })

    # plot_meta(df)
    plot_scenarios(df)

    if args.output:
        plot_to_pdf(args.output)
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main())
