"""Test channel operations (JOIN, PART, TOPIC, MODE)."""

import time
from irc_client import IRCMessage


def test_join_channel(authenticated_client):
    """
    Test joining a channel.

    IRC Protocol Format:
        Client -> Server: JOIN #channel
        Server -> Client: :nick!user@host JOIN :#channel

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /join #test
        (Server sends): :testuser!testuser@localhost JOIN :#test

    Expected: Server confirms JOIN and broadcasts to channel members
    """
    authenticated_client.join("#test")

    # Should receive JOIN confirmation
    lines = authenticated_client.recv_lines(timeout=1.0)
    join_found = any("JOIN" in line and "#test" in line for line in lines)
    assert join_found, "Should receive JOIN confirmation"

    # Validate JOIN message structure
    join_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "JOIN" and "#test" in msg.params:
            join_msg = msg
            break

    assert join_msg is not None, "Should receive JOIN message"
    assert join_msg.command == "JOIN"
    assert len(join_msg.params) >= 1
    assert join_msg.params[0] == "#test"


def test_join_multiple_channels(authenticated_client):
    """
    Test joining multiple channels.

    IRC Protocol Format:
        Client -> Server: JOIN #channel1
        Server -> Client: :nick!user@host JOIN :#channel1
        Client -> Server: JOIN #channel2
        Server -> Client: :nick!user@host JOIN :#channel2

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /join #test1
        (Server sends): :testuser!testuser@localhost JOIN :#test1
        /join #test2
        (Server sends): :testuser!testuser@localhost JOIN :#test2
        /join #test3
        (Server sends): :testuser!testuser@localhost JOIN :#test3

    Expected: Server confirms each JOIN separately
    """
    channels = ["#test1", "#test2", "#test3"]

    for channel in channels:
        authenticated_client.join(channel)
        time.sleep(0.1)  # Small delay between commands

    # Receive all responses
    lines = authenticated_client.recv_lines(timeout=2.0)

    # Check that all channels were joined
    for channel in channels:
        join_found = any("JOIN" in line and channel in line for line in lines)
        assert join_found, f"Should receive JOIN confirmation for {channel}"

    # Validate JOIN message structure for each channel
    for channel in channels:
        join_msg = None
        for line in lines:
            msg = IRCMessage(line)
            if msg.command == "JOIN" and channel in msg.params:
                join_msg = msg
                break

        assert join_msg is not None, f"Should receive JOIN message for {channel}"
        assert join_msg.command == "JOIN"
        assert len(join_msg.params) >= 1
        assert join_msg.params[0] == channel


def test_join_channel_with_key(two_clients):
    """
    Test joining a channel with key (password).

    IRC Protocol Format:
        Client -> Server: JOIN #channel key
        Server -> Client: :nick!user@host JOIN :#channel (on success)
        Server -> Client: :server 475 nick #channel :Cannot join channel (+k) (on error)

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /join #private
        /mode #private +k secret123

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #private
        (Server sends): :ft_irc 475 user2 #private :Cannot join channel (+k)
        /join #private secret123
        (Server sends): :user2!user2@localhost JOIN :#private

    Expected: Server rejects JOIN without key (475), accepts with correct key
    """
    client1, client2 = two_clients

    # Client1 creates channel and sets key
    client1.join("#private")
    time.sleep(0.2)
    client1.mode("#private", "+k secret123")
    time.sleep(0.2)

    # Clear buffers
    client1.recv_lines(timeout=0.5)

    # Client2 tries to join without key (should fail)
    client2.join("#private")
    lines = client2.recv_lines(timeout=1.0)
    # Should receive error (475 - ERR_BADCHANNELKEY)
    error_found = any("475" in line for line in lines)
    assert error_found, "Should receive bad channel key error without key"

    # Validate error message structure
    error_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "475":
            error_msg = msg
            break

    assert error_msg is not None, "Should receive error 475"
    assert error_msg.command == "475"
    assert len(error_msg.params) >= 2
    assert error_msg.params[0] == "user2"

    # Client2 joins with correct key (should succeed)
    client2.join("#private", "secret123")
    lines = client2.recv_lines(timeout=1.0)
    join_found = any("JOIN" in line and "#private" in line for line in lines)
    assert join_found, "Should successfully join with correct key"

    # Validate JOIN message structure
    join_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "JOIN" and "#private" in msg.params:
            join_msg = msg
            break

    assert join_msg is not None, "Should receive JOIN message"
    assert join_msg.command == "JOIN"
    assert len(join_msg.params) >= 1
    assert join_msg.params[0] == "#private"


