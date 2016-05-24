#!/bin/bash
#===============================================================================
#
#          FILE:  ex04.sh
# 
#         USAGE:  ./ex04.sh 
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
#       CREATED:  2012年06月15日 15时04分36秒 CST
#      REVISION:  ---
#===============================================================================

conf_file=./conf_file
proc_file=/proc/huaRain/whiteList

if [ $# -eq 2 ]; then
   conf_file="$1"
   proc_file="$2"
fi

echo 1 > /proc/huaRain/devNodeFromWhite 

while read line  
do  
    # n=`expr $n + 1`  
    # echo -e "$n/t$line"  
    echo -e ${line} > ${proc_file}
done < ${conf_file}

echo 1 > /proc/huaRain/whiteListEnable
echo 1 > /proc/huaRain/hostEnable
