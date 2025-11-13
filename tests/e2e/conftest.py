"""Pytest configuration and fixtures for IRC server e2e tests."""

import pytest
import subprocess
import time
import os
from irc_client import IRCClient


# Test server configuration
TEST_SERVER_HOST = os.environ.get("IRC_SERVER_HOST", "localhost")
TEST_SERVER_PORT = int(os.environ.get("IRC_SERVER_PORT", "6667"))
TEST_SERVER_PASSWORD = os.environ.get("IRC_SERVER_PASSWORD", "password")


@pytest.fixture(scope="session")
def server_config():
    """Provide server configuration for tests."""
    return {
        "host": TEST_SERVER_HOST,
        "port": TEST_SERVER_PORT,
        "password": TEST_SERVER_PASSWORD,
    }


@pytest.fixture
def irc_client(server_config):
    """
    Provide a connected IRC client for tests.

    The client is automatically disconnected after the test.
    """
    client = IRCClient(
        host=server_config["host"],
        port=server_config["port"],
        timeout=5.0
    )
    client.connect()

    yield client

    try:
        client.quit_cmd("Test finished")
    except Exception:
        pass
    finally:
        client.disconnect()


@pytest.fixture
def authenticated_client(irc_client, server_config):
    """
    Provide an authenticated IRC client (PASS + NICK + USER).

    The client is registered with nickname 'testuser'.
    """
    # Receive initial notice
    try:
        irc_client.recv_line()
    except Exception:
        pass

    # Authenticate
    irc_client.pass_cmd(server_config["password"])
    irc_client.nick("testuser")
    irc_client.user("testuser", "Test User")

    # Wait for welcome message (001)
    welcome = irc_client.wait_for_reply("001", timeout=2.0)
    if not welcome:
        pytest.fail("Failed to receive welcome message from server")

    return irc_client


@pytest.fixture
def two_clients(server_config):
    """
    Provide two authenticated IRC clients for testing interactions.

    Returns tuple of (client1, client2) with nicknames 'user1' and 'user2'.
    """
    client1 = IRCClient(
        host=server_config["host"],
        port=server_config["port"],
        timeout=5.0
    )
    client2 = IRCClient(
        host=server_config["host"],
        port=server_config["port"],
        timeout=5.0
    )

    try:
        # Connect and authenticate both clients
        client1.connect()
        try:
            client1.recv_line()
        except Exception:
            pass
        client1.pass_cmd(server_config["password"])
        client1.nick("user1")
        client1.user("user1", "User One")
        client1.wait_for_reply("001", timeout=2.0)

        client2.connect()
        try:
            client2.recv_line()
        except Exception:
            pass
        client2.pass_cmd(server_config["password"])
        client2.nick("user2")
        client2.user("user2", "User Two")
        client2.wait_for_reply("001", timeout=2.0)

        yield (client1, client2)

    finally:
        for client in [client1, client2]:
            try:
                client.quit_cmd("Test finished")
            except Exception:
                pass
            finally:
                client.disconnect()
