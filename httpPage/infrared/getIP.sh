#!/bin/bash
#===============================================================================
#
#          FILE:  getIP.sh
# 
#         USAGE:  ./getIP.sh 
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
#       CREATED:  2014年11月11日 15时33分37秒 CST
#      REVISION:  ---
#===============================================================================

IP=`LC_ALL=C ifconfig  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`

echo $IP

