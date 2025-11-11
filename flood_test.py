#!/usr/bin/env python3
import socket
import time
import sys

def send_command(sock, command):
    """Send IRC command"""
    sock.send(f"{command}\r\n".encode('utf-8'))

def flood_test(host, port, password, channel, count):
    """IRC flood test - mass message version"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print(f"Connecting to {host}:{port}...")
        sock.connect((host, port))
        
        # Authentication
        print("Authenticating...")
        send_command(sock, f"PASS {password}")
        send_command(sock, "NICK flooder")
        send_command(sock, "USER flooder flooder localhost :Flood Bot")
        time.sleep(2)
        
        print(f"Joining {channel}...")
        send_command(sock, f"JOIN {channel}")
        time.sleep(1)
        
        # Send mass messages
        print(f"Sending {count} messages...")
        message = "X" * 80  # 80-character message
        
        for i in range(1, count + 1):
            send_command(sock, f"PRIVMSG {channel} :Msg {i}: {message}")
            
            if i % 1000 == 0:
                print(f"Sent {i}/{count} messages")
                time.sleep(0.01)  # Brief pause
        
        print("Flood complete, disconnecting...")
        send_command(sock, "QUIT :Flood test complete")
        time.sleep(1)
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        sock.close()

if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("Usage: python3 flood_test.py <host> <port> <password> <channel> <count>")
        print("Example: python3 flood_test.py localhost 6667 password '#42tokyo' 10000")
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2])
    password = sys.argv[3]
    channel = sys.argv[4]
    count = int(sys.argv[5])
    
    # Validate channel name (warning only)
    if not channel.startswith('#') and not channel.startswith('&'):
        print(f"Warning: Channel name '{channel}' doesn't start with # or &")
        sys.exit(1)
    
    flood_test(host, port, password, channel, count)
