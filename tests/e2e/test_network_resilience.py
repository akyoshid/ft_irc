"""Test network resilience and edge cases.

These tests verify the server's robustness against various network disruptions
and partial command scenarios as required by the evaluation criteria.
"""

import socket
import time
from irc_client import IRCClient


def test_partial_command(server_config):
    """
    Test server handling of partial commands.

    Manual reproduction:
        Terminal 1:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        (Wait for 001)
        JOIN #te
        (Don't press Enter, keep connection open)

        Terminal 2:
        $ nc -C localhost 6667
        PASS password
        NICK testuser2
        USER testuser2 0 * :Test User 2
        (Server should respond normally with 001)

    Expected: Server handles partial commands gracefully without blocking other connections
    """
    # Client 1 sends partial command
    client1 = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    client1.connect()
    try:
        client1.recv_line()
    except Exception:
        pass

    client1.pass_cmd(server_config["password"])
    client1.nick("testuser")
    client1.user("testuser", "Test User")
    client1.wait_for_reply("001", timeout=2.0)

    # Send partial command without CRLF
    client1.socket.sendall(b"JOIN #te")
    time.sleep(0.5)

    # Client 2 should still connect normally
    client2 = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    try:
        client2.connect()
        try:
            client2.recv_line()
        except Exception:
            pass

        client2.pass_cmd(server_config["password"])
        client2.nick("testuser2")
        client2.user("testuser2", "Test User 2")

        # Client 2 should receive welcome
        welcome = client2.wait_for_reply("001", timeout=2.0)
        assert welcome is not None, "Client 2 should authenticate successfully despite Client 1's partial command"

    finally:
        client2.disconnect()
        client1.disconnect()


def test_sudden_disconnect(authenticated_client, server_config):
    """
    Test server handling of sudden client disconnection.

    Manual reproduction:
        Terminal 1:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        JOIN #test
        (Press Ctrl+C to kill nc)

        Terminal 2:
        $ nc -C localhost 6667
        PASS password
        NICK testuser2
        USER testuser2 0 * :Test User 2
        (Server should respond normally)

    Expected: Server remains operational after unexpected client disconnection
    """
    # First client joins channel
    authenticated_client.join("#test")
    time.sleep(0.2)
    authenticated_client.recv_lines(timeout=0.5)

    # Simulate sudden disconnect (close socket without QUIT)
    authenticated_client.socket.close()
    time.sleep(0.5)

    # New client should connect successfully
    client2 = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    try:
        client2.connect()
        try:
            client2.recv_line()
        except Exception:
            pass

        client2.pass_cmd(server_config["password"])
        client2.nick("testuser2")
        client2.user("testuser2", "Test User 2")

        welcome = client2.wait_for_reply("001", timeout=2.0)
        assert welcome is not None, "Server should accept new connections after sudden client disconnect"

    finally:
        client2.disconnect()


def test_partial_command_then_kill(server_config):
    """
    Test server handling when client is killed with half-sent command.

    Manual reproduction:
        Terminal 1:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        (Wait for 001)
        JOIN #te
        (Don't press Enter, then press Ctrl+C)

        Terminal 2:
        $ nc -C localhost 6667
        PASS password
        NICK testuser2
        USER testuser2 0 * :Test User 2
        (Server should respond normally)

    Expected: Server not in odd state or blocked after client killed with partial command
    """
    client1 = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    client1.connect()
    try:
        client1.recv_line()
    except Exception:
        pass

    client1.pass_cmd(server_config["password"])
    client1.nick("testuser")
    client1.user("testuser", "Test User")
    client1.wait_for_reply("001", timeout=2.0)

    # Send partial command then immediately disconnect
    client1.socket.sendall(b"JOIN #partial")
    client1.socket.close()
    time.sleep(0.5)

    # Verify server is still operational
    client2 = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    try:
        client2.connect()
        try:
            client2.recv_line()
        except Exception:
            pass

        client2.pass_cmd(server_config["password"])
        client2.nick("testuser2")
        client2.user("testuser2", "Test User 2")

        welcome = client2.wait_for_reply("001", timeout=2.0)
        assert welcome is not None, "Server should remain operational after client killed with partial command"

    finally:
        client2.disconnect()


