#!/usr/bin/python

import struct
import time
import traceback

import serial
from cobs import cobs

#SERIAL_PATH = "/dev/cu.usbmodem62709101"
# SERIAL_PATH = "/dev/ttyACM0"
SERIAL_PATH = "/dev/ttyUSB0"

device = serial.Serial(port=SERIAL_PATH, baudrate=115200, timeout=.1)
time.sleep(2)
if device.isOpen():
    print("Port Open")
else:
    print("Port Open failed")
    exit(-1)

device.timeout = 1


def calculateCrc(bytes: bytearray) -> int:
    return (sum(send_buffer_msgpack_packed_payload) % 0x100)


print_padding = 30


SECOUND = 1000000

device.write(b"\x00")

send_buffer_msgpack_packed = b"\x00\x03\x0A"+b"\x03\x03\x04"
delay_us: int = 0 * SECOUND
pulse_limit: int = 0
pulse_hz: int = 0
send_buffer_msgpack_packed_payload = struct.pack(
    ">BII",  pulse_hz, pulse_limit, delay_us)
crc = calculateCrc(send_buffer_msgpack_packed_payload)
send_buffer_msgpack_packed_head = struct.pack(
    ">BBB", 1, struct.calcsize(">IIB"), crc)
send_buffer_msgpack_packed = send_buffer_msgpack_packed_head + \
    send_buffer_msgpack_packed_payload
print("send_buffer_structpack_packed:".ljust(print_padding, " ") +
      str(send_buffer_msgpack_packed.hex(" ").upper()))
send_buffer_cobs_encoded = cobs.encode(send_buffer_msgpack_packed)
print("send_buffer_cobs_encoded:".ljust(print_padding, " ") +
      str(send_buffer_cobs_encoded.hex(" ").upper()))
send_buffer_raw_frame = send_buffer_cobs_encoded + b'\x00'
print("send_buffer_raw_frame:".ljust(print_padding, " ") +
      str(send_buffer_raw_frame.hex(" ").upper()))

# serial
device.write(send_buffer_raw_frame)
recv_buffer_raw_frame = b''
for i in range(10):
    recv_buffer_raw_frame = device.read_until(b'\x00')
    if(len(recv_buffer_raw_frame) == 0):
        continue
    print("# RECV: "+str(i))
    print("recv_buffer_raw_frame:".ljust(print_padding, " ") +
          str(recv_buffer_raw_frame.hex(" ").upper()))
    print("recv_buffer_raw_frame str:".ljust(print_padding, " ") +
          str(recv_buffer_raw_frame))
    # serial
    try:
        recv_buffer_cobs_encoded = recv_buffer_raw_frame[:-1]
        print("recv_buffer_cobs_encoded:".ljust(print_padding, " ") +
              str(recv_buffer_cobs_encoded.hex(" ").upper()))
        recv_buffer_cobs_decoded = cobs.decode(recv_buffer_cobs_encoded)
        print("recv_buffer_cobs_decoded:".ljust(print_padding, " ") +
              str(recv_buffer_cobs_decoded.hex(" ").upper()))
        print("recv_buffer_cobs_decoded str:".ljust(print_padding, " ") +
              str(recv_buffer_cobs_decoded))
    except Exception:
        traceback.print_exc()


device.close()
