ctrlc
===
Helper program to terminate processes gracefully.

I was making a GUI program in Python and wanted to run console apps without any window and terminate them gracefully. No big deal on Linux and Mac, you just send `SIGINT`. But Windows makes it hard. You need to create a subprocess with `CREATE_NEW_PROCESS_GROUP` flag and then you can send `CTRL_BREAK_EVENT`. But that event doesn't work with everything, you won't stop `ping` with it, you need `CTRL_C_EVENT`. But the default handler for that event is disabled when you use `CREATE_NEW_PROCESS_GROUP` flag. And if you add PyInstaller into the mix then even `CTRL_BREAK_EVENT` stops working (unless I missed some flag).

You can read more about that mess here:
https://bugs.python.org/issue33245
https://bugs.python.org/issue23948

Code for gracefully killing used in the example below and in `ctrlc` itself taken from:
https://stackoverflow.com/a/60795888/2428152

Below is the example use. `ctrlc.exe` is used only on Windows and if the code is frozen with PyInstaller.
```py
import os
import sys
import signal
import subprocess
import threading

from time import sleep
from functools import partial


def terminate(p):
    p.terminate()
    print("terminated", p.args)


if os.name == 'nt':
    DEFAULT_KWARGS = {
        "creationflags": subprocess.CREATE_NEW_CONSOLE,
        "startupinfo": subprocess.STARTUPINFO(
            dwFlags=subprocess.STARTF_USESHOWWINDOW, wShowWindow=subprocess.SW_HIDE
        ),
    }
else:
    DEFAULT_KWARGS = {}


if __name__ == "__main__":
    p = subprocess.Popen(
        [
            "aria2c",
            "-o",
            r"test.iso",
            "https://releases.ubuntu.com/20.04.3/ubuntu-20.04.3-desktop-amd64.iso"
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        **DEFAULT_KWARGS,
    )

    sleep(5)

    if os.name == 'nt':  # Windows
        if getattr(sys, 'frozen', False):  # when using PyInstaller
            command = ["ctrlc"]
        else:
            command = [
                sys.executable,
                "-c",
                "import ctypes, sys;"
                "kernel = ctypes.windll.kernel32;"
                "pid = int(sys.argv[1]);"
                "kernel.FreeConsole();"
                "kernel.AttachConsole(pid);"
                "kernel.SetConsoleCtrlHandler(None, 1);"
                "kernel.GenerateConsoleCtrlEvent(0, 0);"
                "sys.exit(0)",
            ]

        p2 = subprocess.Popen([*command, str(p.pid)], **DEFAULT_KWARGS)

        watchdog = threading.Timer(5, partial(terminate, p2))
        watchdog.start()
        p2.wait()
        watchdog.cancel()
    else:  # Linux or Mac
        p.send_signal(signal.SIGINT)

    watchdog = threading.Timer(5, partial(terminate, p))
    watchdog.start()
    exitcode = p.wait()
    watchdog.cancel()

    print(exitcode)  # should be 7 if the download link is correct
    # save to file in case PyInstaller is used with `--noconsole`
    with open("exitcode.txt", "w") as f:
        f.write(str(exitcode))
```
