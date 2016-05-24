#!/bin/sh

WORKDIR=/root/huarain/

/bin/echo $QUERY_STRING > /etc/net.conf.0
/bin/sed -i 's/&/\n/g' /etc/net.conf.0

ipString=`/bin/cat /etc/net.conf.0 | grep "IPADDR"`
ipString=${ipString#*=}
IPADDR=${ipString:-192.168.1.6}

ipString=`/bin/cat /etc/net.conf.0 | grep "NETMASK"`
ipString=${ipString#*=}
NETMASK=${ipString:-255.255.255.0}

ipString=`/bin/cat /etc/net.conf.0 | grep "GATEWAY"`
ipString=${ipString#*=}
GATEWAY=${ipString:-192.168.1.1}

ipString=`/bin/cat /etc/net.conf.0 | grep "MAC"`
ipString=${ipString#*=}
MAC=${ipString:-10:23:45:67:89:ab}

sed -i s/IPADDR=.*/IPADDR=$IPADDR/g /etc/net.conf
sed -i s/NETMASK=.*/NETMASK=$NETMASK/g /etc/net.conf
sed -i s/GATEWAY=.*/GATEWAY=$GATEWAY/g /etc/net.conf
# sed -i s/MAC=.*/MAC=$MAC/g /etc/net.conf

# /bin/ifconfig eth0 $IPADDR netmask $NETMASK
# /bin/route add default gw $GATEWAY dev eth0

echo "Content-type: text/html; charset=gb2312"
echo

/bin/echo done!
# /bin/cat index.html

exit 0

