#!/bin/bash
#===============================================================================
#
#          FILE:  recover.sh
# 
#         USAGE:  ./recover.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR:  zhanglong (zl), zhanglong93@sohu.com
#       COMPANY:  GPL
#       VERSION:  1.0
#       CREATED:  2014年12月11日 13时56分17秒 CST
#      REVISION:  ---
#===============================================================================

echo "IPADDR=192.168.1.6" > /etc/net.conf
echo "NETMASK=255.255.255.0" >> /etc/net.conf
echo "GATEWAY=192.168.1.1" >> /etc/net.conf
echo "MAC=10:23:45:67:89:ab" >> /etc/net.conf
