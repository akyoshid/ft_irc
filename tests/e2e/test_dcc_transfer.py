"""Test DCC (Direct Client-to-Client) file transfer capability.

Note: DCC file transfer is a CLIENT feature, not a server feature.
The server only forwards CTCP DCC messages as regular PRIVMSG.
Actual file transfer happens directly between clients without server involvement.

This test verifies that the server correctly forwards DCC negotiation messages.
"""

import time
from irc_client import IRCClient, IRCMessage


def test_dcc_send_message_forwarding(two_clients, server_config):
    """
    Test that server forwards DCC SEND negotiation messages.

    Manual reproduction with irssi:
        Terminal 1 (sender):
        $ irssi
        /connect localhost 6667 password user1
        /dcc send user2 /path/to/file.txt
        (irssi sends): PRIVMSG user2 :\x01DCC SEND file.txt 192.168.1.10 12345 1024\x01

        Terminal 2 (receiver):
        $ irssi
        /connect localhost 6667 password user2
        (Should see DCC offer notification)

    Expected: Server forwards CTCP DCC message as regular PRIVMSG
    IRC Protocol:
        Client to Server: PRIVMSG target :\x01DCC SEND filename ip port size\x01
        Server to Target: :sender!~sender@host PRIVMSG target :\x01DCC SEND ...\x01

    Note: The server treats this as a normal PRIVMSG. The \x01 (CTCP delimiter)
    is just part of the message text. Clients interpret the CTCP protocol.
    """
    client1, client2 = two_clients

    # Clear buffers
    time.sleep(0.2)
    client1.recv_lines(timeout=0.3)
    client2.recv_lines(timeout=0.3)

    # User1 sends DCC SEND message (simulating /dcc send command)
    # Format: \x01DCC SEND filename ip port filesize\x01
    dcc_message = "\x01DCC SEND testfile.txt 2130706433 12345 1024\x01"
    # IP 2130706433 is decimal representation of 127.0.0.1
    client1.privmsg("user2", dcc_message)

    time.sleep(0.3)

    # User2 should receive the DCC message as a normal PRIVMSG
    lines = client2.recv_lines(timeout=1.0)

    # Find and validate the PRIVMSG containing DCC
    dcc_privmsg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "PRIVMSG" and len(msg.params) >= 2:
            if "DCC SEND" in msg.params[1]:
                dcc_privmsg = msg
                break

    assert dcc_privmsg is not None, "Should receive DCC SEND message"

    # Validate message structure
    assert dcc_privmsg.command == "PRIVMSG"
    assert len(dcc_privmsg.params) >= 2
    assert dcc_privmsg.params[0] == "user2", "Target should be user2"

    # Validate DCC content
    dcc_content = dcc_privmsg.params[1]
    assert dcc_content.startswith("\x01DCC SEND"), "Should start with CTCP DCC SEND"
    assert dcc_content.endswith("\x01"), "Should end with CTCP delimiter"
    assert "testfile.txt" in dcc_content, "Should contain filename"
    assert "2130706433" in dcc_content, "Should contain IP address"
    assert "12345" in dcc_content, "Should contain port"
    assert "1024" in dcc_content, "Should contain file size"


def test_dcc_chat_message_forwarding(two_clients):
    """
    Test that server forwards DCC CHAT negotiation messages.

    Manual reproduction with irssi:
        Terminal 1:
        $ irssi
        /connect localhost 6667 password user1
        /dcc chat user2
        (Sends DCC CHAT request)

        Terminal 2:
        $ irssi
        /connect localhost 6667 password user2
        (Should see DCC CHAT offer)

    Expected: Server forwards DCC CHAT as PRIVMSG
    IRC Protocol: PRIVMSG target :\x01DCC CHAT chat ip port\x01
    """
    client1, client2 = two_clients

    # Clear buffers
    time.sleep(0.2)
    client1.recv_lines(timeout=0.3)
    client2.recv_lines(timeout=0.3)

    # User1 sends DCC CHAT request
    dcc_chat_message = "\x01DCC CHAT chat 2130706433 54321\x01"
    client1.privmsg("user2", dcc_chat_message)

    time.sleep(0.3)

    # User2 should receive the DCC CHAT message
    lines = client2.recv_lines(timeout=1.0)

    dcc_chat_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "PRIVMSG" and len(msg.params) >= 2:
            if "DCC CHAT" in msg.params[1]:
                dcc_chat_msg = msg
                break

    assert dcc_chat_msg is not None, "Should receive DCC CHAT message"

    # Validate structure
    assert dcc_chat_msg.command == "PRIVMSG"
    assert dcc_chat_msg.params[0] == "user2"
    assert dcc_chat_msg.params[1].startswith("\x01DCC CHAT")
    assert dcc_chat_msg.params[1].endswith("\x01")


def test_dcc_with_channel(two_clients, server_config):
    """
    Test that DCC messages are NOT typically sent to channels.

    Manual reproduction with irssi:
        Terminal 1:
        $ irssi
        /connect localhost 6667 password user1
        /join #test
        /dcc send #test file.txt
        (irssi may reject this, but server would forward if sent)

        Terminal 2:
        $ irssi
        /connect localhost 6667 password user2
        /join #test
        (Should receive DCC message)

    Expected: Server treats it as normal PRIVMSG to channel
    Note: Most IRC clients don't allow DCC to channels, but the
    server doesn't enforce this - it's a client-side restriction.
    """
    client1, client2 = two_clients

    # Both users join the channel
    client1.join("#test")
    client2.join("#test")
    time.sleep(0.2)
    client1.recv_lines(timeout=0.3)
    client2.recv_lines(timeout=0.3)

    # User1 sends DCC message to channel (unusual but not prohibited)
    dcc_message = "\x01DCC SEND file.txt 2130706433 12345 1024\x01"
    client1.privmsg("#test", dcc_message)

    time.sleep(0.3)

    # User2 should receive the DCC message (broadcast to channel members)
    lines = client2.recv_lines(timeout=1.0)

    channel_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "PRIVMSG" and len(msg.params) >= 2:
            if msg.params[0] == "#test" and "DCC SEND" in msg.params[1]:
                channel_msg = msg
                break

    assert channel_msg is not None, "Should receive DCC message to channel"
    assert channel_msg.command == "PRIVMSG"
    assert channel_msg.params[0] == "#test"


def test_ctcp_version_as_comparison(authenticated_client):
    """
    Test other CTCP commands work similarly to DCC.

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /ctcp testuser VERSION
        (Sends and receives CTCP VERSION)

    Expected: CTCP messages are just PRIVMSG with \x01 delimiters
    IRC Protocol: PRIVMSG target :\x01COMMAND [params]\x01

    This demonstrates that DCC is just one type of CTCP message.
    """
    # Send CTCP VERSION to self
    ctcp_version = "\x01VERSION\x01"
    authenticated_client.privmsg("testuser", ctcp_version)

    time.sleep(0.3)

    # Should receive it back (message to self)
    lines = authenticated_client.recv_lines(timeout=1.0)

    version_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "PRIVMSG" and len(msg.params) >= 2:
            if "\x01VERSION\x01" in msg.params[1]:
                version_msg = msg
                break

    assert version_msg is not None, "Should receive CTCP VERSION message"
    assert version_msg.command == "PRIVMSG"
    assert version_msg.params[1] == "\x01VERSION\x01"
