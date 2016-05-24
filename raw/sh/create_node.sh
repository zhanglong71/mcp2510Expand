#!/bin/bash
#===============================================================================
#
#          FILE:  create_node.sh
# 
#         USAGE:  ./create_node.sh 
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
#       CREATED:  2014年05月21日 11时40分55秒 CST
#      REVISION:  ---
#===============================================================================

#funcName -a address -s AdapterNo -t DevType

./app_raw_ioctl -a 010f -s 2 -t 2
./app_raw_ioctl -a 020f -s 2 -t 2
./app_raw_ioctl -a 030f -s 2 -t 2
./app_raw_ioctl -a 040f -s 2 -t 2

./app_raw_ioctl -a 100f -s 3 -t 3

