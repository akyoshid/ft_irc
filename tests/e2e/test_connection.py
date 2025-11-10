"""Test basic connection and authentication scenarios."""

from irc_client import IRCClient


def test_connect_to_server(server_config):
    """
    Test basic connection to IRC server.

    Manual reproduction:
        $ nc -C localhost 6667
        (Server sends): :ft_irc NOTICE * :Please authenticate with PASS command

    Expected: Server accepts TCP connection and sends initial NOTICE
    """
    client = IRCClient(
        host=server_config["host"],
        port=server_config["port"]
    )
    client.connect()

    # Should receive initial notice
    line = client.recv_line()
    assert "NOTICE" in line or ":" in line

    client.disconnect()


def test_authentication_success(irc_client, server_config):
    """
    Test successful authentication flow.

    Manual reproduction:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        (Server sends): :ft_irc 001 testuser :Welcome to the Internet Relay Network...

    Expected: Server accepts authentication and sends RPL_WELCOME (001)
    """
    # Receive initial notice
    try:
        irc_client.recv_line()
    except Exception:
        pass

    # Send authentication commands
    irc_client.pass_cmd(server_config["password"])
    irc_client.nick("testuser")
    irc_client.user("testuser", "Test User")

    # Wait for welcome message (001)
    welcome = irc_client.wait_for_reply("001", timeout=2.0)
    assert welcome is not None, "Should receive welcome message (001)"
    assert "testuser" in welcome


def test_authentication_wrong_password(irc_client):
    """
    Test authentication with wrong password.

    Manual reproduction:
        $ nc -C localhost 6667
        PASS wrongpassword
        NICK testuser
        USER testuser 0 * :Test User
        (Server sends): :ft_irc 464 * :Password incorrect

    Expected: Server rejects authentication and sends ERR_PASSWDMISMATCH (464)
    """
    # Receive initial notice
    try:
        irc_client.recv_line()
    except Exception:
        pass

    # Send wrong password
    irc_client.pass_cmd("wrongpassword")
    irc_client.nick("testuser")
    irc_client.user("testuser", "Test User")

    # Should receive error (464 - ERR_PASSWDMISMATCH)
    error = irc_client.wait_for_reply("464", timeout=2.0)
    assert error is not None, "Should receive password mismatch error (464)"


def test_authentication_no_password(irc_client):
    """
    Test authentication without PASS command.

    Manual reproduction:
        $ nc -C localhost 6667
        NICK testuser
        USER testuser 0 * :Test User
        (No 001 welcome message)

    Expected: Server does not send RPL_WELCOME without valid PASS
    """
    # Receive initial notice
    try:
        irc_client.recv_line()
    except Exception:
        pass

    # Try to authenticate without PASS
    irc_client.nick("testuser")
    irc_client.user("testuser", "Test User")

    # Should receive error (no 001 welcome)
    welcome = irc_client.wait_for_reply("001", timeout=1.0)
    assert welcome is None, "Should not receive welcome without PASS"


def test_duplicate_nickname(authenticated_client, server_config):
    """
    Test connecting with duplicate nickname.

    Manual reproduction:
        Terminal 1:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User

        Terminal 2:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser2 0 * :Test User 2
        (Server sends): :ft_irc 433 * testuser :Nickname is already in use

    Expected: Server rejects duplicate nickname with ERR_NICKNAMEINUSE (433)
    """
    # First client is already authenticated as 'testuser'

    # Try to connect second client with same nickname
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
        client2.nick("testuser")  # Same nickname
        client2.user("testuser2", "Test User 2")

        # Should receive nickname in use error (433)
        error = client2.wait_for_reply("433", timeout=2.0)
        assert error is not None, "Should receive nickname in use error (433)"

    finally:
        client2.disconnect()


def test_ping_pong(authenticated_client):
    """
    Test PING/PONG mechanism.

    Manual reproduction:
        $ nc -C localhost 6667
        PASS password
        NICK testuser
        USER testuser 0 * :Test User
        PING test123
        (Server sends): PONG :test123

    Expected: Server responds to PING with PONG
    """
    authenticated_client.ping("test123")

    # Wait for PONG response
    lines = authenticated_client.recv_lines(timeout=1.0)
    pong_found = any("PONG" in line and "test123" in line for line in lines)
    assert pong_found, "Should receive PONG response"