def test_multiple_simultaneous_connections(server_config):
    """
    Test server can handle multiple simultaneous connections.

    Manual reproduction:
        Terminal 1:
        $ nc -C localhost 6667
        PASS password
        NICK user1
        USER user1 0 * :User One
        JOIN #multi

        Terminal 2:
        $ nc -C localhost 6667
        PASS password
        NICK user2
        USER user2 0 * :User Two
        JOIN #multi

        Terminal 3:
        $ nc -C localhost 6667
        PASS password
        NICK user3
        USER user3 0 * :User Three
        JOIN #multi

        (All should join successfully and receive messages from others)

    Expected: Server handles multiple connections without blocking
    """
    clients = []
    try:
        # Create and authenticate 5 clients
        for i in range(5):
            client = IRCClient(
                host=server_config["host"],
                port=server_config["port"]
            )
            client.connect()
            try:
                client.recv_line()
            except Exception:
                pass

            client.pass_cmd(server_config["password"])
            client.nick(f"user{i}")
            client.user(f"user{i}", f"User {i}")

            welcome = client.wait_for_reply("001", timeout=2.0)
            assert welcome is not None, f"Client {i} should authenticate successfully"

            clients.append(client)
            time.sleep(0.1)

        # All clients join same channel
        for client in clients:
            client.join("#multi")
            time.sleep(0.1)

        # Clear buffers
        time.sleep(0.5)
        for client in clients:
            client.recv_lines(timeout=0.5)

        # First client sends message
        test_message = "Testing multiple connections"
        clients[0].privmsg("#multi", test_message)
        time.sleep(0.3)

        # All other clients should receive the message
        for i, client in enumerate(clients[1:], start=1):
            lines = client.recv_lines(timeout=1.0)
            msg_found = any(test_message in line for line in lines)
            assert msg_found, f"Client {i} should receive broadcast message"

    finally:
        for client in clients:
            try:
                client.disconnect()
            except Exception:
                pass


def test_buffer_overflow_protection(server_config):
    """
    Test server handling of very long commands.

    Manual reproduction:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        (Wait for 001)
        PRIVMSG #test :<very long message with 1000+ characters>
        (Server should handle gracefully)

    Expected: Server handles long commands without crashing or hanging
    """
    client = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    try:
        client.connect()
        try:
            client.recv_line()
        except Exception:
            pass

        client.pass_cmd(server_config["password"])
        client.nick("testuser")
        client.user("testuser", "Test User")

        welcome = client.wait_for_reply("001", timeout=2.0)
        assert welcome is not None, "Should authenticate successfully"

        # Send very long message (IRC typically limits to 512 bytes including CRLF)
        long_message = "A" * 1000
        client.privmsg("#test", long_message)
        time.sleep(0.5)

        # Server should still respond (might be error or truncation)
        # The key is that server doesn't crash
        client.ping("stillalive")
        lines = client.recv_lines(timeout=2.0)
        pong_found = any("PONG" in line for line in lines)
        assert pong_found, "Server should still respond after receiving long command"

    finally:
        client.disconnect()


def test_rapid_reconnection(server_config):
    """
    Test server handling of rapid connect/disconnect cycles.

    Manual reproduction:
        $ for i in {1..10}; do
            echo -e "PASS password\\r\\nNICK test$i\\r\\nUSER test$i 0 * :Test\\r\\nQUIT\\r\\n" | nc localhost 6667
            sleep 0.1
        done
        (Server should handle all connections)

    Expected: Server handles rapid connect/disconnect without resource leaks
    """
    for i in range(10):
        client = IRCClient(
            host=server_config["host"],
            port=server_config["port"]
        )
        try:
            client.connect()
            try:
                client.recv_line()
            except Exception:
                pass

            client.pass_cmd(server_config["password"])
            client.nick(f"test{i}")
            client.user(f"test{i}", f"Test {i}")

            # Wait briefly for response
            time.sleep(0.1)

            # Disconnect immediately
            client.disconnect()
            time.sleep(0.1)

        except Exception as e:
            # If connection fails, server might be rate limiting (acceptable)
            # But should not crash
            pass

    # Verify server is still operational with a normal connection
    final_client = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    try:
        final_client.connect()
        try:
            final_client.recv_line()
        except Exception:
            pass

        final_client.pass_cmd(server_config["password"])
        final_client.nick("finaltest")
        final_client.user("finaltest", "Final Test")

        welcome = final_client.wait_for_reply("001", timeout=2.0)
        assert welcome is not None, "Server should accept connections after rapid reconnection cycles"

    finally:
        final_client.disconnect()
