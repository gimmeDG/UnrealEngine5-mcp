"""
Socket Client - TCP communication with Unreal Engine plugin.

Singleton client with automatic retry logic for transient connection errors.
"""
import socket
import json
import time
from typing import Dict, Any, Optional
from config import UNREAL_HOST, UNREAL_PORT, SOCKET_TIMEOUT
from ..utils import log_error, log_warning, log_mcp_call, create_error_response

MAX_RETRIES = 3
RETRY_DELAY = 0.5
RETRYABLE_ERRORS = (
    ConnectionResetError,
    ConnectionAbortedError,
    BrokenPipeError,
    OSError,
)


class UnrealSocketClient:
    """TCP socket client for Unreal Engine communication with retry logic."""
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(UnrealSocketClient, cls).__new__(cls)
            cls._instance._initialized = False
        return cls._instance

    def __init__(self):
        if self._initialized:
            return
        self.host = UNREAL_HOST
        self.port = UNREAL_PORT
        self.timeout = float(SOCKET_TIMEOUT)
        self._socket: Optional[socket.socket] = None
        self._initialized = True

    def _connect(self) -> bool:
        """Establish connection to Unreal Engine."""
        try:
            self._close_socket()
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.settimeout(self.timeout)
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)
            self._socket.connect((self.host, self.port))
            return True
        except ConnectionRefusedError as e:
            log_error(f"Cannot connect to {self.host}:{self.port}", error=e)
            self._socket = None
            return False
        except Exception as e:
            log_error("Connection failed", error=e)
            self._socket = None
            return False

    def _close_socket(self):
        if self._socket:
            try:
                self._socket.close()
            except Exception:
                pass
            self._socket = None

    def close(self):
        self._close_socket()

    def _receive_full_response(self) -> bytes:
        """Receive complete JSON response."""
        chunks = []
        self._socket.settimeout(self.timeout)

        while True:
            chunk = self._socket.recv(65536)
            if not chunk:
                if not chunks:
                    raise ConnectionResetError("Connection closed before receiving data")
                break

            chunks.append(chunk)
            data = b''.join(chunks)

            try:
                json.loads(data.decode('utf-8'))
                return data
            except json.JSONDecodeError:
                continue

        return b''.join(chunks)

    def _send_command_once(self, command_type: str, params: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """Single attempt to send command."""
        self._close_socket()

        if not self._connect():
            return {"status": "error", "error": "Failed to connect to Unreal Engine"}

        command = {"type": command_type, "params": params}

        try:
            command_json = json.dumps(command) + "\n"
            self._socket.sendall(command_json.encode('utf-8'))

            response_data = self._receive_full_response()
            response_str = response_data.decode('utf-8').strip()
            return json.loads(response_str)
        finally:
            self._close_socket()

    def send_command(self, command_type: str, params: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """Send command with retry logic for transient connection errors."""
        last_error = None

        for attempt in range(MAX_RETRIES):
            try:
                return self._send_command_once(command_type, params)

            except json.JSONDecodeError as e:
                log_error(f"'{command_type}' response decode failed", error=e)
                return {"status": "error", "error": "Invalid response format from Unreal Engine"}

            except socket.timeout as e:
                log_error(f"'{command_type}' timed out after {self.timeout}s", error=e)
                return {"status": "error", "error": f"Command timed out after {self.timeout}s"}

            except RETRYABLE_ERRORS as e:
                last_error = e
                if attempt < MAX_RETRIES - 1:
                    log_warning(
                        f"Retry {attempt + 1}/{MAX_RETRIES} for '{command_type}'",
                        error=e
                    )
                    time.sleep(RETRY_DELAY)
                    continue
                else:
                    log_error(
                        f"'{command_type}' failed after {MAX_RETRIES} retries",
                        error=e
                    )

            except Exception as e:
                log_error(f"'{command_type}' unexpected error", error=e, include_traceback=True)
                return {"status": "error", "error": str(e)}

        return {"status": "error", "error": f"Command failed after {MAX_RETRIES} retries"}

    def execute_python(self, code: str) -> Dict[str, Any]:
        return self.send_command("execute_python", {"script": code})

    def ping(self) -> bool:
        response = self.send_command("ping", {})
        return response.get("status") == "success" if response else False

    def get_connection_status(self) -> Dict[str, Any]:
        return {
            "connected": self.ping(),
            "host": self.host,
            "port": self.port
        }

    def execute_command(
        self,
        command: str,
        params: Dict[str, Any],
        log_errors: bool = True
    ) -> Dict[str, Any]:
        """Execute command with error handling and I/O logging."""
        start_time = time.time()
        response = None

        try:
            response = self.send_command(command, params)

            if not response:
                error_msg = "No response from Unreal Engine"
                if log_errors:
                    log_error(f"{command}: {error_msg}")
                response = create_error_response(error_msg)
                return response

            if response.get("status") == "error" and log_errors:
                log_error(f"{command} failed: {response.get('error')}")

            return response

        except Exception as e:
            if log_errors:
                log_error(f"{command} exception", error=e, include_traceback=True)
            response = create_error_response(str(e))
            return response

        finally:
            duration_ms = (time.time() - start_time) * 1000
            log_mcp_call(command, params, response, duration_ms)


def get_unreal_client() -> UnrealSocketClient:
    return UnrealSocketClient()
