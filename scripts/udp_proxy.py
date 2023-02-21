#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import socket
import sys
import argparse
from queue import Empty, PriorityQueue
from dataclasses import dataclass, field
import multiprocessing.synchronize
from multiprocessing import Process, Pipe, Event
from multiprocessing.connection import Connection
from multiprocessing.managers import SyncManager
import numpy as np


# Provides a process safe PriorityQueue.
# See https://stackoverflow.com/questions/25324560/strange-queue-priorityqueue-behaviour-with-multiprocessing-in-python-2-7-6
class QueueManager(SyncManager):
    pass


QueueManager.register("PriorityQueue", PriorityQueue)  # pylint: disable=no-member


class UDPListener:
    def __init__(self, port: int, ip="localhost"):
        self._address = (ip, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def recv(self):
        return self.sock.recvfrom(4096 * 4)

    def listen(self) -> None:
        self.sock.bind(self._address)

    def close(self) -> None:
        self.sock.close()

    def __enter__(self):
        self.listen()
        return self

    def __exit__(self, _exc_type, _exc_value, _traceback):
        self.close()


class UDPSender:
    def __init__(self, port_to: int, ip="localhost"):
        self._address = (ip, port_to)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def send(self, data: bytes):
        self.sock.sendto(data, self._address)


class PacketQueue:
    @dataclass(order=True)
    class QueueData:
        target_time: float
        data: bytes = field(compare=False)

    def __init__(self, sender: UDPSender, delay_seconds: float, jitter_seconds: float):
        self._queue_manager = QueueManager()
        self._sender = sender
        self._delay_seconds = delay_seconds
        self._jitter_seconds = jitter_seconds
        self._process: Process
        self._quit_control: Connection
        self._queue_changed = Event()
        self._rng: np.random.Generator = np.random.default_rng()
        self._queue: PriorityQueue

    def start(self) -> None:
        self._queue_manager.start()
        self._queue: PriorityQueue = self._queue_manager.PriorityQueue()  # type: ignore  # pylint: disable=no-member

        self_pipe, thread_pipe = Pipe()
        self._quit_control = self_pipe

        self._queue_changed.clear()

        self._process = Process(target=self._worker, args=(self._queue, self._sender, thread_pipe, self._queue_changed))
        self._process.start()

    def stop(self) -> None:
        self._quit_control.send(True)
        self._process.join()
        self._queue_manager.shutdown()

    def add(self, data: bytes) -> None:
        self._queue.put(PacketQueue.QueueData(
            time.time() + self._delay_seconds + self._rng.random() * self._jitter_seconds,
            data
        ))

        # Message the consumer thread that the queue changed, since the new packet could be
        # scheduled earlier than what the consumer is currently waiting for.
        self._queue_changed.set()

    @staticmethod
    def _worker(queue: PriorityQueue, sender: UDPSender, quit_control: Connection, queue_changed: multiprocessing.synchronize.Event) -> None:
        while not quit_control.poll():
            queue_changed.clear()

            try:
                data = queue.get(block=True, timeout=1)
            except Empty:
                pass
            else:
                timediff = data.target_time - time.time()
                if timediff > 0:
                    if queue_changed.wait(timediff):
                        # If the queue changed meanwhile, put the current item back and retry
                        # This prevents cases where a new packet that is scheduled earlier than the
                        # current packet, hence would be sent too late.
                        # NOTE: This actually happens quite frequently. Maybe we can optimize it.
                        queue.put(data)
                        # print("New packet arrived -> Reschedule")
                        continue
                else:
                    print("Running late:", timediff)

                print(len(data.data))
                sender.send(data.data)

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, _exc_type, _exc_value, _traceback):
        self.stop()


def main() -> int:
    parser = argparse.ArgumentParser(description="UDP proxy")
    parser.add_argument("listen_port", type=int, help="Listen port")
    parser.add_argument("relay_port", type=int, help="Relay port port")
    parser.add_argument("-d", "--delay", type=float, default=0.0, help="Packet delay in seconds")
    parser.add_argument("-j", "--jitter", type=float, default=0.0, help="Random packet delay in seconds")
    args = parser.parse_args()

    sender = UDPSender(args.relay_port)
    print("UDP proxy started:", args.listen_port, "->", args.relay_port)
    print("Delay:", args.delay, "seconds")
    print("Jitter:", args.jitter, "seconds")

    with PacketQueue(sender, args.delay, args.jitter) as queue:
        with UDPListener(args.listen_port) as listener:
            while True:
                data, _addr = listener.recv()

                if not data:
                    break

                queue.add(data)

    return 0


if __name__ == "__main__":
    sys.exit(main())
