#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from functools import reduce
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
import tkinter.simpledialog
import tkinter.messagebox
from pathlib import Path



@dataclass(frozen=True)
class UserSurvey:
    age: int
    gender: int
    time_spent: int
    genres: int
    devices: int


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


SELF_DIR = Path(sys.argv[0]).absolute().parent
PROJECT_ROOT = SELF_DIR.parent
ALARM_PATH = SELF_DIR / "alarm.ogg"

DISPLAY = ":0"
PREPARE_DURATION_SECONDS = 60 * 5
SCENARIO_DURATION_SECONDS = 60 * 1
SCENARIO_BASELINE = Scenario("B")
SCENARIOS = [
    Scenario("D1", rtt_ms=50),
    Scenario("D2", rtt_ms=100),
    Scenario("D3", rtt_ms=200),
    Scenario("L1", loss_start=0.0005, loss_stop=0.9995),
    Scenario("L2", loss_start=0.001, loss_stop=0.999),
    Scenario("L3", loss_start=0.01, loss_stop=0.99),
    Scenario("M1", rtt_ms=50, loss_start=0.001, loss_stop=0.999),
    Scenario("M2", rtt_ms=100, loss_start=0.001, loss_stop=0.999),
    Scenario("M3", rtt_ms=200, loss_start=0.001, loss_stop=0.999),
]
BACKEND_COMMAND = [ f"DISPLAY={DISPLAY}", str(PROJECT_ROOT / "cfg/proton.sh"), str(PROJECT_ROOT / "run.sh"), str(PROJECT_ROOT / "apps/hatintime.sh"), "app,stream", ]
FRONTEND_COMMAND = [ f"DISPLAY={DISPLAY}", str(PROJECT_ROOT / "cfg/vsync.sh"), str(PROJECT_ROOT / "run.sh"), "", "proxy,syncinput,frontend", ]


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


def list_to_bitflags(seq: List[bool]) -> int:
    return reduce(lambda a, b: a | b, (v << i for i, v in enumerate(seq)), 0)


def prepare_tk_win(title: str, description: str = "") -> Tuple[tkinter.Tk, tkinter.Frame]:
    window = tkinter.Tk()
    window.title(title)
    window.protocol("WM_DELETE_WINDOW", lambda: None)
    # window.resizable(False, False)
    frame = tkinter.Frame(window, borderwidth=10)
    frame.pack()
    tkinter.Label(frame, text=description or title).pack(anchor=tkinter.W, expand=True)
    return window, frame


def center(win: tkinter.Tk):
    win.update_idletasks()
    win.eval('tk::PlaceWindow . center')


def ask_rating(title: str, options: Tuple[str, ...], description: str = "", horizontal: bool = True) -> int:
    font = ("Arial", 20)

    window, rootframe = prepare_tk_win(title, description)
    frame = tkinter.Frame(rootframe)
    frame.pack(expand=True)
    rating = tkinter.IntVar()

    for value, text in enumerate(options, 1):
        if horizontal:
            tkinter.Radiobutton(frame,
                                text=text,
                                font=font,
                                value=value,
                                width=10,
                                height=5,
                                variable=rating,
                                indicatoron=False).pack(side=tkinter.LEFT, expand=True)
        else:
            tkinter.Radiobutton(frame,
                                text=text,
                                value=value,
                                variable=rating).pack(anchor=tkinter.W, expand=True)

    def on_submit():
        if rating.get() > 0 and rating.get() <= 5:
            window.destroy()

    if horizontal:
        tkinter.Button(rootframe, text="Submit", font=font, command=on_submit).pack(side=tkinter.BOTTOM, expand=True)
    else:
        tkinter.Button(rootframe, text="Submit", command=on_submit).pack(expand=True)

    center(window)
    window.mainloop()
    return rating.get()


def ask_choose(title: str, options: Tuple[str, ...], description: str = "") -> List[bool]:
    window, frame = prepare_tk_win(title, description)
    values = []

    for option in options:
        result = tkinter.IntVar(window, 0)
        values.append(result)
        tkinter.Checkbutton(frame, text=option, variable=result).pack(anchor=tkinter.W, expand=True)

    tkinter.Button(frame, text="Submit", command=window.destroy).pack(expand=True)

    center(window)
    window.mainloop()
    return [ bool(i.get()) for i in values ]


def ask_mos(title: str) -> int:
    return ask_rating(title, (
        "Bad\n1",
        "Poor\n2",
        "Fair\n3",
        "Good\n4",
        "Excellent\n5",
    ))


