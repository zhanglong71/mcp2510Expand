#!/bin/sh

key="01"
house="0f"
devType="04"
function="00"

case $QUERY_STRING in
    *FFF00*)
        function="00"
        ;;
    *FFF01*)
        function="01"
        ;;
    *FFF02*)
        function="02"
        ;;
    *FFF03*)
        function="03"
        ;;
    *FFF04*)
        function="04"
        ;;
    *FFF05*)
        function="05"
        ;;
    *FFF06*)
        function="06"
        ;;
    *FFF07*)
        function="07"
        ;;
    *FFF08*)
        function="08"
        ;;
    *FFF09*)
        function="09"
        ;;
    *FFF10*)
        function="10"
        ;;
esac

WORKDIR=/root/huarain/
#${WORKDIR}/app_raw_write /dev/mcp2510 ${key}${house}${devType}000000000000004201${function}0000 4
${WORKDIR}/app_raw_write /dev/mcp2510 ${key}${house}${devType}000000000000004201${function}0000

# /root/app_raw_write /dev/mcp2510 010f00000000000000004201${function}0000 e

echo "Content-type: text/html; charset=gb2312"
echo

# /bin/cat goto2.html
# /bin/cat goto_IrChSelect.html
/bin/cat IrChSelect.html

exit 0

