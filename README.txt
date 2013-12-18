1. Compile 
make

2. Run
./iptables.sh start <remote_ip>
./iptables.sh show
./linphone_proxy <remote_ip>

3. Close
killall linphone_proxy
./iptables.sh stop

4. Clean
make clean
