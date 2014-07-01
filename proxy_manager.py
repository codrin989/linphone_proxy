#!/usr/bin/env python

import os
import sys
import shutil
import socket
import datetime

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
	print cmd
	rc = os.system(cmd)
	if rc != 0:
		print "... error"
		sys.exit(1)

		

usage = "./proxy_manager.py <VNC_PID> <TRUDY_IP>"

if len(sys.argv) < 3:
	print "usage: " + usage
	sys.exit(1)

vnc_pid = sys.argv[1]
trudy_ip = sys.argv[2]
#trudy_ip = "localhost"
local_addr = ("localhost", PROXY_PORT)
remote_addr = (trudy_ip, PROXY_PORT)
print "generating map ..."
print "disabling criugenics ..."
print "distributing enemies ..."
print "unlocking portals ..."
print "activating mines ..."
print "restarting at checkpoint ..."
print "[MIGRATION STARTED]"

print ""
# check: VNC_PID is running
print "checking PID " + vnc_pid + "..."
output = os.popen("ps -e | grep " + vnc_pid)
count = len(output.readlines())
if count < 1:
	print "VNC_PID not found"
	sys.exit(1)
print "OK"

# check: TRUDY_IP is valid
# TODO
print "checking IP " + trudy_ip + "..."
print "OK"

print ""
# create dirs
delete_dir("images")
create_dir("images/0")
print "created images/0"
create_dir("images/1")
print "created images/1"
create_dir("images/2")
print "created images/2"
create_dir("images/3")
print "created images/3"
create_dir("images/4")
print "created images/4"

print ""
print "CRIU initial dump:"
t1 = datetime.datetime.now()
sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/0 --tcp-established --leave-running --track-mem")
sys_cmd("scp -r images codrin@" + trudy_ip + ":~ > /dev/null")
t2 = datetime.datetime.now()
dt = t2 - t1
print "... " + str(dt)

print ""
print "CRIU incremental dump 1:"
t1 = datetime.datetime.now()
sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/1 --tcp-established --leave-running --track-mem --prev-images-dir ../0")
sys_cmd("scp -r images/1 codrin@" + trudy_ip + ":~/images > /dev/null")
t2 = datetime.datetime.now()
dt = t2 - t1
print "... " + str(dt)

print ""
print "CRIU incremental dump 2:"
t1 = datetime.datetime.now()
sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/2 --tcp-established --leave-running --track-mem --prev-images-dir ../1")
sys_cmd("scp -r images/2 codrin@" + trudy_ip + ":~/images > /dev/null")
t2 = datetime.datetime.now()
dt = t2 - t1
print "... " + str(dt)

print ""
print "CRIU incremental dump 3:"
t1 = datetime.datetime.now()
sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/3 --tcp-established --leave-running --track-mem --prev-images-dir ../2")
sys_cmd("scp -r images/3 codrin@" + trudy_ip + ":~/images > /dev/null")
t2 = datetime.datetime.now()
dt = t2 - t1
print "... " + str(dt)

#"""
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("", MANAGER_PORT))

t9 = datetime.datetime.now()
s.sendto("Stop", local_addr)

print "Waiting for message from session proxy"
msg, addr = s.recvfrom(MAX_FRAME_SIZE)
print "received " + str(len(msg)) + " bytes: " + msg
if not msg.startswith("Ack"):
	print "wrong message"
	sys.exit(1)

print "copy TCP repair structure ..."
sys_cmd("scp /tmp/proxy_tcp_repair.dmp codrin@" + trudy_ip + ":/tmp/proxy_tcp_repair.dmp_out > /dev/null")
#"""

print ""
print "last CRIU incremental dump:"
t1 = datetime.datetime.now()
sys_cmd("criu dump --tree " + vnc_pid + " --images-dir images/4 --tcp-established --track-mem --prev-images-dir ../3")
t11 = datetime.datetime.now()
sys_cmd("scp -r images/4 codrin@" + trudy_ip + ":~/images > /dev/null")

t12 = datetime.datetime.now()
#"""
s.sendto("TCP repair", remote_addr)
#s.sendto(buffer[:n], remote_addr)

msg, addr = s.recvfrom(MAX_FRAME_SIZE)
print "received " + str(len(msg)) + " bytes: " + msg
if not msg.startswith("Ack"):
	print "wrong message"
	sys.exit(1)
#"""

print "CRIU restore:"
sys_cmd("ssh codrin@" + trudy_ip +" 'sudo criu restore --images-dir ~/images/4 --tcp-established -d'")
t2 = datetime.datetime.now()
dt = t2 - t1
print "... " + str(dt)

#"""
s.sendto("IP: " + trudy_ip, local_addr)

msg, addr = s.recvfrom(MAX_FRAME_SIZE)
print "received " + str(len(msg)) + " bytes: " + msg
if not msg.startswith("Ack"):
	print "wrong message"
	sys.exit(1)
#"""

t10 = datetime.datetime.now()
dt = t10 - t9

print "Total downtime..." + str(dt)

dt = t12 - t11
print "Copied time within downtime..." + str(dt)

print ""
print "ending migration ..."
print "VICTORY!"
