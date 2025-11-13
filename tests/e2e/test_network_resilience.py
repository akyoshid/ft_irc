"""Test network resilience and edge cases.

These tests verify the server's robustness against various network disruptions
and partial command scenarios as required by the evaluation criteria.
"""

import socket
import time
from irc_client import IRCClient, IRCMessage


def test_partial_command(server_config):
    """
    Test server handling of partial commands.

    IRC Protocol: Commands are terminated with CRLF (\\r\\n). This test verifies
    the server correctly handles incomplete commands that lack proper termination.

    Manual reproduction (testing protocol edge case):
        Terminal 1:
        $ irssi
        /connect localhost 6667 password testuser
        (Wait for connection)
        Type: /join #te
        (Don't press Enter - this simulates a partial command without CRLF)

        Terminal 2:
        $ irssi
        /connect localhost 6667 password testuser2
        (Server should respond normally with RPL_WELCOME 001)

    Expected: Server handles partial commands gracefully without blocking other connections

    Note: This tests protocol edge cases where commands are incomplete. Normal
    irssi usage would complete the command, but this verifies server robustness.
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
        $ irssi
        /connect localhost 6667 password testuser
        /join #test
        (Press Ctrl+C or close terminal to simulate sudden disconnect)

        Terminal 2:
        $ irssi
        /connect localhost 6667 password testuser2
        (Server should respond normally with RPL_WELCOME 001)

    Expected: Server remains operational after unexpected client disconnection

    Note: This simulates network failures where clients disconnect without
    sending QUIT command.
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

    IRC Protocol: This tests the edge case where a command is sent without
    CRLF termination and the connection is abruptly closed.

    Manual reproduction (testing protocol edge case):
        Terminal 1:
        $ irssi
        /connect localhost 6667 password testuser
        (Wait for RPL_WELCOME 001)
        Type: /join #te
        (Don't press Enter, then press Ctrl+C to force close)

        Terminal 2:
        $ irssi
        /connect localhost 6667 password testuser2
        (Server should respond normally with RPL_WELCOME 001)

    Expected: Server not in odd state or blocked after client killed with partial command

    Note: This simulates crash or force-close scenarios during command input.
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

    IRC Protocol: Tests concurrent connection handling and message routing
    to multiple clients on the same channel.

    Manual reproduction:
        Terminal 1:
        $ irssi
        /connect localhost 6667 password user1
        /join #multi

        Terminal 2:
        $ irssi
        /connect localhost 6667 password user2
        /join #multi

        Terminal 3:
        $ irssi
        /connect localhost 6667 password user3
        /join #multi

        (All should join successfully and receive JOIN messages from others)

    Expected: Server handles multiple connections without blocking

    IRC Messages:
        - JOIN #multi (sent by each client)
        - :user1!~user1@host JOIN :#multi (broadcast to channel members)
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
            privmsg_found = None
            for line in lines:
                msg = IRCMessage(line)
                if msg.command == "PRIVMSG" and len(msg.params) >= 2 and test_message in msg.params[1]:
                    privmsg_found = msg
                    break
            assert privmsg_found is not None, f"Client {i} should receive broadcast message"
            assert privmsg_found.command == "PRIVMSG"
            assert privmsg_found.params[0] == "#multi"

    finally:
        for client in clients:
            try:
                client.disconnect()
            except Exception:
                pass


def test_buffer_overflow_protection(server_config):
    """
    Test server handling of very long commands.

    IRC Protocol: RFC 1459 specifies messages must not exceed 512 characters
    including CRLF. Servers should truncate or reject oversized messages.

    Manual reproduction:
        $ irssi
        /connect localhost 6667 password testuser
        (Wait for RPL_WELCOME 001)
        /msg #test <paste very long message with 1000+ characters>
        (Server should handle gracefully - may truncate or send ERR_INPUTTOOLONG)

    Expected: Server handles long commands without crashing or hanging

    IRC Protocol Format:
        PRIVMSG #test :message text
        Maximum: 512 bytes including "\\r\\n"
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
        pong_msg = None
        for line in lines:
            msg = IRCMessage(line)
            if msg.command == "PONG":
                pong_msg = msg
                break
        assert pong_msg is not None, "Should receive PONG"
        assert pong_msg.command == "PONG"

    finally:
        client.disconnect()


def test_rapid_reconnection(server_config):
    """
    Test server handling of rapid connect/disconnect cycles.

    IRC Protocol: Tests connection lifecycle management with rapid
    authentication and disconnection sequences.

    Manual reproduction:
        $ for i in {1..10}; do
            irssi -c localhost -p 6667 -w password -n test$i --noconnect &
            sleep 0.2
            pkill -f "irssi.*test$i"
        done
        (Server should handle all connections)

    Alternative:
        In 10 terminals, rapidly:
        $ irssi
        /connect localhost 6667 password test1
        /quit
        (Repeat with test2, test3, etc.)

    Expected: Server handles rapid connect/disconnect without resource leaks

    IRC Commands:
        PASS password
        NICK test1
        USER test1 0 * :Test
        QUIT :Leaving
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
