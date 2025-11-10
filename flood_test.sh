#!/bin/bash

# Argument Check
if [ $# -ne 3 ]; then
    echo "Usage: $0 <port> <password> <message_count>"
    exit 1
fi

PORT=$1
PASSWORD=$2
COUNT=$3

# Generate IRC commands
{
    echo "PASS $PASSWORD"
    echo "NICK flooder"
    echo "USER flooder flooder localhost :Flood Bot"
    sleep 1  # Waiting for registration to complete
    echo "JOIN #42tokyo"
    sleep 1  # Waiting for JOIN to complete
    
    # Flood start
    for i in $(seq 1 $COUNT); do
        echo "PRIVMSG #42tokyo :Flood message $i"
    done
    
    sleep 2
    echo "QUIT :Flood test complete"
} | nc -C localhost $PORT
