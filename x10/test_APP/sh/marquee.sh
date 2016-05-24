#!/bin/bash
#===============================================================================
#
#          FILE:  marquee.sh
# 
#         USAGE:  ./marquee.sh 
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
#       CREATED:  2013年01月12日 14时42分36秒 CST
#      REVISION:  ---
#===============================================================================

while true
do
######################################
## all on
./app_x10 /dev/xlight-1-1 3
sleep 5
## all off
./app_x10 /dev/xlight-1-1 1
sleep 5

## on==>off one-by-one
./app_x10 /dev/xlight-1-1 5
sleep 2
./app_x10 /dev/xlight-1-1 7

./app_x10 /dev/xlight-2-2 5
sleep 2
./app_x10 /dev/xlight-2-2 7

./app_x10 /dev/xlight-3-3 5
sleep 2
./app_x10 /dev/xlight-3-3 7

###############################################################################
## light-1 on
./app_x10 /dev/xlight-1-1 5
## light-1 dim
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-1-1 9
i=`expr $i - 1`
done

## light bright
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-1-1 11
i=`expr $i - 1`
done
## light-1 off
./app_x10 /dev/xlight-1-1 7
######################################
## light-2 on
./app_x10 /dev/xlight-2-2 5
## light-2 dim
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-2-2 9
i=`expr $i - 1`
done

## light bright
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-2-2 11
i=`expr $i - 1`
done
## light-2 off
./app_x10 /dev/xlight-2-2 7
######################################
## light-3 on
./app_x10 /dev/xlight-3-3 5
## light-3 dim
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-3-3 9
i=`expr $i - 1`
done

## light bright
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-3-3 11
i=`expr $i - 1`
done
## light-3 off
./app_x10 /dev/xlight-3-3 7
sleep 5

######################################
## all on
./app_x10 /dev/xlight-3-3 3
## turn off one-by-one
## xlight-3-3
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-3-3 9
##sleep 1
i=`expr $i - 1`
done
./app_x10 /dev/xlight-3-3 7

## xlight-2-2
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-2-2 9
i=`expr $i - 1`
done
./app_x10 /dev/xlight-2-2 7

## xlight-1-1
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-1-1 9
i=`expr $i - 1`
done
./app_x10 /dev/xlight-1-1 7
sleep 5
######################################
## turn on one-by-one
./app_x10 /dev/xlight-1-1 5
##sleep 2
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-1-1 11
i=`expr $i - 1`
done

./app_x10 /dev/xlight-2-2 5
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-2-2 11
i=`expr $i - 1`
done

./app_x10 /dev/xlight-3-3 5
i=16
while [ $i -gt 0 ]
do
./app_x10 /dev/xlight-3-3 11
i=`expr $i - 1`
done

######################################
## turn off one-by-one
./app_x10 /dev/xlight-3-3 7
./app_x10 /dev/xlight-2-2 7
./app_x10 /dev/xlight-1-1 7
sleep 3

######################################

done

