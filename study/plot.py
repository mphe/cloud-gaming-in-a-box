#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from matplotlib import pyplot as plt
from matplotlib.ticker import MaxNLocator
import pandas as pd
import sys
from typing import Sequence, Tuple, cast, Optional
from plot_utils import plot_to_pdf, subplot, style_setup, boxplot, confidence_interval, SINGLE_SCENARIO_LABELS, plot_multi_bar

META_COLUMNS = ["age", "gender", "time", "genres", "devices"]
NUM_SCENARIOS = 9
ROUND_A_LABELS = [ "D1a", "D2a", "D3a", "L1a", "L2a", "L3a", "M1a", "M2a", "M3a" ]
ROUND_B_LABELS = [ "D1b", "D2b", "D3b", "L1b", "L2b", "L3b", "M1b", "M2b", "M3b" ]
BASELINE_LABEL = "B"
SORTED_SCENARIO_LABELS = [ BASELINE_LABEL ] + list(sum(zip(ROUND_A_LABELS, ROUND_B_LABELS), ()))
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
BITFLAG_PLATFORMER = 1 << 15
DEVICES = (
    "PC",
    "Console",
    "Hand-held",
    "Mobile",
)


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


def parse_bitflags(flags: int, num_labels: int) -> pd.Series:
    return pd.Series([ int(bool(flags & (1 << i))) for i in range(num_labels) ])


def annotate(ax: plt.Axes, text: str, pos: Tuple[float, float], xytext: Tuple[float, float], *args, **kwargs):
    ax.annotate(text, pos, *args, xytext=xytext, textcoords="offset points", arrowprops=dict(linestyle=":", arrowstyle="-"), **kwargs)


def plot_meta(df: pd.DataFrame) -> None:
    ax: plt.Axes

    with subplot() as (_fig, ax):
        ax.hist(df["age"], bins="auto")
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


def split_rounds(df: pd.DataFrame, mask: Optional[pd.Series] = None) -> Tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    if mask is not None:
        df = df[mask]

    baseline = df[BASELINE_LABEL]
    rounda: pd.DataFrame = df[ROUND_A_LABELS].rename(columns=COLUMN_RENAME_MAPPING)
    roundb: pd.DataFrame = df[ROUND_B_LABELS].rename(columns=COLUMN_RENAME_MAPPING)

    all_rounds: pd.DataFrame = pd.concat([ rounda, roundb ]).reset_index(drop=True)
    all_rounds.insert(0, BASELINE_LABEL, pd.concat([ baseline, baseline ]).reset_index(drop=True))

    return rounda, roundb, all_rounds


def plot_scenarios(df: pd.DataFrame) -> None:  # pylint: disable=too-many-statements
    df_full: pd.DataFrame = df
    df = df.drop(columns=META_COLUMNS)

    rounda: pd.DataFrame = df[ROUND_A_LABELS].rename(columns=COLUMN_RENAME_MAPPING)
    roundb: pd.DataFrame = df[ROUND_B_LABELS].rename(columns=COLUMN_RENAME_MAPPING)

    rounda, roundb, all_rounds = split_rounds(df)
    all_means: pd.Series = all_rounds.mean()
    all_medians: pd.Series = all_rounds.median()

    fig: plt.Figure
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
        ax.bar(all_means.index, all_means, yerr=confidence_interval(all_rounds))
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
        plot_multi_bar(ax, rounda.mean(), roundb.mean(), SINGLE_SCENARIO_LABELS, a_yerr=confidence_interval(rounda), b_yerr=confidence_interval(roundb))
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

    with subplot(plot_width=10) as (_fig, ax):
        platformer_mask = df_full["genres"].apply(lambda x: bool(x & BITFLAG_PLATFORMER))
        _, _, platformers = split_rounds(df_full, platformer_mask)
        _, _, non_platformers = split_rounds(df_full, ~platformer_mask)
        plot_multi_bar(ax, platformers.mean(), non_platformers.mean(), platformers.columns, a_yerr=confidence_interval(platformers), b_yerr=confidence_interval(non_platformers))
        ax.set_title("Mean rating - Comparison players preferring Platformers vs others")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Mean rating")
        ax.set_yticks(range(6))
        ax.legend(["Platformer", "Non-Platformer"])

    with subplot(plot_width=10) as (_fig, ax):
        time_mask = df_full["time"] == 5
        _, _, a = split_rounds(df_full, time_mask)
        _, _, b = split_rounds(df_full, ~time_mask)
        plot_multi_bar(ax, a.mean(), b.mean(), a.columns, a_yerr=confidence_interval(a), b_yerr=confidence_interval(b))
        ax.set_title("Mean rating - Comparison 17+ hours players vs others")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Mean rating")
        ax.set_yticks(range(6))
        ax.legend(["17+ hours", "0-16 hours"])

    with subplot(plot_width=10) as (_fig, ax):
        time_mask = df_full["time"] == 1
        _, _, a = split_rounds(df_full, time_mask)
        _, _, b = split_rounds(df_full, ~time_mask)
        plot_multi_bar(ax, a.mean(), b.mean(), a.columns, a_yerr=confidence_interval(a), b_yerr=confidence_interval(b))
        ax.set_title("Mean rating - Comparison 0-4 hours players vs others")
        ax.set_xlabel("Scenario")
        ax.set_ylabel("Mean rating")
        ax.set_yticks(range(6))
        ax.legend(["0-4 hours", "5+ hours"])

    with subplot(plot_height=10) as (fig, ax):
        grid = pd.concat([ split_rounds(df_full, df_full["time"] == i)[2].mean() for i in range(1, 6) ], axis=1)
        pos = ax.imshow(grid)
        ax.set_title("Mean rating per average time spent on gaming")
        ax.set_xlabel("Hours spent on gaming per week")
        ax.set_ylabel("Scenario")
        ax.set_xticks(range(5))
        ax.set_yticks(range(len(grid.index)))
        ax.set_xticklabels(map(lambda x: x.replace(" hours", ""), TIME_SPENT))
        ax.set_yticklabels(grid.index)
        fig.colorbar(pos, ax=ax)
        ax.grid(visible=False)

        for x, cname in enumerate(grid):
            col = grid[cname]
            for y, value in enumerate(col):
                ax.text(x, y, f"{value:.2f}", ha="center", va="center")


def main() -> int:
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", help="Input CSV")
    parser.add_argument("--output", "-o", help="Output PDF")
    args = parser.parse_args()

    df: pd.DataFrame = pd.read_csv(args.csv)

    style_setup()

    plot_meta(df)
    plot_scenarios(df)

    if args.output:
        plot_to_pdf(args.output)
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main())