def ask_survey() -> UserSurvey:
    age = tkinter.simpledialog.askinteger("Survey", "Age")

    gender = ask_rating("Survey", (
        "Female",
        "Male",
        "Diverse",
    ), description="Gender", horizontal=False)

    time_spent = ask_rating("Survey", (
        "0-4 hours",
        "5-8 hours",
        "9-12 hours",
        "13-16 hours",
        "17 or more hours",
    ), description="Rate your average time spent on gaming per week", horizontal=False)

    genres = ask_choose("Survey", (
        "Action",
        "Adventure",
        "Battle royale",
        "Bullet hell",
        "Card game",
        "Construction and management simulation",
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
    ), description="Choose your most played genres")

    devices = ask_choose("Survey", (
        "PC",
        "Console (Playstation, Xbox, ...)",
        "Hand-held (Switch, Steam Deck, ...)",
        "Mobile",
    ), description="Which devices do regularily play on")

    tkinter.messagebox.showinfo("Survey", "Thank you!\n\nPress OK to continue with the preparation phase.")

    return UserSurvey(age, gender, time_spent, list_to_bitflags(genres), list_to_bitflags(devices))


def open_timer_window(seconds: int) -> None:
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


def run_scenario(scenario: Scenario, duration_seconds: float) -> None:
    # Use process groups to be able to kill the process with all its children.
    # See https://stackoverflow.com/a/4791612/4778400
    with run_subprocess([ "env", *scenario.to_env_list(), *FRONTEND_COMMAND, ]) as p:
        try:
            try:
                time.sleep(duration_seconds - 5)
                os.spawnlp(os.P_NOWAIT, "paplay", "paplay", ALARM_PATH)
                time.sleep(5)
                # open_timer_window(5)

            except KeyboardInterrupt:
                print("Sleep aborted")

        finally:
            print("Sending SIGINT, waiting to exit...")
            kill_subprocess(p)


def run_subprocess(args: List[str]) -> subprocess.Popen:
    return subprocess.Popen(args, preexec_fn=os.setsid, stdout=subprocess.DEVNULL)  # pylint: disable=subprocess-popen-preexec-fn


def kill_subprocess(p: subprocess.Popen, sig=signal.SIGINT) -> None:
    os.killpg(os.getpgid(p.pid), sig)
    p.wait()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("-u", "--user", help="User ID that is also used as seed for randomization.")
    parser.add_argument("-o", "--offset", type=int, help="Scenario offset where to start")
    args = parser.parse_args()

    # Execute in project root
    os.chdir(PROJECT_ROOT)

    userid = str(args.user if args.user else time.time())
    random.seed(userid)
    print("User ID/Seed:", userid)

    scenarios = get_scenarios()
    print("Scenarios:", ", ".join(map(str, scenarios)))

    offset = args.offset if args.offset else -1
    append_mode = offset > 0

    if append_mode:
        scenarios = scenarios[offset:]

    os.environ["DISPLAY"] = DISPLAY

    with open(f"results_{userid}.csv", "a" if append_mode else "w") as f:
        if not append_mode:
            # f.write("userid,scenario,rating,skill,playtime\n")
            f.write("key,value\n")

        survey = ask_survey()
        f.write(f"userid,{userid}\n")
        f.write(f"age,{survey.age}\n")
        f.write(f"gender,{survey.gender}\n")
        f.write(f"time,{survey.time_spent}\n")
        f.write(f"genres,{survey.genres}\n")
        f.write(f"devices,{survey.devices}\n")

        # subprocess.call(SELF_DIR / "hatintime/copy_savegame.sh")

        with run_subprocess([ "env", *BACKEND_COMMAND ]) as p_backend:
            try:
                if offset < 0:
                    print("Running preparation with baseline scenario")
                    offset = 0
                    run_scenario(SCENARIO_BASELINE, PREPARE_DURATION_SECONDS)
                    tkinter.messagebox.showinfo("Survey", "Preparation time ended.\n\nPress OK to continue with the test scenarios.")
                    # input("Press enter to start with testing...")

                for i, scenario in enumerate(scenarios, offset):
                    print(f"Running scenario {scenario.name} ({i})")
                    run_scenario(scenario, SCENARIO_DURATION_SECONDS)

                    rating = ask_mos("Rate your experience")
                    f.write(f"scenario_{scenario},{rating}\n")
                    # f.write(f"{userid},{scenario},{rating},{user_skill},{user_playtime}\n")

                tkinter.messagebox.showinfo("Survey", "Done.\n\nThank you for participating!")
                print("Done")
            finally:
                print("Terminating backend, waiting to exit...")
                kill_subprocess(p_backend)
                p_backend.terminate()
                p_backend.wait()

    return 0


if __name__ == "__main__":
    sys.exit(main())
