"""IRC Client utility for testing ft_irc server."""

import socket
import time
from typing import Dict, List, Optional


class IRCMessage:
    """Parsed IRC message structure."""

    def __init__(self, raw_line: str):
        """
        Parse an IRC protocol message.

        Format: [:prefix] COMMAND [param1 param2 ...] [:trailing]

        Args:
            raw_line: Raw IRC message line (without CRLF)
        """
        self.raw = raw_line
        self.prefix = None
        self.command = None
        self.params = []

        parts = raw_line.split(' ')
        idx = 0

        # Parse prefix (optional)
        if parts[idx].startswith(':'):
            self.prefix = parts[idx][1:]  # Remove leading ':'
            idx += 1

        # Parse command
        if idx < len(parts):
            self.command = parts[idx]
            idx += 1

        # Parse parameters
        while idx < len(parts):
            if parts[idx].startswith(':'):
                # Trailing parameter (rest of message)
                self.params.append(' '.join(parts[idx:])[1:])  # Remove leading ':'
                break
            else:
                self.params.append(parts[idx])
                idx += 1

    def __repr__(self):
        return f"IRCMessage(command={self.command}, prefix={self.prefix}, params={self.params})"

    def to_dict(self) -> Dict:
        """Convert to dictionary for easy testing."""
        return {
            'raw': self.raw,
            'prefix': self.prefix,
            'command': self.command,
            'params': self.params
        }


class IRCClient:
    """Simple IRC client for testing purposes."""

    def __init__(self, host: str = "localhost", port: int = 6667, timeout: float = 5.0):
        """
        Initialize IRC client.

        Args:
            host: Server hostname
            port: Server port
            timeout: Socket timeout in seconds
        """
        self.host = host
        self.port = port
        self.timeout = timeout
        self.socket: Optional[socket.socket] = None
        self.buffer = ""

    def connect(self) -> None:
        """Establish connection to IRC server."""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.settimeout(self.timeout)
        self.socket.connect((self.host, self.port))
        self.buffer = ""

    def disconnect(self) -> None:
        """Close connection to IRC server."""
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass
            finally:
                self.socket = None
                self.buffer = ""

    def send_raw(self, message: str) -> None:
        """
        Send raw message to server.

        Args:
            message: Message to send (CRLF will be added automatically)
        """
        if not self.socket:
            raise RuntimeError("Not connected to server")
        if not message.endswith("\r\n"):
            message += "\r\n"
        self.socket.sendall(message.encode("utf-8"))

    def recv_raw(self, size: int = 4096) -> str:
        """
        Receive raw data from server.

        Args:
            size: Maximum bytes to receive

        Returns:
            Received data as string
        """
        if not self.socket:
            raise RuntimeError("Not connected to server")
        data = self.socket.recv(size)
        return data.decode("utf-8", errors="ignore")

    def recv_line(self) -> str:
        """
        Receive one complete line from server.

        Returns:
            One line (without CRLF)
        """
        while "\r\n" not in self.buffer:
            data = self.recv_raw()
            if not data:
                raise ConnectionError("Connection closed by server")
            self.buffer += data

        line, self.buffer = self.buffer.split("\r\n", 1)
        return line

    def recv_lines(self, timeout: Optional[float] = None) -> List[str]:
        """
        Receive multiple lines until timeout.

        Args:
            timeout: Timeout in seconds (default: use socket timeout)

        Returns:
            List of received lines
        """
        if timeout is None:
            timeout = self.timeout

        lines = []
        original_timeout = self.socket.gettimeout() if self.socket else None

        try:
            if self.socket:
                self.socket.settimeout(timeout)

            while True:
                try:
                    line = self.recv_line()
                    lines.append(line)
                except socket.timeout:
                    break
                except ConnectionError:
                    break
        finally:
            if self.socket and original_timeout is not None:
                self.socket.settimeout(original_timeout)

        return lines

    def wait_for_reply(self, expected_code: str, timeout: float = 2.0) -> Optional[IRCMessage]:
        """
        Wait for a specific numeric reply from server.

        Args:
            expected_code: Expected numeric reply code (e.g., "001", "433")
            timeout: Timeout in seconds

        Returns:
            Parsed IRCMessage if found, None otherwise
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                line = self.recv_line()
                msg = IRCMessage(line)
                if msg.command == expected_code:
                    return msg
            except socket.timeout:
                continue
            except ConnectionError:
                return None
        return None

    # ==========================================
    # High-level IRC commands
    # ==========================================

    def pass_cmd(self, password: str) -> None:
        """Send PASS command."""
        self.send_raw(f"PASS {password}")

    def nick(self, nickname: str) -> None:
        """Send NICK command."""
        self.send_raw(f"NICK {nickname}")

    def user(self, username: str, realname: str = "Test User") -> None:
        """Send USER command."""
        self.send_raw(f"USER {username} 0 * :{realname}")

    def join(self, channel: str, key: str = "") -> None:
        """Send JOIN command."""
        if key:
            self.send_raw(f"JOIN {channel} {key}")
        else:
            self.send_raw(f"JOIN {channel}")

    def part(self, channel: str, reason: str = "") -> None:
        """Send PART command."""
        if reason:
            self.send_raw(f"PART {channel} :{reason}")
        else:
            self.send_raw(f"PART {channel}")

    def privmsg(self, target: str, message: str) -> None:
        """Send PRIVMSG command."""
        self.send_raw(f"PRIVMSG {target} :{message}")

    def kick(self, channel: str, user: str, reason: str = "") -> None:
        """Send KICK command."""
        if reason:
            self.send_raw(f"KICK {channel} {user} :{reason}")
        else:
            self.send_raw(f"KICK {channel} {user}")

    def invite(self, nickname: str, channel: str) -> None:
        """Send INVITE command."""
        self.send_raw(f"INVITE {nickname} {channel}")

    def topic(self, channel: str, topic: str = None) -> None:
        """Send TOPIC command."""
        if topic is None:
            self.send_raw(f"TOPIC {channel}")
        else:
            self.send_raw(f"TOPIC {channel} :{topic}")

    def mode(self, target: str, modes: str = "") -> None:
        """Send MODE command."""
        if modes:
            self.send_raw(f"MODE {target} {modes}")
        else:
            self.send_raw(f"MODE {target}")

    def quit_cmd(self, reason: str = "Leaving") -> None:
        """Send QUIT command."""
        self.send_raw(f"QUIT :{reason}")

    def ping(self, token: str = "test") -> None:
        """Send PING command."""
        self.send_raw(f"PING {token}")

    def pong(self, token: str) -> None:
        """Send PONG command."""
        self.send_raw(f"PONG {token}")

    # ==========================================
    # Context manager support
    # ==========================================

    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
        return False
