#!/usr/bin/env python3
import socket
import sys

espwake_addr = sys.argv[1]
command = "status"

if len(sys.argv) > 2:
	command = sys.argv[2]

def reply_to_str(reply):
	r_b = reply[0]
	r_map = ["off", "powering off", "powering on", "on"]
	return r_map[r_b]

def command_byte(cstr):
	if cstr == "start":
		return 1
	elif cstr == "shutdown":
		return 2
	elif cstr == "forceoff":
		return 3
	else:
		return 0

sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
cmd_byte = command_byte(command)
cmd_payload = bytes([cmd_byte, 0, 0, 0])
sck.sendto(cmd_payload, (espwake_addr, 9999))

if cmd_byte == 0:
	reply, addr = sck.recvfrom(4)
	print(f"Received reply: {reply} ({reply_to_str(reply)})")
