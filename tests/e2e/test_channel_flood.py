"""Test channel flooding scenarios.

These tests verify the server's ability to handle message flooding
and buffer management as required by the evaluation criteria.
"""

import time
from irc_client import IRCClient


def test_channel_message_flood(two_clients):
    """
    Test server handling of channel message flooding.

    IRC Protocol: Tests server's ability to handle rapid PRIVMSG commands
    and message queue management.

    Manual reproduction:
        Terminal 1 (receiver):
        $ irssi
        /connect localhost 6667 password receiver
        /join #flood
        (Messages will queue up while receiving)

        Terminal 2 (sender):
        $ irssi
        /connect localhost 6667 password sender
        /join #flood

        Send messages rapidly using a loop or script:
        /msg #flood Message 1
        /msg #flood Message 2
        ... (send 100+ messages rapidly by pasting or scripting)

    Expected: Server queues messages and delivers when client is ready.
             No memory leaks during this operation.

    IRC Protocol Format:
        PRIVMSG #flood :message text
        Server broadcasts: :sender!~sender@host PRIVMSG #flood :message text

    Note: This test simulates the stopped client by using a slow reader.
    """
    client1, client2 = two_clients

    # Both join the same channel
    client1.join("#flood")
    time.sleep(0.1)
    client2.join("#flood")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 sends many messages rapidly
    message_count = 50
    for i in range(message_count):
        client1.privmsg("#flood", f"Flood message {i}")
        # No sleep - send as fast as possible

    # Small delay to let server process
    time.sleep(0.5)

    # Client2 should receive all messages (or server should handle gracefully)
    received_messages = []
    try:
        lines = client2.recv_lines(timeout=3.0)
        received_messages.extend(lines)
    except Exception:
        pass

    # Check that we received some messages (server didn't crash)
    flood_messages = [line for line in received_messages if "Flood message" in line]

    # We should receive messages (exact count depends on buffering)
    # The key is that server doesn't crash or hang
    assert len(flood_messages) > 0, "Should receive at least some flood messages"

    # Verify server is still responsive
    client1.ping("stillalive")
    lines = client1.recv_lines(timeout=2.0)
    pong_found = any("PONG" in line for line in lines)
    assert pong_found, "Server should still be responsive after flood"


def test_large_channel_broadcast(server_config):
    """
    Test broadcasting messages to a channel with many users.

    IRC Protocol: Tests server's broadcast efficiency when sending messages
    to channels with multiple members.

    Manual reproduction:
        Terminal 1-5 (5 different users):
        $ irssi
        /connect localhost 6667 password user1
        /join #large
        (Repeat for user2, user3, user4, user5 in separate terminals)

        Terminal 1:
        /msg #large Message to all!
        (All other 4 terminals should receive the message)

    Expected: Server efficiently broadcasts to all channel members

    IRC Messages:
        Client sends: PRIVMSG #large :Message to all!
        Each member receives: :user1!~user1@host PRIVMSG #large :Message to all!
    """
    clients = []
    try:
        # Create 10 clients
        for i in range(10):
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
            client.wait_for_reply("001", timeout=2.0)

            clients.append(client)
            time.sleep(0.1)

        # All join same channel
        for client in clients:
            client.join("#large")
            time.sleep(0.1)

        # Clear buffers
        time.sleep(0.5)
        for client in clients:
            client.recv_lines(timeout=0.5)

        # First client broadcasts message
        test_message = "Broadcast to large channel"
        clients[0].privmsg("#large", test_message)
        time.sleep(0.5)

        # All other clients should receive
        received_count = 0
        for client in clients[1:]:
            lines = client.recv_lines(timeout=1.0)
            if any(test_message in line for line in lines):
                received_count += 1

        # At least most clients should receive (allow for some timing issues)
        assert received_count >= len(clients) - 2, f"Most clients should receive broadcast (got {received_count}/{len(clients)-1})"

    finally:
        for client in clients:
            try:
                client.disconnect()
            except Exception:
                pass


