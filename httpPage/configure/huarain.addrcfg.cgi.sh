#!/bin/sh

ipString=`/bin/cat net.conf.0 | grep "IPADDR"`
ipString=${ipString#*=}
IPADDR=${ipString:-192.168.1.6}

ipString=`/bin/cat net.conf.0 | grep "NETMASK"`
ipString=${ipString#*=}
NETMASK=${ipString:-255.255.255.0}

ipString=`/bin/cat net.conf.0 | grep "GATEWAY"`
ipString=${ipString#*=}
GATEWAY=${ipString:-192.168.1.1}

ipString=`/bin/cat net.conf.0 | grep "MAC"`
ipString=${ipString#*=}
MAC=${ipString:-10:23:45:67:89:ab}

echo "IPADDR = $IPADDR"
echo "NETMASK = $NETMASK"
echo "GATEWAY = $GATEWAY"
echo "MAC = $MAC"

sed -i s/IPADDR=.*/IPADDR=$IPADDR/g net.conf
sed -i s/NETMASK=.*/NETMASK=$NETMASK/g net.conf
sed -i s/GATEWAY=.*/GATEWAY=$GATEWAY/g net.conf
sed -i s/MAC=.*/MAC=$MAC/g net.conf

#IPADDR="192.168.1.6"
#NETMASK="255.255.255.0"
#GATEWAY="192.168.1.1"
#MAC="10:23:45:67:89:ab"

