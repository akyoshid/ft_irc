"""Test channel operations (JOIN, PART, TOPIC, MODE)."""

import time


def test_join_channel(authenticated_client):
    """
    Test joining a channel.

    Manual reproduction:
        $ telnet localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        JOIN #test
        (Server sends): :testuser!testuser@localhost JOIN :#test

    Expected: Server confirms JOIN and broadcasts to channel members
    """
    authenticated_client.join("#test")

    # Should receive JOIN confirmation
    lines = authenticated_client.recv_lines(timeout=1.0)
    join_found = any("JOIN" in line and "#test" in line for line in lines)
    assert join_found, "Should receive JOIN confirmation"


def test_join_multiple_channels(authenticated_client):
    """Test joining multiple channels."""
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


def test_join_channel_with_key(two_clients):
    """
    Test joining a channel with key (password).

    Manual reproduction:
        Terminal 1 (user1):
        $ telnet localhost 6667
        PASS password
        NICK user1
        USER user1 0 * :User One
        JOIN #private
        MODE #private +k secret123

        Terminal 2 (user2):
        $ telnet localhost 6667
        PASS password
        NICK user2
        USER user2 0 * :User Two
        JOIN #private
        (Server sends): :ft_irc 475 user2 #private :Cannot join channel (+k)
        JOIN #private secret123
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

    # Client2 joins with correct key (should succeed)
    client2.join("#private", "secret123")
    lines = client2.recv_lines(timeout=1.0)
    join_found = any("JOIN" in line and "#private" in line for line in lines)
    assert join_found, "Should successfully join with correct key"


def test_part_channel(authenticated_client):
    """Test leaving a channel."""
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


def test_topic_operations(two_clients):
    """Test setting and querying channel topic."""
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


def test_mode_invite_only(two_clients):
    """Test invite-only mode (+i)."""
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


def test_mode_topic_restricted(two_clients):
    """Test topic-restricted mode (+t)."""
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


def test_mode_user_limit(two_clients):
    """Test user limit mode (+l)."""
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
