#!/usr/bin/env python

import os
import sys
import shutil
import socket

MAX_FRAME_SIZE = 1024
MANAGER_PORT = 52123
PROXY_PORT = 52124
STRUCT_SIZE = 10

def create_dir(path):
    try:
		os.makedirs(path)
    except OSError:
		 print "error creating: " + path

def delete_dir(path):
	try:
		shutil.rmtree(path)
	except:
		print "error deleting: " + path

def sys_cmd(cmd):
	rc = os.system(cmd)
	if rc != 0:
		print "error running: " + cmd
		sys.exit(1)

usage = "./proxy_manager.py <VNC_PID> <TRUDY_IP>"

if len(sys.argv) < 3:
	print "usage: " + usage
	sys.exit(1)

vnc_pid = sys.argv[1]
#trudy_ip = sys.argv[2]
trudy_ip = "localhost"
local_addr = ("localhost", PROXY_PORT)
remote_addr = (trudy_ip, PROXY_PORT)

# check: VNC_PID is running
output = os.popen("ps -e | grep " + vnc_pid)
count = len(output.readlines())
if count < 1:
	print "VNC_PID not found"
	sys.exit(1)

# check: TRUDY_IP is valid
# TODO

# create dirs
delete_dir("images")	
create_dir("images/0")
create_dir("images/1")
create_dir("images/2")
create_dir("images/3")

sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/0 --tcp-established --leave-running --track-mem")
sys_cmd("scp -r images codrin@" + trudy_ip + ":~")

sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/1 --tcp-established --leave-running --track-mem --prev-images-dir images/0")
sys_cmd("scp -r images/1 codrin@" + trudy_ip + ":~/images")

sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/2 --tcp-established --leave-running --track-mem --prev-images-dir images/1")
sys_cmd("scp -r images/2 codrin@" + trudy_ip + ":~/images")

sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/3 --tcp-established --leave-running --track-mem --prev-images-dir images/2")
sys_cmd("scp -r images/3 codrin@" + trudy_ip + ":~/images")

"""
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("", MANAGER_PORT))

s.sendto("Stop", local_addr)

msg, addr = s.recvfrom(MAX_FRAME_SIZE)
print "received " + len(msg) + " bytes: " + msg
if not msg.startswith("Ack"):
	print "wrong message"
	sys.exit(1)

buffer = bytearray(MAX_FRAME_SIZE)
n, addr = s.recvfrom_into(buffer, MAX_FRAME_SIZE)
print "received: " + n + "bytes"
if not n == STRUCT_SIZE:
	print "wrong structure size"
	sys.exit(1)
"""

sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/4 --tcp-established --track-mem --prev-images-dir images/3")
sys_cmd("scp -r images/4 codrin@" + trudy_ip + ":~/images")

"""
s.sendto("TCP repair", remote_addr)
s.sendto(buffer[:n], remote_addr)

msg, addr = s.recvfrom(MAX_FRAME_SIZE)
print "received " + len(msg) + " bytes: " + msg
if not msg.startswith("Ack"):
	print "wrong message"
	sys.exit(1)
"""

sys_cmd("ssh codrin@" + remote_addr +" 'criu restore --images-dir ~/images/4 --tcp-established -d'")

"""
s.sendto("IP: " + trudy_ip, remote_addr)

msg, addr = s.recvfrom(MAX_FRAME_SIZE)
print "received " + len(msg) + " bytes: " + msg
if not msg.startswith("Ack"):
	print "wrong message"
	sys.exit(1)
"""

print "finished migration"
