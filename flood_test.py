#!/usr/bin/env python3
import socket
import time
import sys

def send_command(sock, command):
    """IRC コマンドを送信"""
    sock.send(f"{command}\r\n".encode('utf-8'))

def flood_test(host, port, password, channel, count):
    """IRC フラッドテスト - 大量メッセージ版"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print(f"Connecting to {host}:{port}...")
        sock.connect((host, port))
        
        # 認証
        print("Authenticating...")
        send_command(sock, f"PASS {password}")
        send_command(sock, "NICK flooder")
        send_command(sock, "USER flooder flooder localhost :Flood Bot")
        time.sleep(2)
        
        print(f"Joining {channel}...")
        send_command(sock, f"JOIN {channel}")
        time.sleep(1)
        
        # 大量メッセージ送信
        print(f"Sending {count} messages...")
        message = "X" * 80  # 80文字のメッセージ
        
        for i in range(1, count + 1):
            send_command(sock, f"PRIVMSG {channel} :Msg {i}: {message}")
            
            if i % 1000 == 0:
                print(f"Sent {i}/{count} messages")
                time.sleep(0.01)  # 少し待つ
        
        print("Flood complete, disconnecting...")
        send_command(sock, "QUIT :Flood test complete")
        time.sleep(1)
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python3 irc_flood.py <host> <port> <password> <count>")
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2])
    password = sys.argv[3]
    count = int(sys.argv[4])
    
    flood_test(host, port, password, "#42tokyo", count)
