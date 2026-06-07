#!/usr/bin/env python3
import socket
import sys

espwake_addr = sys.argv[1]
password = sys.argv[2]
espwake_port = 9999
command = "status"

if len(sys.argv) > 3:
	command = sys.argv[3]

def command_byte(cstr):
	if cstr == "start":
		return 1
	elif cstr == "shutdown":
		return 2
	elif cstr == "forceoff":
		return 3
	elif cstr == "status":
		return 0
	else:
		raise ValueError("Unknown command")

def map_reply_status(status):
	if status == 0:
		return "ok"
	elif status == 1:
		return "unauthorized"
	elif status == 2:
		return "incompatible_protocols"
	elif status == 3:
		return "unknown_command"
	else:
		return f"unknown_error({status})"

def map_reply_devstatus(devstatus):
	r_map = ["off", "powering off", "powering on", "on", "suspended"]
	return r_map[devstatus]

def make_request(cmd, passwd):
	assert len(passwd) < 16
	buffer = [0] * 40
	buffer[0] = (command_byte(command) << 4) | 1 # CMD=4b MSB | VER=4b LSB
	for i in range(len(passwd)):
		buffer[i+1] = ord(passwd[i])
	return bytes(buffer)

def read_reply(data):
	status = map_reply_status(data[0] >> 4)
	devstatus = map_reply_devstatus(data[1])
	return (status, devstatus)

def hexstring(data):
	return str(list(map(lambda v: hex(v), data)))

sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)

req = make_request(command, password)
#print(f"Sending {hexstring(req)}")
sck.sendto(req, (espwake_addr, espwake_port))
reply_data, addr = sck.recvfrom(4)

print(f"Received reply: {hexstring(reply_data)} {read_reply(reply_data)}")