def test_part_channel(authenticated_client):
    """
    Test leaving a channel.

    IRC Protocol Format:
        Client -> Server: PART #channel [reason]
        Server -> Client: :nick!user@host PART #channel [reason]

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /join #test
        /part #test Goodbye
        (Server sends): :testuser!testuser@localhost PART #test :Goodbye

    Expected: Server confirms PART and broadcasts to channel members
    """
    # Join channel first
    authenticated_client.join("#test")
    time.sleep(0.2)
    authenticated_client.recv_lines(timeout=0.5)

    # Part from channel
    authenticated_client.part("#test", "Goodbye")

    # Should receive PART confirmation
    lines = authenticated_client.recv_lines(timeout=1.0)
    part_found = any("PART" in line and "#test" in line for line in lines)
    assert part_found, "Should receive PART confirmation"

    # Validate PART message structure
    part_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "PART" and "#test" in msg.params:
            part_msg = msg
            break

    assert part_msg is not None, "Should receive PART message"
    assert part_msg.command == "PART"
    assert len(part_msg.params) >= 1
    assert part_msg.params[0] == "#test"


def test_topic_operations(two_clients):
    """
    Test setting and querying channel topic.

    IRC Protocol Format:
        Client -> Server: TOPIC #channel :new topic text
        Server -> Client: :nick!user@host TOPIC #channel :new topic text

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /join #topic-test
        /topic #topic-test Welcome to our IRC channel!
        (Server sends): :user1!user1@localhost TOPIC #topic-test :Welcome to our IRC channel!

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #topic-test
        (Receives): :user1!user1@localhost TOPIC #topic-test :Welcome to our IRC channel!

    Expected: Server broadcasts TOPIC change to all channel members
    """
    client1, client2 = two_clients

    # Both join the same channel
    client1.join("#topic-test")
    time.sleep(0.1)
    client2.join("#topic-test")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 sets topic
    new_topic = "Welcome to our IRC channel!"
    client1.topic("#topic-test", new_topic)
    time.sleep(0.2)

    # Client2 should receive topic change notification
    lines = client2.recv_lines(timeout=1.0)
    topic_found = any("TOPIC" in line and new_topic in line for line in lines)
    assert topic_found, "Client2 should receive topic change notification"

    # Validate TOPIC message structure
    topic_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "TOPIC" and new_topic in msg.params:
            topic_msg = msg
            break

    assert topic_msg is not None, "Should receive TOPIC message"
    assert topic_msg.command == "TOPIC"
    assert len(topic_msg.params) >= 2
    assert topic_msg.params[0] == "#topic-test"
    assert topic_msg.params[1] == new_topic


def test_mode_invite_only(two_clients):
    """
    Test invite-only mode (+i).

    IRC Protocol Format:
        Client -> Server: MODE #channel +i
        Server -> Client: :nick!user@host MODE #channel +i
        Server -> Client: :server 473 nick #channel :Cannot join channel (+i) (when JOIN fails)

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /join #invite-only
        /mode #invite-only +i
        (Server sends): :user1!user1@localhost MODE #invite-only +i

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #invite-only
        (Server sends): :ft_irc 473 user2 #invite-only :Cannot join channel (+i)

    Expected: Server rejects JOIN to invite-only channel without invite (ERR_INVITEONLYCHAN)
    """
    client1, client2 = two_clients

    # Client1 creates channel
    client1.join("#invite-only")
    time.sleep(0.2)
    client1.recv_lines(timeout=0.5)

    # Set invite-only mode
    client1.mode("#invite-only", "+i")
    time.sleep(0.2)
    client1.recv_lines(timeout=0.5)

    # Client2 tries to join (should fail)
    client2.join("#invite-only")
    lines = client2.recv_lines(timeout=1.0)
    # Should receive error (473 - ERR_INVITEONLYCHAN)
    error_found = any("473" in line for line in lines)
    assert error_found, "Should receive invite-only error"

    # Validate error message structure
    error_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "473":
            error_msg = msg
            break

    assert error_msg is not None, "Should receive error 473"
    assert error_msg.command == "473"
    assert len(error_msg.params) >= 2
    assert error_msg.params[0] == "user2"


