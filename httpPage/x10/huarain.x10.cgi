#!/bin/sh

type=0
period=1

case $QUERY_STRING in
    *ping*)
        type=0
        ;;
    *counter*)
        type=1
        ;;
    *stop*)
        type=2
        ;;
esac

case $QUERY_STRING in
    *slow*)
        period=0.25
        ;;
    *normal*)
        period=0.125
        ;;
    *fast*)
        period=0.0625
        ;;
esac

/bin/echo $type $period > /tmp/led-control


key="01"
house="0f"
devType="02"
function="00"

case $QUERY_STRING in
    *000*)
        key="00"
        ;;
    *111*)
        key="01"
        ;;
    *222*)
        key="02"
        ;;
    *333*)
        key="03"
        ;;
    *444*)
        key="04"
        ;;
    *555*)
        key="05"
        ;;
    *666*)
        key="06"
        ;;
    *777*)
        key="07"
        ;;
    *888*)
        key="08"
        ;;
    *999*)
        key="09"
        ;;
    *aaa*)
        key="0a"
        ;;
    *bbb*)
        key="0b"
        ;;
    *ccc*)
        key="0c"
        ;;
    *ddd*)
        key="0d"
        ;;
    *eee*)
        key="0e"
        ;;
    *fff*)
        key="0f"
        ;;
esac

case $QUERY_STRING in
    *AAA*)
        house="00"
        ;;
    *BBB*)
        house="01"
        ;;
    *CCC*)
        house="02"
        ;;
    *DDD*)
        house="03"
        ;;
    *EEE*)
        house="04"
        ;;
    *FFF*)
        house="05"
        ;;
    *GGG*)
        house="06"
        ;;
    *HHH*)
        house="07"
        ;;
    *III*)
        house="08"
        ;;
    *JJJ*)
        house="09"
        ;;
    *KKK*)
        house="0a"
        ;;
    *LLL*)
        house="0b"
        ;;
    *MMM*)
        house="0c"
        ;;
    *NNN*)
        house="0d"
        ;;
    *OOO*)
        house="0e"
        ;;
    *PPP*)
        house="0f"
        ;;
    *allallall*)
        house="0f"
        ;;
esac

case $QUERY_STRING in
    *devType-zigbee*)
        devType="01"
        ;;
    *devType-x10x10x10*)
        devType="02"
        ;;
    *devType-lockBy315M*)
        devType="03"
        ;;
    *devType-infrared*)
        devType="04"
        ;;
esac

case $QUERY_STRING in
    *on*)
        function="05"
        ;;
    *off*)
        function="07"
        ;;
    *bright*)
        function="0b"
        ;;
    *dim*)
        function="09"
        ;;
esac

case $QUERY_STRING in
    *all*)
	if [ $function -eq "05" ]
	then 
		function="03"
	fi

	if [ $function -eq "07" ]
	then 
		function="01"
	fi

        ;;
esac

# /bin/echo $type $period > /tmp/led-control
WORKDIR=/root/huarain/
#${WORKDIR}/app_raw_write /dev/mcp2510 ${key}${house}00000000000000004201${function}0000 2
#${WORKDIR}/app_raw_write /dev/mcp2510 ${key}${house}${devType}000000000000004201${function}0000 4
${WORKDIR}/app_raw_write /dev/mcp2510 ${key}${house}${devType}000000000000004201${function}0000

# /root/app_raw_write /dev/mcp2510 ${key}${house}00000000000000004201${function}0000 e
# /root/app_raw_write /dev/mcp2510 010f00000000000000004201${function}0000 e

echo "Content-type: text/html; charset=gb2312"
echo
## /bin/cat led-result.template

# /bin/cat index.html
/bin/cat goto2.html

exit 0

