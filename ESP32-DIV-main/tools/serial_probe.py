"""Read a few seconds of serial output (used to check the boot log).

Usage: python tools/serial_probe.py COM5 [baud] [seconds]
"""
import sys
import time

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM5"
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 8.0

ser = serial.Serial(port, baud, timeout=1)
time.sleep(0.3)
ser.reset_input_buffer()
# DTR -> EN: pulse low then release to force a real reset.
ser.setDTR(True)
time.sleep(0.15)
ser.setDTR(False)

chunks = []
deadline = time.time() + seconds
while time.time() < deadline:
    chunks.append(ser.read(4096))
ser.close()

sys.stdout.write(b"".join(chunks).decode("utf-8", "replace"))
