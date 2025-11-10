"""Test messaging operations (PRIVMSG, KICK, INVITE)."""

import time


def test_privmsg_to_user(two_clients):
    """
    Test sending private message to another user.

    Manual reproduction:
        Terminal 1 (user1):
        $ telnet localhost 6667
        PASS password
        NICK user1
        USER user1 0 * :User One
        PRIVMSG user2 :Hello user2!

        Terminal 2 (user2):
        $ telnet localhost 6667
        PASS password
        NICK user2
        USER user2 0 * :User Two
        (Receives): :user1!user1@localhost PRIVMSG user2 :Hello user2!

    Expected: Server delivers private message from user1 to user2
    """
    client1, client2 = two_clients

    # Clear initial messages
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 sends message to Client2
    test_message = "Hello user2!"
    client1.privmsg("user2", test_message)
    time.sleep(0.2)

    # Client2 should receive the message
    lines = client2.recv_lines(timeout=1.0)
    msg_found = any("PRIVMSG" in line and test_message in line for line in lines)
    assert msg_found, "Client2 should receive private message from Client1"


def test_privmsg_to_channel(two_clients):
    """
    Test sending message to channel.

    Manual reproduction:
        Terminal 1 (user1):
        $ telnet localhost 6667
        PASS password
        NICK user1
        USER user1 0 * :User One
        JOIN #chat
        PRIVMSG #chat :Hello everyone in #chat!

        Terminal 2 (user2):
        $ telnet localhost 6667
        PASS password
        NICK user2
        USER user2 0 * :User Two
        JOIN #chat
        (Receives): :user1!user1@localhost PRIVMSG #chat :Hello everyone in #chat!

    Expected: Server broadcasts channel message to all members except sender
    """
    client1, client2 = two_clients

    # Both join the same channel
    client1.join("#chat")
    time.sleep(0.1)
    client2.join("#chat")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 sends message to channel
    test_message = "Hello everyone in #chat!"
    client1.privmsg("#chat", test_message)
    time.sleep(0.2)

    # Client2 should receive the message
    lines = client2.recv_lines(timeout=1.0)
    msg_found = any("PRIVMSG" in line and "#chat" in line and test_message in line
                    for line in lines)
    assert msg_found, "Client2 should receive channel message"


def test_privmsg_to_nonexistent_user(authenticated_client):
    """Test sending message to non-existent user."""
    authenticated_client.privmsg("nonexistent", "Hello?")

    # Should receive error (401 - ERR_NOSUCHNICK)
    error = authenticated_client.wait_for_reply("401", timeout=1.0)
    assert error is not None, "Should receive no such nick error"


def test_privmsg_to_channel_not_joined(authenticated_client):
    """Test sending message to channel not joined."""
    authenticated_client.privmsg("#notjoined", "Hello?")

    # Should receive error (404 - ERR_CANNOTSENDTOCHAN)
    lines = authenticated_client.recv_lines(timeout=1.0)
    error_found = any("404" in line for line in lines)
    assert error_found, "Should receive cannot send to channel error"


def test_kick_user_from_channel(two_clients):
    """Test kicking a user from channel."""
    client1, client2 = two_clients

    # Both join the same channel
    client1.join("#kick-test")
    time.sleep(0.1)
    client2.join("#kick-test")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 (operator) kicks Client2
    kick_reason = "You have been kicked"
    client1.kick("#kick-test", "user2", kick_reason)
    time.sleep(0.2)

    # Client2 should receive KICK notification
    lines = client2.recv_lines(timeout=1.0)
    kick_found = any("KICK" in line and "#kick-test" in line and "user2" in line
                     for line in lines)
    assert kick_found, "Client2 should receive KICK notification"


def test_kick_without_operator_privilege(two_clients):
    """Test that non-operators cannot kick users."""
    client1, client2 = two_clients

    # Both join the same channel
    client1.join("#nokick")
    time.sleep(0.1)
    client2.join("#nokick")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client2 (non-operator) tries to kick Client1
    client2.kick("#nokick", "user1", "Trying to kick")

    # Should receive error (482 - ERR_CHANOPRIVSNEEDED)
    lines = client2.recv_lines(timeout=1.0)
    error_found = any("482" in line for line in lines)
    assert error_found, "Non-operator should not be able to kick"


def test_invite_user_to_channel(two_clients):
    """Test inviting a user to channel."""
    client1, client2 = two_clients

    # Client1 creates and sets invite-only channel
    client1.join("#invite-test")
    time.sleep(0.2)
    client1.mode("#invite-test", "+i")
    time.sleep(0.2)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 invites Client2
    client1.invite("user2", "#invite-test")
    time.sleep(0.2)

    # Client2 should receive INVITE notification
    lines = client2.recv_lines(timeout=1.0)
    invite_found = any("INVITE" in line and "#invite-test" in line for line in lines)
    assert invite_found, "Client2 should receive INVITE notification"

    # Client2 should now be able to join
    client2.join("#invite-test")
    time.sleep(0.2)
    lines = client2.recv_lines(timeout=1.0)
    join_found = any("JOIN" in line and "#invite-test" in line for line in lines)
    assert join_found, "Client2 should be able to join after invite"


def test_invite_without_operator_privilege(two_clients):
    """Test that non-operators cannot invite to invite-only channels."""
    client1, client2 = two_clients

    # Client1 creates invite-only channel
    client1.join("#noinvite")
    time.sleep(0.2)
    client1.mode("#noinvite", "+i")
    time.sleep(0.2)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Create a third client for testing
    from irc_client import IRCClient
    client3 = IRCClient(
        host=client1.host,
        port=client1.port,
        timeout=5.0
    )

    try:
        client3.connect()
        try:
            client3.recv_line()
        except Exception:
            pass

        # Authenticate client3
        client3.send_raw("PASS password")  # Using default password
        client3.nick("user3")
        client3.user("user3", "User Three")
        client3.wait_for_reply("001", timeout=2.0)

        # Client2 (not in channel) tries to invite Client3
        client2.invite("user3", "#noinvite")

        # Should receive error (442 - ERR_NOTONCHANNEL or 482 - ERR_CHANOPRIVSNEEDED)
        lines = client2.recv_lines(timeout=1.0)
        error_found = any("442" in line or "482" in line for line in lines)
        assert error_found, "Non-member should not be able to invite"

    finally:
        try:
            client3.quit_cmd()
        except Exception:
            pass
        client3.disconnect()


def test_broadcast_message_to_all_channel_members(two_clients):
    """Test that messages are broadcast to all channel members."""
    client1, client2 = two_clients

    # Both join the same channel
    client1.join("#broadcast")
    time.sleep(0.1)
    client2.join("#broadcast")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 sends message
    test_message = "Broadcasting to all!"
    client1.privmsg("#broadcast", test_message)
    time.sleep(0.2)

    # Client2 should receive it
    lines_client2 = client2.recv_lines(timeout=1.0)
    msg_found = any(test_message in line for line in lines_client2)
    assert msg_found, "All channel members should receive broadcast message"

    # Client1 should NOT receive their own message
    lines_client1 = client1.recv_lines(timeout=0.5)
    msg_echoed = any(test_message in line and "PRIVMSG" in line for line in lines_client1)
    assert not msg_echoed, "Sender should not receive their own message"
