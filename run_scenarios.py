#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import sys
import subprocess
import signal
import time
from dataclasses import dataclass
import random
from typing import List, Tuple
import tkinter


@dataclass(frozen=True)
class Scenario:
    name: str
    loss_start: float = 0.0
    loss_stop: float = 1.0
    rtt_ms: int = 0

    def __str__(self) -> str:
        return self.name

    def to_env_list(self) -> Tuple[str, ...]:
        delay = self.rtt_ms // 2
        return (
            f"SERVER_DELAY_MS={delay}",
            f"SERVER_LOSS_START={self.loss_start}",
            f"SERVER_LOSS_STOP={self.loss_stop}",
            f"CLIENT_DELAY_MS={delay}",
            f"CLIENT_LOSS_START={self.loss_start}",
            f"CLIENT_LOSS_STOP={self.loss_stop}",
        )

    def to_env_str(self) -> str:
        return " ".join(self.to_env_list())


DISPLAY = ":0"
PREPARE_DURATION_SECONDS = 60 * 5
SCENARIO_DURATION_SECONDS = 60 * 1
SCENARIO_BASELINE = Scenario("B")
SCENARIOS = [
    # Scenario("D1", rtt_ms=50),
    # Scenario("D2", rtt_ms=100),
    # Scenario("D3", rtt_ms=200),
    Scenario("L1", loss_start=0.0005, loss_stop=0.9995),
    Scenario("L2", loss_start=0.001, loss_stop=0.999),
    # Scenario("M1", rtt_ms=50, loss_start=0.001, loss_stop=0.999),
    # Scenario("M2", rtt_ms=100, loss_start=0.001, loss_stop=0.999),
    # Scenario("M3", rtt_ms=200, loss_start=0.001, loss_stop=0.999),
]
BACKEND_COMMAND = [ "cfg/proton.sh", "./run.sh", "apps/hatintime.sh", "app,stream", ]
FRONTEND_COMMAND = [ f"DISPLAY={DISPLAY}", "cfg/vsync.sh", "./run.sh", "", "proxy,syncinput,frontend", ]


def ask_rating(prompt: str = "Rating", min_value: int = 1, max_value: int = 5) -> int:
    while True:
        rating = input(f"{prompt} ({min_value}-{max_value}): ")
        try:
            value = int(rating)
        except:  # pylint: disable=bare-except
            continue

        if value >= min_value and value <= max_value:
            return value


def ask_yesno(question: str) -> bool:
    while True:
        ans = input(f"{question}? (y/n): ").lower()

        if ans == "y":
            return True
        if ans == "n":
            return False


def get_scenarios() -> List[Scenario]:
    round1 = list(SCENARIOS)
    random.shuffle(round1)
    round1.append(SCENARIO_BASELINE)

    round2 = list(SCENARIOS)
    random.shuffle(round2)
    round1.extend(round2)
    return round1


def open_timer_window(seconds: int) -> None:
    os.environ["DISPLAY"] = DISPLAY
    font = ("Arial", 20)

    window = tkinter.Tk()
    # window.resizable(False, False)
    # window.wm_attributes('-type', 'splash')

    window.overrideredirect(True)
    width = 100
    height = 100
    screenw = window.winfo_screenwidth()
    screenh = window.winfo_screenheight()
    x = screenw - width
    y = screenh - height
    window.geometry(f"{width}x{height}+{x}+{y}")

    label = tkinter.Label(window, text="", font=font)
    label.place(relx=0.5, rely=0.5, anchor=tkinter.CENTER)

    def update_timer(value: int) -> None:
        if value <= 0:
            window.destroy()
            return
        label.config(text=str(value))
        window.after(1000, lambda: update_timer(value - 1))

    update_timer(seconds)
    window.mainloop()
    del os.environ["DISPLAY"]


