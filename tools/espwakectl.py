#!/usr/bin/env python3
import socket
import sys
import os
import getopt
import getpass

optlist, args = getopt.getopt(sys.argv[1:], "hvbP:p:", ["help", "verbose", "batch", "port", "password"])

def print_usage():
	print(f"Usage: {sys.argv[0]} [-h|--help] [-v|--verbose] [-b|--batch] [-P|--port <port>] [-p|--password <password>] <host> <command>")

def debug_print(*args):
	global verbose
	if not verbose:
		return
	print(*args)

password = os.environ.get('ESPWAKE_PASSWORD')
verbose = False
batch_mode = False
espwake_port = 9999

for k, v in optlist:
	if k in ("-h", "--help"):
		print_usage()
		sys.exit(0)
	elif k in ("-v", "--verbose"):
		verbose = True
	elif k in ("-b", "--batch"):
		batch_mode = True
	elif k in ("-P", "--port"):
		espwake_port = int(v)
	elif k in ("-p", "--password"):
		password = v
	else:
		raise RuntimeError(f"Unrecognized option {k}")

if len(args) < 1:
	print_usage()
	sys.exit(1)

if not password:
	if batch_mode:
		print("ERROR: Password not provided in batch mode", file=sys.stderr)
		sys.exit(1)
	password = getpass.getpass()

espwake_addr = args[0]
command = "status"

if len(args) >= 2:
	command = args[1]

def _espwake_map_command_byte(cstr):
	if cstr == "start":
		return 1
	elif cstr == "shutdown":
		return 2
	elif cstr == "forceoff":
		return 3
	elif cstr == "status":
		return 0
	else:
		raise ValueError(f"Unknown command \"{cstr}\"")

def _espwake_map_reply_status(status):
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

def _espwake_map_reply_devstatus(status):
	if status == 0:
		return "off"
	elif status == 1:
		return "powering_off"
	elif status == 2:
		return "powering_on"
	elif status == 3:
		return "on"
	elif status == 4:
		return "suspended"
	else:
		return f"unknown({devstatus:02x})"

def _espwake_make_request_payload(cmd, passwd):
	assert len(passwd) < 16
	buffer = [0] * 40
	buffer[0] = (_espwake_map_command_byte(command) << 4) | 1 # CMD=4b MSB | VER=4b LSB
	for i in range(len(passwd)):
		buffer[i+1] = ord(passwd[i])
	return bytes(buffer)

def _espwake_parse_reply_payload(data):
	status = _espwake_map_reply_status(data[0] >> 4)
	devstatus = _espwake_map_reply_devstatus(data[1])
	return (status, devstatus)

def hexstring(data):
	return str(list(map(lambda v: hex(v), data)))

class EspwakeClient:
	def __init__(self, host, port=9999):
		self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
		self.host = host
		self.port = port
		self.password = ""

	def _dest_sockaddr(self):
		return (self.host, self.port)

	def set_password(self, password):
		self.password = password

	def query(self, command):
		req = _espwake_make_request_payload(command, password)
		debug_print(f"Sending request payload {hexstring(req)}")
		self.socket.sendto(req, self._dest_sockaddr())
		reply_data, addr = self.socket.recvfrom(4)
		debug_print(f"Received payload: {hexstring(reply_data)}")
		return _espwake_parse_reply_payload(reply_data)

client = EspwakeClient(espwake_addr, espwake_port)
client.set_password(password)
status, powerstate = client.query(command)
print(f"{status} {powerstate}")
