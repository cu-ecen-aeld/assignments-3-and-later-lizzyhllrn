#!/bin/sh

case "$1" in 
 start)
   echo "Starting aesdsocket"
   start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket-simple -- -d
   ;;
 stop)
   echo "Stopping aesdsocket"
   start-stop-daemon -K -n /usr/bin/aesdsocket-simple
   ;;
  *)
   echo "Usage: $0 {start|stop}"
  exit 1
 esac