def ask_rating_window() -> int:
    os.environ["DISPLAY"] = DISPLAY
    options = (
        ("Bad\n1", 1),
        ("Poor\n2", 2),
        ("Fair\n3", 3),
        ("Good\n4", 4),
        ("Excellent\n5", 5),
    )

    font = ("Arial", 20)

    window = tkinter.Tk()
    window.title("Rate your experience")
    window.protocol("WM_DELETE_WINDOW", lambda: None)
    window.resizable(False, False)
    window.eval('tk::PlaceWindow . center')
    frame = tkinter.Frame(window)
    frame.pack()
    rating = tkinter.IntVar()

    for text, value in options:
        tkinter.Radiobutton(frame,
                            text=text,
                            font=font,
                            value=value,
                            width=10,
                            height=5,
                            variable=rating,
                            indicatoron=False).pack(side=tkinter.LEFT, expand=True)

    def on_submit():
        if rating.get() > 0 and rating.get() <= 5:
            window.destroy()

    tkinter.Button(window, text="Submit", font=font, command=on_submit).pack(side=tkinter.BOTTOM, expand=True)

    window.mainloop()
    del os.environ["DISPLAY"]
    return rating.get()


def run_scenario(scenario: Scenario, duration_seconds: float) -> None:
    # Use process groups to be able to kill the process with all its children.
    # See https://stackoverflow.com/a/4791612/4778400
    with run_subprocess([ "env", *scenario.to_env_list(), *FRONTEND_COMMAND, ]) as p:
        try:
            try:
                time.sleep(duration_seconds - 3)
                os.spawnlp(os.P_NOWAIT, "paplay", "paplay", "alarm.ogg")
                time.sleep(3)
                # open_timer_window(5)

            except KeyboardInterrupt:
                print("Sleep aborted")

        finally:
            print("Sending SIGINT, waiting to exit...")
            kill_subprocess(p)


def run_subprocess(args: List[str]) -> subprocess.Popen:
    return subprocess.Popen(args, preexec_fn=os.setsid, stdout=subprocess.DEVNULL)


def kill_subprocess(p: subprocess.Popen, sig=signal.SIGINT) -> None:
    os.killpg(os.getpgid(p.pid), sig)
    p.wait()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("-u", "--user", help="User ID that is also used as seed for randomization.")
    parser.add_argument("-o", "--offset", type=int, help="Scenario offset where to start")
    args = parser.parse_args()

    userid = str(args.user if args.user else time.time())
    random.seed(userid)
    print("User ID/Seed:", userid)

    user_skill = ask_rating("Rate self-perceived skill level")
    user_playtime = ask_rating("Rate how much time spent on gaming")

    scenarios = get_scenarios()
    print("Scenarios:", ", ".join(map(str, scenarios)))

    offset = args.offset if args.offset else -1
    append_mode = offset > 0

    if append_mode:
        scenarios = scenarios[offset:]

    with open(f"results_{userid}.csv", "a" if append_mode else "w") as f:
        if not append_mode:
            f.write("userid,scenario,rating,skill,playtime\n")

        with run_subprocess([ "env", *BACKEND_COMMAND ]) as p_backend:
            try:
                if offset < 0:
                    print("Running preparation with baseline scenario")
                    offset = 0
                    run_scenario(SCENARIO_BASELINE, PREPARE_DURATION_SECONDS)
                    input("Press enter to start with testing...")

                for i, scenario in enumerate(scenarios, offset):
                    print(f"Running scenario {scenario.name} ({i})")
                    run_scenario(scenario, SCENARIO_DURATION_SECONDS)

                    rating = ask_rating_window()
                    f.write(f"{userid},{scenario},{rating},{user_skill},{user_playtime}\n")
                    # input("Press enter to continue")

                print("Done")
            finally:
                print("Terminating backend, waiting to exit...")
                kill_subprocess(p_backend)
                p_backend.terminate()
                p_backend.wait()

    return 0


if __name__ == "__main__":
    sys.exit(main())
