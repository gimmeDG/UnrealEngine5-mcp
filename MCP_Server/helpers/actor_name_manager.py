"""
Actor Name Manager - Prevents duplicate actor name conflicts.
"""

import time
import uuid
from typing import Dict, Any, Set, Optional
from ..utils import log_info, log_error

class ActorNameManager:
    """Manages unique actor names across MCP functions."""

    def __init__(self):
        self._known_actors: Set[str] = set()
        self._session_id = str(int(time.time()))[-6:]
        self._actor_counters: Dict[str, int] = {}
        log_info(f"ActorNameManager initialized with session ID: {self._session_id}")

    def generate_unique_name(self, base_name: str, unreal_connection=None) -> str:
        """Generate a unique actor name based on the desired base name."""
        base_name = str(base_name).strip()
        if not base_name:
            base_name = f"Actor_{self._session_id}"

        if not self._actor_exists(base_name, unreal_connection):
            return base_name

        session_name = f"{base_name}_{self._session_id}"
        if not self._actor_exists(session_name, unreal_connection):
            return session_name

        counter_key = base_name
        if counter_key not in self._actor_counters:
            self._actor_counters[counter_key] = 0

        for _ in range(1000):
            self._actor_counters[counter_key] += 1
            counter_name = f"{base_name}_{self._actor_counters[counter_key]}"
            if not self._actor_exists(counter_name, unreal_connection):
                return counter_name

        unique_suffix = str(uuid.uuid4())[:8]
        final_name = f"{base_name}_{self._session_id}_{self._actor_counters[counter_key]}_{unique_suffix}"

        log_info(f"Generated unique name: {base_name} -> {final_name}")
        return final_name

    def _actor_exists(self, name: str, unreal_connection=None) -> bool:
        """Check if an actor with the given name exists.

        Returns True if:
        1. Exact match found in cache
        2. Exact match found in Unreal Engine
        3. An actor with name starting with 'name' exists (name collision prevention)
        """
        if name in self._known_actors:
            return True

        if unreal_connection:
            try:
                response = unreal_connection.send_command("search_actors", {"pattern": name})
                if response and response.get("status") == "success":
                    result = response.get("result", {})
                    actors = result.get("actors", []) if isinstance(result, dict) else []

                    if isinstance(actors, list):
                        # First pass: exact match - cache the name
                        for actor in actors:
                            if isinstance(actor, dict) and actor.get("name") == name:
                                self._known_actors.add(name)
                                return True
                        # Second pass: prefix match - don't cache (may cause false positives)
                        # This prevents naming collisions like "Actor" when "Actor_1" exists
                        for actor in actors:
                            if isinstance(actor, dict) and actor.get("name", "").startswith(name):
                                # Return True but don't cache - next query will re-check
                                return True
            except Exception as e:
                log_error(f"Error checking actor existence for '{name}': {e}")

        return False

    def mark_actor_created(self, name: str):
        """Add actor to known actors cache."""
        self._known_actors.add(name)

    def remove_actor(self, name: str):
        """Remove actor from known actors cache."""
        self._known_actors.discard(name)


_global_actor_name_manager = ActorNameManager()


def get_global_actor_name_manager() -> ActorNameManager:
    """Get global actor name manager instance."""
    return _global_actor_name_manager


def get_unique_actor_name(base_name: str, unreal_connection=None) -> str:
    """Get a unique actor name."""
    return _global_actor_name_manager.generate_unique_name(base_name, unreal_connection)

def safe_spawn_actor(unreal_connection, params: Dict[str, Any], auto_unique_name: bool = True) -> Dict[str, Any]:
    """Spawn actor with automatic unique name generation."""
    if not unreal_connection:
        return {"success": False, "status": "error", "error": "No Unreal connection available"}

    original_name = params.get("name", "Actor")

    if auto_unique_name:
        unique_name = _global_actor_name_manager.generate_unique_name(original_name, unreal_connection)
        params["name"] = unique_name

        if unique_name != original_name:
            log_info(f"Actor name changed: '{original_name}' -> '{unique_name}'")

    try:
        response = unreal_connection.send_command("spawn_actor", params)

        if response and response.get("status") == "success":
            _global_actor_name_manager.mark_actor_created(params["name"])

            if "result" in response:
                if isinstance(response["result"], dict):
                    response["result"]["final_name"] = params["name"]
                    response["result"]["original_name"] = original_name

        elif response and response.get("status") == "error" and "already exists" in response.get("error", ""):
            log_info(f"Actor '{params['name']}' was created elsewhere, marking as success")
            _global_actor_name_manager.mark_actor_created(params["name"])
            return {
                "status": "success",
                "result": {
                    "name": params["name"],
                    "final_name": params["name"],
                    "original_name": original_name,
                    "concurrent": True,
                    "reason": "Created by concurrent process"
                }
            }

        return response or {"success": False, "status": "error", "error": "No response from Unreal"}

    except Exception as e:
        log_error(f"Error in safe_spawn_actor: {e}")
        return {"success": False, "status": "error", "error": str(e)}

def safe_delete_actor(unreal_connection, actor_name: str) -> Dict[str, Any]:
    """Delete actor and update name tracking."""
    if not unreal_connection:
        return {"success": False, "status": "error", "error": "No Unreal connection available"}

    try:
        response = unreal_connection.send_command("delete_actor", {"name": actor_name})

        if response and response.get("status") == "success":
            _global_actor_name_manager.remove_actor(actor_name)

        return response or {"success": False, "status": "error", "error": "No response from Unreal"}

    except Exception as e:
        log_error(f"Error in safe_delete_actor: {e}")
        return {"success": False, "status": "error", "error": str(e)}
