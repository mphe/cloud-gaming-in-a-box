#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from matplotlib.backends.backend_pdf import PdfPages
from matplotlib import pyplot as plt
from matplotlib.ticker import MaxNLocator
import pandas as pd
import sys
from contextlib import contextmanager
from typing import Tuple, cast

META_COLUMNS = ["age", "gender", "time", "genres", "devices"]
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


def plot_bar(ax: plt.Axes, col: pd.Series, labels: Tuple[str, ...]):
    counts: pd.Series = col.value_counts().sort_index()
    values = [ counts.get(i + 1) or 0 for i in range(len(labels)) ]
    ax.bar(labels, values)
    ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel("Frequency")


def plot_bar_bitflags(ax: plt.Axes, col: pd.Series, labels: Tuple[str, ...]):
    values = cast(pd.Series, sum(map(lambda flags: parse_bitflags(flags, len(labels)), col)))
    ax.bar(labels, values)
    ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel("Frequency")


def parse_bitflags(flags: int, num_labels: int) -> pd.Series:
    return pd.Series([ int(bool(flags & (1 << i))) for i in range(num_labels) ])


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
        plot_bar(ax, df["gender"], GENDERS)
        ax.set_title("Gender distribution")

    with subplot() as (_fig, ax):
        plot_bar(ax, df["time"], TIME_SPENT)
        ax.set_title("Average time spent on gaming per week")

    with subplot(plot_width=10, plot_height=5) as (_fig, ax):
        plot_bar_bitflags(ax, df["genres"], GENRES)
        ax.set_title("Distribution of preferred game genres")
        plt.setp(ax.get_xticklabels(), rotation=30, horizontalalignment='right')  # type: ignore

    with subplot() as (_fig, ax):
        plot_bar_bitflags(ax, df["devices"], DEVICES)
        ax.set_title("Distribution of most used devices for gaming")


def plot_scenarios(df: pd.DataFrame) -> None:
    df = df.drop(columns=META_COLUMNS)


def main() -> int:
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", help="Input CSV")
    parser.add_argument("--output", "-o", help="Output PDF")
    args = parser.parse_args()

    df: pd.DataFrame = pd.read_csv(args.csv)

    plot_meta(df)
    plot_scenarios(df)

    if args.output:
        plot_to_pdf(args.output)
    else:
        plt.show()

    return 0


if __name__ == "__main__":
    sys.exit(main())


# x: delay
# y: loss
# 1-5 farben
# cluster
