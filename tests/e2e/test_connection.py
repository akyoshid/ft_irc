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

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        (Server sends): :ft_irc 001 testuser :Welcome to the Internet Relay Network...

    Expected: Server accepts authentication and sends RPL_WELCOME (001)
    IRC protocol format: :server 001 target :message
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

    # Validate message structure
    assert welcome.command == "001", "Command should be 001"
    assert len(welcome.params) >= 2, "Should have target and welcome message"
    assert welcome.params[0] == "testuser", "First param should be target nickname"
    assert "Welcome" in welcome.params[1], "Should contain welcome message"


def test_authentication_wrong_password(irc_client):
    """
    Test authentication with wrong password.

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 wrongpassword testuser
        (Server sends): :ft_irc 464 * :Password incorrect

    Expected: Server rejects authentication and sends ERR_PASSWDMISMATCH (464)
    IRC protocol format: :server 464 target :message
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

    # Validate message structure
    assert error.command == "464", "Command should be 464"
    assert len(error.params) >= 1, "Should have target and error message"
    # Target might be * for unauthenticated users
    assert error.params[0] in ["*", "testuser"], "First param should be target"


def test_authentication_no_password(irc_client):
    """
    Test authentication without PASS command.

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 "" testuser
        (No 001 welcome message - connection should fail or timeout)

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

    # Should not receive welcome (no 001)
    welcome = irc_client.wait_for_reply("001", timeout=1.0)
    assert welcome is None, "Should not receive welcome without PASS"


def test_duplicate_nickname(authenticated_client, server_config):
    """
    Test connecting with duplicate nickname.

    Manual reproduction with irssi:
        Terminal 1:
        $ irssi
        /connect localhost 6667 password testuser

        Terminal 2:
        $ irssi
        /connect localhost 6667 password testuser
        (Server sends): :ft_irc 433 * testuser :Nickname is already in use

    Expected: Server rejects duplicate nickname with ERR_NICKNAMEINUSE (433)
    IRC protocol format: :server 433 target attempted_nick :message
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

        # Validate message structure
        assert error.command == "433", "Command should be 433"
        assert len(error.params) >= 2, "Should have target and attempted nickname"
        assert error.params[1] == "testuser", "Second param should be attempted nickname"

    finally:
        client2.disconnect()


def test_ping_pong(authenticated_client):
    """
    Test PING/PONG mechanism.

    Manual reproduction with irssi:
        $ irssi
        /connect localhost 6667 password testuser
        /quote PING test123
        (Server sends): PONG :test123

    Expected: Server responds to PING with PONG
    IRC protocol format: PONG :token
    """
    authenticated_client.ping("test123")

    # Wait for PONG response
    lines = authenticated_client.recv_lines(timeout=1.0)
    pong_found = any("PONG" in line and "test123" in line for line in lines)
    assert pong_found, "Should receive PONG response"
