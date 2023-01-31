#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import socket
import sys
import argparse
from queue import Empty
from dataclasses import dataclass
from multiprocessing import Queue, Process, Pipe
from multiprocessing.connection import Connection


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
    @dataclass
    class QueueData:
        target_time: float
        data: bytes

    def __init__(self, sender: UDPSender, delay_seconds: float):
        self._queue: Queue = Queue()
        self._sender = sender
        self._delay_seconds = delay_seconds
        self._process: Process
        self._quit_control: Connection

    def start(self) -> None:
        self_pipe, thread_pipe = Pipe()
        self._quit_control = self_pipe
        self._process = Process(target=self._worker, args=(self._queue, self._sender, thread_pipe))
        self._process.start()

    def stop(self) -> None:
        self._quit_control.send(True)
        self._process.join()

    def add(self, data: bytes) -> None:
        self._queue.put(PacketQueue.QueueData(time.time() + self._delay_seconds, data))

    @staticmethod
    def _worker(queue: Queue, sender: UDPSender, quit_control: Connection) -> None:
        while not quit_control.poll():
            try:
                data = queue.get(block=True, timeout=1)
            except Empty:
                pass
            else:
                timediff = data.target_time - time.time()
                if timediff > 0:
                    time.sleep(timediff)
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
    args = parser.parse_args()

    sender = UDPSender(args.relay_port)
    print("UDP proxy started:", args.listen_port, "->", args.relay_port)
    print("Delay:", args.delay, "seconds")

    with PacketQueue(sender, args.delay) as queue:
        with UDPListener(args.listen_port) as listener:
            while True:
                data, _addr = listener.recv()

                if not data:
                    break

                print(len(data))
                queue.add(data)

    return 0


if __name__ == "__main__":
    sys.exit(main())
