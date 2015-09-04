#!/bin/bash

case "$1" in
	start)
		screenPIDs=$(screen -list | grep "(.*)" | grep 'WebSocketControl\|RoPi_Socket' | awk -F"." {'print $1'})
		if [[ ! -z "$screenPIDs" ]]; then
			echo "FAILED! Screens already run:"
			$0 list
			exit 1
		fi
		screen -dmS RoPi_Socket /usr/bin/python /usr/local/bin/RoPi_Socket.py
		sleep 2
		screen -dmS WebSocketControl /usr/bin/python /usr/local/bin/WebSocketControl.py
		if [ -z "$(ps aux | grep -v grep | grep rtmp-nginx.sh)" ]; then
			nohup /usr/local/bin/rtmp-nginx.sh >/dev/null 2>&1 &
		fi
	;;
	list)
		screen -list | grep "(.*)" | grep --color 'WebSocketControl\|RoPi_Socket'
	;;
	stop)
		screenPIDs=$(screen -list | grep "(.*)" | grep 'WebSocketControl\|RoPi_Socket' | awk -F"." {'print $1'})
		if [[ -z "$screenPIDs" ]]; then
			echo "FAILED! Screens doesnt run!"
			exit 1
		fi
		for pid in $screenPIDs; do kill -9 $pid >/dev/null 2>&1; screen -wipe $pid >/dev/null 2>&1; done
		echo Done
	;;
	*)
		echo "Usage: $(basename $0) [start|stop|list]"
	;;
esac

exit 0