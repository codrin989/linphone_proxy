#!/usr/bin/env python

import os
import sys
import shutil

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

def criu(cmd):
	rc = os.system(cmd)
	if rc != 0:
		print "error running criu: " + cmd
		sys.exit(1)

usage = "./proxy_manager.py <VNC_PID> <TRUDY_IP>"

if len(sys.argv) < 3:
	print "usage: " + usage
	sys.exit(1)

vnc_pid = sys.argv[1]
trudy_ip = sys.argv[2]

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

criu("criu dump --tree " + vnc_pid + " --images-dir images/0 --tcp-established --leave-running --track-mem")
criu("criu dump --tree " + vnc_pid + " --images-dir images/1 --tcp-established --leave-running --track-mem --prev-images-dir images/0")
criu("criu dump --tree " + vnc_pid + " --images-dir images/2 --tcp-established --leave-running --track-mem --prev-images-dir images/1")
criu("criu dump --tree " + vnc_pid + " --images-dir images/3 --tcp-established --leave-running --track-mem --prev-images-dir images/2")



print "hello"
