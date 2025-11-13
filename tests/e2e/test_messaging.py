"""Test messaging operations (PRIVMSG, KICK, INVITE)."""

import time


def test_privmsg_to_user(two_clients):
    """
    Test sending private message to another user.

    IRC Protocol:
        Client sends: PRIVMSG <nickname> :<message>
        Server relays: :<sender>!<user>@<host> PRIVMSG <target> :<message>

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /msg user2 Hello user2!

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        (Receives private message from user1)

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

    IRC Protocol:
        Client sends: PRIVMSG <channel> :<message>
        Server broadcasts: :<sender>!<user>@<host> PRIVMSG <channel> :<message>

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /join #chat
        /msg #chat Hello everyone in #chat!

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #chat
        (Receives channel message from user1)

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
    """
    Test sending message to non-existent user.

    IRC Protocol:
        Client sends: PRIVMSG <nickname> :<message>
        Server responds: 401 <client> <nickname> :No such nick/channel

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /msg nonexistent Hello?
        (Server sends error: No such nick/channel)

    Expected: Server returns ERR_NOSUCHNICK (401)
    """
    authenticated_client.privmsg("nonexistent", "Hello?")

    # Should receive error (401 - ERR_NOSUCHNICK)
    error = authenticated_client.wait_for_reply("401", timeout=1.0)
    assert error is not None, "Should receive no such nick error"


def test_privmsg_to_channel_not_joined(authenticated_client):
    """
    Test sending message to channel not joined.

    IRC Protocol:
        Client sends: PRIVMSG <channel> :<message>
        Server responds with one of:
            403 <client> <channel> :No such channel
            404 <client> <channel> :Cannot send to channel
            401 <client> <channel> :No such nick/channel

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /msg #notjoined Hello?
        (Server sends error indicating channel doesn't exist or cannot send)

    Expected: Server returns an error indicating the channel doesn't exist or
              the user cannot send to it (403, 404, or 401)

    Note: IRC servers may return different error codes:
          - 403 (ERR_NOSUCHCHANNEL): No such channel
          - 404 (ERR_CANNOTSENDTOCHAN): Cannot send to channel
          - 401 (ERR_NOSUCHNICK): No such nick/channel
    """
    authenticated_client.privmsg("#notjoined", "Hello?")

    # Should receive error (403, 404, or 401)
    lines = authenticated_client.recv_lines(timeout=1.0)
    error_found = any("403" in line or "404" in line or "401" in line for line in lines)
    assert error_found, f"Should receive error (403, 404, or 401), received: {lines}"


def test_kick_user_from_channel(two_clients):
    """
    Test kicking a user from channel.

    IRC Protocol:
        Client sends: KICK <channel> <nickname> :<reason>
        Server broadcasts: :<operator>!<user>@<host> KICK <channel> <nickname> :<reason>

    Manual reproduction with irssi:
        Terminal 1 (user1 - operator):
        $ irssi
        /connect localhost 6667 password user1
        /join #kick-test
        /kick user2 You have been kicked

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #kick-test
        (Receives KICK notification and is removed from channel)

    Expected: Server broadcasts KICK to all channel members and removes kicked user
    """
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
    """
    Test that non-operators cannot kick users.

    IRC Protocol:
        Client sends: KICK <channel> <nickname> :<reason>
        Server responds: 482 <client> <channel> :You're not channel operator

    Manual reproduction with irssi:
        Terminal 1 (user1 - operator):
        $ irssi
        /connect localhost 6667 password user1
        /join #nokick

        Terminal 2 (user2 - regular user):
        $ irssi
        /connect localhost 6667 password user2
        /join #nokick
        /kick user1 Trying to kick
        (Server sends error: You're not channel operator)

    Expected: Server rejects KICK from non-operators (ERR_CHANOPRIVSNEEDED)
    """
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
    """
    Test inviting a user to channel.

    IRC Protocol:
        Client sends: INVITE <nickname> <channel>
        Server confirms: 341 <client> <nickname> <channel>
        Server notifies: :<inviter>!<user>@<host> INVITE <nickname> :<channel>

    Manual reproduction with irssi:
        Terminal 1 (user1 - operator):
        $ irssi
        /connect localhost 6667 password user1
        /join #invite-test
        /mode #invite-test +i
        /invite user2 #invite-test

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        (Receives INVITE notification from user1)
        /join #invite-test
        (Successfully joins the invite-only channel)

    Expected: Server sends INVITE notification and allows invited user to join
    """
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
    """
    Test that non-operators cannot invite to invite-only channels.

    IRC Protocol:
        Client sends: INVITE <nickname> <channel>
        Server responds with one of:
            442 <client> <channel> :You're not on that channel
            482 <client> <channel> :You're not channel operator

    Manual reproduction with irssi:
        Terminal 1 (user1 - operator):
        $ irssi
        /connect localhost 6667 password user1
        /join #noinvite
        /mode #noinvite +i

        Terminal 2 (user2 - not in channel):
        $ irssi
        /connect localhost 6667 password user2
        /invite user3 #noinvite
        (Server sends error: You're not on that channel or not channel operator)

    Expected: Server rejects INVITE from users not in channel or non-operators
    """
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
    """
    Test that messages are broadcast to all channel members.

    IRC Protocol:
        Client sends: PRIVMSG <channel> :<message>
        Server broadcasts: :<sender>!<user>@<host> PRIVMSG <channel> :<message>
        Note: Server does NOT echo message back to sender

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /join #broadcast
        /msg #broadcast Broadcasting to all!
        (Server does NOT echo back to sender)

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #broadcast
        (Receives channel message from user1)

    Expected: Server broadcasts to all members EXCEPT the sender
    """
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
