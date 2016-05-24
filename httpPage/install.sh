#!/bin/bash
#===============================================================================
#
#          FILE:  install.sh
# 
#         USAGE:  ./install.sh 
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
#       CREATED:  2014年05月29日 15时02分39秒 CST
#      REVISION:  ---
#===============================================================================

cp -vr index.html ~/tftpboot/

cp -rv ./configure/addrcfg.html ~/tftpboot/
cp -rv ./configure/huarain.addrcfg.cgi ~/tftpboot/

cp -rv ./authentication/authen.html ~/tftpboot/
