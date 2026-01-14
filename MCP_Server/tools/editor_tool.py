"""
Editor Tools - Level actor instance manipulation.

Modifies actors placed in the current level (instances).
For Blueprint asset (CDO) modifications, use blueprint_tool instead.
"""
from typing import List, Dict, Any, Optional, Literal
from mcp.server.fastmcp import FastMCP
from ..unreal_client import get_unreal_client
from ..utils import (
    log_info, log_error,
    validate_vector3, validate_color, ensure_floats, create_error_response,
    validate_vectors
)
from ..helpers import get_unique_actor_name, safe_spawn_actor, safe_delete_actor


def register_editor_tools(mcp: FastMCP):
    """Register editor manipulation tools with MCP server"""

    # =========================================================================
    # Level Actor Tools
    # =========================================================================

    @mcp.tool()
    def spawn_actor(
        name: str,
        actor_type: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0],
        scale: List[float] = [1.0, 1.0, 1.0],
        static_mesh: str = "",
        auto_unique_name: bool = True
    ) -> Dict[str, Any]:
        """Spawn an actor in the current level."""
        try:
            if err := validate_vectors("spawn_actor", log_error, [
                (location, "location", validate_vector3),
                (rotation, "rotation", validate_vector3),
                (scale, "scale", validate_vector3)
            ]):
                return err

            client = get_unreal_client()

            params = {
                "name": name,
                "type": actor_type.upper(),
                "location": ensure_floats(location),
                "rotation": ensure_floats(rotation),
                "scale": ensure_floats(scale)
            }

            if static_mesh:
                params["static_mesh"] = static_mesh

            return safe_spawn_actor(client, params, auto_unique_name)

        except Exception as e:
            log_error(f"spawn_actor exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def spawn_blueprint_actor(
        blueprint_name: str,
        actor_name: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0],
        scale: List[float] = [1.0, 1.0, 1.0],
        auto_unique_name: bool = True,
        blueprint_path: str = "/Game/Blueprints/"
    ) -> Dict[str, Any]:
        """Spawn a Blueprint asset as an actor in the current level."""
        try:
            if err := validate_vectors("spawn_blueprint_actor", log_error, [
                (location, "location", validate_vector3),
                (rotation, "rotation", validate_vector3),
                (scale, "scale", validate_vector3)
            ]):
                return err

            client = get_unreal_client()
            original_name = actor_name

            if auto_unique_name:
                actor_name = get_unique_actor_name(actor_name, client)
                if actor_name != original_name:
                    log_info(f"Actor name changed: '{original_name}' -> '{actor_name}'")

            params = {
                "blueprint_name": blueprint_name,
                "actor_name": actor_name,
                "location": ensure_floats(location),
                "rotation": ensure_floats(rotation),
                "scale": ensure_floats(scale),
                "blueprint_path": blueprint_path
            }
            response = client.execute_command("spawn_blueprint_actor", params)

            if response.get("status") == "success":
                if "result" in response and isinstance(response["result"], dict):
                    response["result"]["final_name"] = actor_name
                    response["result"]["original_name"] = original_name

            return response

        except Exception as e:
            log_error(f"spawn_blueprint_actor exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def delete_actor(name: str) -> Dict[str, Any]:
        """Delete an actor from the current level by name."""
        try:
            client = get_unreal_client()
            return safe_delete_actor(client, name)
        except Exception as e:
            log_error(f"delete_actor exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def list_level_actors(include_level_instances: bool = True) -> Dict[str, Any]:
        """List all actors in the current level."""
        client = get_unreal_client()
        response = client.execute_command("list_level_actors", {
            "include_level_instances": include_level_instances
        }, log_errors=True)
        return response

    @mcp.tool()
    def set_actor_transform(
        name: str,
        location: Optional[List[float]] = None,
        rotation: Optional[List[float]] = None,
        scale: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """Update an actor's transform."""
        if err := validate_vectors("set_actor_transform", log_error, [
            (location, "location", validate_vector3),
            (rotation, "rotation", validate_vector3),
            (scale, "scale", validate_vector3)
        ], optional=True):
            return err

        params = {"name": name}
        if location is not None:
            params["location"] = ensure_floats(location)
        if rotation is not None:
            params["rotation"] = ensure_floats(rotation)
        if scale is not None:
            params["scale"] = ensure_floats(scale)

        return get_unreal_client().execute_command("set_actor_transform", params)

    @mcp.tool()
    def get_actor_properties(name: str) -> Dict[str, Any]:
        """Retrieve all properties of an actor by name."""
        return get_unreal_client().execute_command("get_actor_properties", {"name": name})

    @mcp.tool()
    def set_actor_property(
        name: str,
        property_name: str,
        property_value: Any
    ) -> Dict[str, Any]:
        """Set a specific property on an actor."""
        return get_unreal_client().execute_command("set_actor_property", {
            "name": name,
            "property_name": property_name,
            "property_value": property_value
        })

    # =========================================================================
    # Material Tools
    # =========================================================================

    @mcp.tool()
    def create_material(
        material_name: str,
        color: List[float] = [1.0, 0.0, 0.0],
        material_path: str = "/Game/Materials/"
    ) -> Dict[str, Any]:
        """Create a material with a base color."""
        if error := validate_color(color, "color"):
            log_error(f"create_material validation failed: {error}")
            return create_error_response(error)

        try:
            return get_unreal_client().execute_command("create_material", {
                "material_name": material_name,
                "color": ensure_floats(color),
                "material_path": material_path
            })
        except Exception as e:
            log_error(f"create_material exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def apply_material_to_actor(
        actor_name: str,
        material_path: str,
        material_slot: int = 0
    ) -> Dict[str, Any]:
        """Apply material to a level actor instance."""
        return get_unreal_client().execute_command("apply_material_to_actor", {
            "actor_name": actor_name,
            "material_path": material_path,
            "material_slot": material_slot
        })

    @mcp.tool()
    def get_actor_material_info(actor_name: str) -> Dict[str, Any]:
        """Get material info for a level actor instance."""
        return get_unreal_client().execute_command("get_actor_material_info", {
            "actor_name": actor_name
        })

    # =========================================================================
    # Actor Search Tools
    # =========================================================================

    @mcp.tool()
    def search_actors(
        pattern: str,
        class_filter: str = "",
        limit: int = 100,
        include_level_instances: bool = True,
        level_instance_filter: str = ""
    ) -> Dict[str, Any]:
        """Search actors by name pattern in the current level."""
        params = {
            "pattern": pattern,
            "class_filter": class_filter,
            "limit": limit,
            "include_level_instances": include_level_instances
        }
        if level_instance_filter:
            params["level_instance_filter"] = level_instance_filter

        response = get_unreal_client().execute_command("search_actors", params)

        if response.get("status") == "success":
            actors = response.get("result", {}).get("actors", [])
            if len(actors) == 0:
                response["suggestion"] = "Infer English keywords from user request and retry."

        return response

    # =========================================================================
    # Asset Search Tools
    # =========================================================================

    @mcp.tool()
    def search_assets(
        name: str,
        search_scope: Literal["asset", "class", "all"],
        object_type: Optional[str] = None,
        base_class: Optional[str] = None,
        search_path: str = "/",
        limit: int = 50
    ) -> Dict[str, Any]:
        """Search any asset type (Level, DataTable, Blueprint, Material, Mesh, etc.) in Content Browser.
        Searches all paths including plugins by default. Filter by object_type (UClass name) or use base_class to find derived classes."""
        params = {
            "name": name,
            "search_scope": search_scope,
            "search_path": search_path,
            "limit": limit
        }
        if object_type:
            params["object_type"] = object_type
        if base_class:
            params["base_class"] = base_class

        response = get_unreal_client().execute_command("search_assets", params)

        if response.get("status") == "success":
            result = response.get("result", {})
            assets = result.get("assets", [])
            classes = result.get("classes", [])
            if len(assets) == 0 and len(classes) == 0:
                response["suggestion"] = "Infer English keywords from user request and retry."

        return response

    @mcp.tool()
    def list_folder_assets(
        folder_path: str,
        asset_type: Optional[str] = None,
        recursive: bool = False,
        limit: int = 100
    ) -> Dict[str, Any]:
        """List assets in a Content Browser folder."""
        params = {
            "folder_path": folder_path,
            "recursive": recursive,
            "limit": limit
        }
        if asset_type:
            params["asset_type"] = asset_type

        return get_unreal_client().execute_command("list_folder_assets", params)

    # =========================================================================
    # World Partition Tools
    # =========================================================================

    @mcp.tool()
    def get_world_partition_info() -> Dict[str, Any]:
        """Get World Partition status for the current level."""
        return get_unreal_client().execute_command("get_world_partition_info", {})

    @mcp.tool()
    def search_actors_in_region(
        x: float = 0.0,
        y: float = 0.0,
        z: float = 0.0,
        radius: float = 10000.0,
        class_filter: str = "",
        limit: int = 100
    ) -> Dict[str, Any]:
        """Search actors within a spherical region."""
        return get_unreal_client().execute_command("search_actors_in_region", {
            "x": float(x),
            "y": float(y),
            "z": float(z),
            "radius": float(radius),
            "class_filter": class_filter,
            "limit": limit
        })

    @mcp.tool()
    def load_actor_by_guid(guid: str) -> Dict[str, Any]:
        """Load an actor by GUID in World Partition map."""
        return get_unreal_client().execute_command("load_actor_by_guid", {"guid": guid})

    @mcp.tool()
    def set_region_loaded(
        loaded: bool,
        x: float,
        y: float,
        z: float,
        radius: float
    ) -> Dict[str, Any]:
        """Set load state of actors in a World Partition region."""
        return get_unreal_client().execute_command("set_region_loaded", {
            "loaded": loaded,
            "x": float(x),
            "y": float(y),
            "z": float(z),
            "radius": float(radius)
        })

    # =========================================================================
    # Level Instance Tools
    # =========================================================================

    @mcp.tool()
    def list_level_instances() -> Dict[str, Any]:
        """List all Level Instance actors in the current level."""
        client = get_unreal_client()
        return client.execute_command("list_level_instances", {})

    @mcp.tool()
    def get_level_instance_actors(level_instance_name: str) -> Dict[str, Any]:
        """Get all actors inside a specific Level Instance."""
        client = get_unreal_client()
        return client.execute_command("get_level_instance_actors", {
            "level_instance_name": level_instance_name
        })

    # =========================================================================
    # GameplayTag Tools
    # =========================================================================

    @mcp.tool()
    def list_gameplay_tags(
        prefix: str = "",
        max_depth: int = 5,
        limit: int = 100
    ) -> Dict[str, Any]:
        """List registered GameplayTags. Useful for tag validation and discovery."""
        return get_unreal_client().execute_command("list_gameplay_tags", {
            "prefix": prefix,
            "max_depth": int(max_depth),
            "limit": int(limit)
        })

    # =========================================================================
    # Utility Tools
    # =========================================================================

    @mcp.tool()
    def get_connection_status() -> Dict[str, Any]:
        """Get connection status to Unreal Engine."""
        client = get_unreal_client()
        if not client.ping():
            return {"connected": False, "message": "Failed to ping Unreal Engine"}
        return client.get_connection_status()

    log_info("Editor tools registered successfully")
