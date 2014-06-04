#!/bin/bash

USAGE="./iptables.sh <command> [<remote_ip>]"

if [ -z $1 ] ; then
    echo $USAGE
    exit 1
fi

case "$1" in
    start)
        if [ -z $2 ] ; then
            echo $USAGE
            exit 1
        fi
        REMOTE_IP=$2

        echo "start iptables ..."
		echo "remote IP: $REMOTE_IP"

        iptables -t nat -F OUTPUT
        iptables -t nat -F POSTROUTING

        iptables -t nat -A OUTPUT -p udp --sport 5060 -d $REMOTE_IP --dport 5060 -j DNAT --to 127.0.0.1:50616
        iptables -t nat -A POSTROUTING -p udp -d 127.0.0.1 --sport 50616 --dport 5060 -j SNAT --to-source=$REMOTE_IP:5060

        iptables -t nat -A OUTPUT -p udp --sport 7078 -d $REMOTE_IP --dport 7078 -j DNAT --to 127.0.0.1:51796
        iptables -t nat -A POSTROUTING -p udp -d 127.0.0.1 --sport 51796 --dport 7078 -j SNAT --to-source=$REMOTE_IP:7078
        ;;

    stop)
        echo "stop iptables ..."
        iptables -t nat -F OUTPUT
        iptables -t nat -F POSTROUTING
        ;;

    show)
		iptables -t nat -L -n -v
        ;;

    *)
        echo "unknown option"
        exit 1
esac

