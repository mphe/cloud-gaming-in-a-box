#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import math
import pandas as pd
from contextlib import contextmanager
import matplotlib
from matplotlib import pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.patches as mpatches
from typing import Sequence, Union

SINGLE_SCENARIO_LABELS = [ "D1", "D2", "D3", "L1", "L2", "L3", "M1", "M2", "M3" ]


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


def style_setup():
    matplotlib.rcParams.update({
        "axes.grid": True,
        "axes.axisbelow": True,
        "grid.alpha": 1.0,
        "grid.linestyle": ":",
        "errorbar.capsize": 4,
    })


def boxplot(ax: plt.Axes, data, labels: Sequence[str], *args, **kwargs):
    ax.boxplot(data, *args, patch_artist=True, labels=labels, showmeans=True, meanline=True, meanprops=dict(linestyle="-", color="lime"), **kwargs)
    ax.legend(handles=[
        mpatches.Patch(linestyle="-", color='orange', label="Median"),
        mpatches.Patch(linestyle="-", color='lime', label="Mean"),
    ])


def confidence_interval(y: Union[pd.Series, pd.DataFrame]):
    if len(y) <= 1:
        if isinstance(y, pd.Series):
            return 0
        return pd.Series(0, index=y.columns)

    return 1.96 * y.std() / math.sqrt(len(y))


def bar_with_ci(ax: plt.Axes, data: Union[pd.Series, pd.DataFrame], labels: Sequence[str], *args, **kwargs):
    ax.bar(labels, data.mean(), *args, yerr=confidence_interval(data), **kwargs)


def plot_multi_bar_with_ci(ax: plt.Axes, a: pd.Series | pd.DataFrame, b: pd.Series | pd.DataFrame, labels: Sequence[str], width: float = 0.35, **kwargs) -> None:
    plot_multi_bar(ax, a.mean(), b.mean(), labels, width, a_yerr=confidence_interval(a), b_yerr=confidence_interval(b), **kwargs)


def plot_multi_bar(ax: plt.Axes, a: pd.Series, b: pd.Series, labels: Sequence[str], width: float = 0.35, a_yerr=None, b_yerr=None, **kwargs) -> None:
    ind = pd.Series(range(len(a)))
    ax.bar(ind, a, width, yerr=a_yerr, **kwargs)
    ax.bar(ind + width, b, width, yerr=b_yerr, **kwargs)
    ax.set_xticks(ind + width / 2)
    ax.set_xticklabels(labels)