def test_mode_topic_restricted(two_clients):
    """
    Test topic-restricted mode (+t).

    IRC Protocol Format:
        Client -> Server: MODE #channel +t
        Server -> Client: :nick!user@host MODE #channel +t
        Server -> Client: :server 482 nick #channel :You're not channel operator (when TOPIC fails)

    Manual reproduction with irssi:
        Terminal 1 (user1 - operator):
        $ irssi
        /connect localhost 6667 password user1
        /join #restricted
        /mode #restricted +t
        (Server sends): :user1!user1@localhost MODE #restricted +t

        Terminal 2 (user2 - regular user):
        $ irssi
        /connect localhost 6667 password user2
        /join #restricted
        /topic #restricted New topic by non-op
        (Server sends): :ft_irc 482 user2 #restricted :You're not channel operator

    Expected: Server rejects TOPIC from non-operators when +t is set (ERR_CHANOPRIVSNEEDED)
    """
    client1, client2 = two_clients

    # Both join channel
    client1.join("#restricted")
    time.sleep(0.1)
    client2.join("#restricted")
    time.sleep(0.3)

    # Clear buffers
    client1.recv_lines(timeout=0.5)
    client2.recv_lines(timeout=0.5)

    # Client1 (operator) sets +t mode
    client1.mode("#restricted", "+t")
    time.sleep(0.2)
    client1.recv_lines(timeout=0.5)

    # Client2 (non-operator) tries to set topic
    client2.topic("#restricted", "New topic by non-op")
    lines = client2.recv_lines(timeout=1.0)
    # Should receive error (482 - ERR_CHANOPRIVSNEEDED)
    error_found = any("482" in line for line in lines)
    assert error_found, "Non-operator should not be able to change topic with +t"

    # Validate error message structure
    error_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "482":
            error_msg = msg
            break

    assert error_msg is not None, "Should receive error 482"
    assert error_msg.command == "482"
    assert len(error_msg.params) >= 2
    assert error_msg.params[0] == "user2"


def test_mode_user_limit(two_clients):
    """
    Test user limit mode (+l).

    IRC Protocol Format:
        Client -> Server: MODE #channel +l limit
        Server -> Client: :nick!user@host MODE #channel +l limit
        Server -> Client: :server 471 nick #channel :Cannot join channel (+l) (when JOIN fails)

    Manual reproduction with irssi:
        Terminal 1 (user1):
        $ irssi
        /connect localhost 6667 password user1
        /join #limited
        /mode #limited +l 1
        (Server sends): :user1!user1@localhost MODE #limited +l 1

        Terminal 2 (user2):
        $ irssi
        /connect localhost 6667 password user2
        /join #limited
        (Server sends): :ft_irc 471 user2 #limited :Cannot join channel (+l)

    Expected: Server rejects JOIN when user limit is reached (ERR_CHANNELISFULL)
    """
    client1, client2 = two_clients

    # Client1 creates channel and sets user limit to 1
    client1.join("#limited")
    time.sleep(0.2)
    client1.mode("#limited", "+l 1")
    time.sleep(0.2)

    # Clear buffers
    client1.recv_lines(timeout=0.5)

    # Client2 tries to join (should fail due to limit)
    client2.join("#limited")
    lines = client2.recv_lines(timeout=1.0)
    # Should receive error (471 - ERR_CHANNELISFULL)
    error_found = any("471" in line for line in lines)
    assert error_found, "Should receive channel is full error"

    # Validate error message structure
    error_msg = None
    for line in lines:
        msg = IRCMessage(line)
        if msg.command == "471":
            error_msg = msg
            break

    assert error_msg is not None, "Should receive error 471"
    assert error_msg.command == "471"
    assert len(error_msg.params) >= 2
    assert error_msg.params[0] == "user2"