def test_multiple_channel_flood(server_config):
    """
    Test flooding messages across multiple channels simultaneously.

    IRC Protocol: Tests server's ability to handle rapid PRIVMSG commands
    across multiple channels concurrently.

    Manual reproduction:
        Terminal 1:
        $ irssi
        /connect localhost 6667 password flooder
        /join #chan1
        /join #chan2
        /join #chan3

        Send messages rapidly to different channels:
        /msg #chan1 Message to chan1
        /msg #chan2 Message to chan2
        /msg #chan3 Message to chan3
        (Repeat rapidly by pasting multiple times or using a script)

    Expected: Server handles multi-channel flooding without issues

    IRC Protocol Format:
        PRIVMSG #chan1 :Message to chan1
        PRIVMSG #chan2 :Message to chan2
        PRIVMSG #chan3 :Message to chan3
    """
    sender = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )

    receivers = []
    channels = ["#chan1", "#chan2", "#chan3"]

    try:
        # Setup sender
        sender.connect()
        try:
            sender.recv_line()
        except Exception:
            pass
        sender.pass_cmd(server_config["password"])
        sender.nick("flooder")
        sender.user("flooder", "Flooder")
        sender.wait_for_reply("001", timeout=2.0)

        # Sender joins all channels
        for channel in channels:
            sender.join(channel)
            time.sleep(0.1)

        # Create receiver for each channel
        for i, channel in enumerate(channels):
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
            client.nick(f"receiver{i}")
            client.user(f"receiver{i}", f"Receiver {i}")
            client.wait_for_reply("001", timeout=2.0)
            client.join(channel)

            receivers.append(client)
            time.sleep(0.2)

        # Clear buffers
        time.sleep(0.5)
        sender.recv_lines(timeout=0.5)
        for client in receivers:
            client.recv_lines(timeout=0.5)

        # Flood all channels
        for i in range(20):
            for channel in channels:
                sender.privmsg(channel, f"Flood message {i} to {channel}")

        time.sleep(1.0)

        # Verify server is still responsive
        sender.ping("stillalive")
        lines = sender.recv_lines(timeout=2.0)
        pong_found = any("PONG" in line for line in lines)
        assert pong_found, "Server should remain responsive after multi-channel flood"

        # Each receiver should have received some messages
        for i, client in enumerate(receivers):
            lines = client.recv_lines(timeout=2.0)
            channel_messages = [line for line in lines if channels[i] in line]
            assert len(channel_messages) > 0, f"Receiver {i} should receive messages from {channels[i]}"

    finally:
        try:
            sender.disconnect()
        except Exception:
            pass
        for client in receivers:
            try:
                client.disconnect()
            except Exception:
                pass


def test_slow_client_doesnt_block_fast_clients(server_config):
    """
    Test that a slow-reading client doesn't block fast clients.

    IRC Protocol: Tests server's non-blocking I/O and per-client buffer management
    to ensure one slow client doesn't impact others.

    Manual reproduction:
        Terminal 1 (slow client):
        $ irssi
        /connect localhost 6667 password slowclient
        /join #mixed
        (Press Ctrl+Z to suspend and simulate slow reading)

        Terminal 2 (fast client):
        $ irssi
        /connect localhost 6667 password fastclient
        /join #mixed

        Terminal 3 (sender):
        $ irssi
        /connect localhost 6667 password sender
        /join #mixed

        Send multiple messages rapidly:
        /msg #mixed Message 1
        /msg #mixed Message 2
        ... (send many messages rapidly)

        Terminal 2:
        (Should continue receiving messages despite Terminal 1 being suspended)

    Expected: Fast clients continue receiving messages even if slow client is blocked

    IRC Messages:
        Sender broadcasts: PRIVMSG #mixed :Message N
        Fast client receives: :sender!~sender@host PRIVMSG #mixed :Message N
        Slow client buffer fills but doesn't block others
    """
    sender = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    fast_client = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    slow_client = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )

    try:
        # Setup all clients
        for client, nickname in [(sender, "sender"), (fast_client, "fast"), (slow_client, "slow")]:
            client.connect()
            try:
                client.recv_line()
            except Exception:
                pass
            client.pass_cmd(server_config["password"])
            client.nick(nickname)
            client.user(nickname, nickname.title())
            client.wait_for_reply("001", timeout=2.0)
            client.join("#mixed")
            time.sleep(0.2)

        # Clear buffers
        time.sleep(0.5)
        sender.recv_lines(timeout=0.5)
        fast_client.recv_lines(timeout=0.5)
        slow_client.recv_lines(timeout=0.5)

        # Sender floods channel (slow_client won't read)
        for i in range(30):
            sender.privmsg("#mixed", f"Message {i}")

        time.sleep(0.5)

        # Fast client should receive messages
        fast_lines = fast_client.recv_lines(timeout=2.0)
        fast_messages = [line for line in fast_lines if "Message" in line]

        # Fast client should receive some messages (not blocked by slow client)
        assert len(fast_messages) > 0, "Fast client should receive messages despite slow client"

        # Verify server is still responsive
        sender.ping("check")
        lines = sender.recv_lines(timeout=2.0)
        pong_found = any("PONG" in line for line in lines)
        assert pong_found, "Server should remain responsive"

    finally:
        try:
            sender.disconnect()
            fast_client.disconnect()
            slow_client.disconnect()
        except Exception:
            pass
