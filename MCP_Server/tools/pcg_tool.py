"""
PCG Tools - Procedural Content Generation graph manipulation.

Creates and modifies PCG Graphs for procedural world generation.
For Blueprint-based PCG setup, use blueprint_tool to add PCGComponent first.
"""
from typing import List, Dict, Any, Optional, Literal
from mcp.server.fastmcp import FastMCP
from ..unreal_client import get_unreal_client
from ..utils import (
    log_info, log_error,
    validate_vector2, ensure_floats, create_error_response,
    validate_vectors
)


def register_pcg_tools(mcp: FastMCP):
    """Register PCG tools with MCP server"""

    # =========================================================================
    # PCG Graph Asset Tools
    # =========================================================================

    @mcp.tool()
    def create_pcg_graph(
        name: str,
        path: str
    ) -> Dict[str, Any]:
        """Create a PCG Graph asset."""
        try:
            return get_unreal_client().execute_command("create_pcg_graph", {
                "name": name,
                "path": path
            })
        except Exception as e:
            log_error(f"create_pcg_graph exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def analyze_pcg_graph(
        graph_name: str,
        graph_path: str
    ) -> Dict[str, Any]:
        """Analyze PCG Graph structure. Returns all nodes with their pins and connections."""
        try:
            return get_unreal_client().execute_command("analyze_pcg_graph", {
                "graph_name": graph_name,
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"analyze_pcg_graph exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def set_pcg_graph_to_component(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str,
        graph_name: str,
        graph_path: str
    ) -> Dict[str, Any]:
        """Assign a PCG Graph to a PCGComponent in a Blueprint."""
        try:
            return get_unreal_client().execute_command("set_pcg_graph_to_component", {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "graph_name": graph_name,
                "blueprint_path": blueprint_path,
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"set_pcg_graph_to_component exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    # =========================================================================
    # PCG Node Creation Tools
    # =========================================================================

    @mcp.tool()
    def add_pcg_sampler_node(
        graph_name: str,
        graph_path: str,
        sampler_type: Literal["Surface", "Spline", "Mesh", "Volume", "Landscape"],
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add a sampler node."""
        try:
            if err := validate_vectors("add_pcg_sampler_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_sampler_node", {
                "graph_name": graph_name,
                "sampler_type": sampler_type,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_sampler_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def add_pcg_filter_node(
        graph_name: str,
        graph_path: str,
        filter_type: Literal["Density", "Bounds", "Point", "SelfPruning"],
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add a filter node."""
        try:
            if err := validate_vectors("add_pcg_filter_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_filter_node", {
                "graph_name": graph_name,
                "filter_type": filter_type,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_filter_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def add_pcg_transform_node(
        graph_name: str,
        graph_path: str,
        transform_type: Literal["Transform", "Projection", "NormalToDensity", "BoundsModifier"],
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add a transform node."""
        try:
            if err := validate_vectors("add_pcg_transform_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_transform_node", {
                "graph_name": graph_name,
                "transform_type": transform_type,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_transform_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def add_pcg_spawner_node(
        graph_name: str,
        graph_path: str,
        spawner_type: Literal["StaticMesh", "Actor", "CopyPoints"],
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add a spawner node."""
        try:
            if err := validate_vectors("add_pcg_spawner_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_spawner_node", {
                "graph_name": graph_name,
                "spawner_type": spawner_type,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_spawner_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def add_pcg_attribute_node(
        graph_name: str,
        graph_path: str,
        attribute_type: Literal["Create", "Delete", "Copy", "Rename", "Metadata", "Noise", "PropertyToParams"],
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add an attribute node."""
        try:
            if err := validate_vectors("add_pcg_attribute_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_attribute_node", {
                "graph_name": graph_name,
                "attribute_type": attribute_type,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_attribute_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def add_pcg_flow_control_node(
        graph_name: str,
        graph_path: str,
        flow_type: Literal["Branch", "Collapse", "Merge", "Difference", "Intersection", "Subgraph", "Loop"],
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add a flow control node."""
        try:
            if err := validate_vectors("add_pcg_flow_control_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_flow_control_node", {
                "graph_name": graph_name,
                "flow_type": flow_type,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_flow_control_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def add_pcg_generic_node(
        graph_name: str,
        graph_path: str,
        node_class: str,
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add any PCG node by settings class name."""
        try:
            if err := validate_vectors("add_pcg_generic_node", log_error, [
                (node_position, "node_position", validate_vector2)
            ]):
                return err

            return get_unreal_client().execute_command("add_pcg_generic_node", {
                "graph_name": graph_name,
                "node_class": node_class,
                "node_position": ensure_floats(node_position),
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"add_pcg_generic_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    # =========================================================================
    # PCG Node Search Tools
    # =========================================================================

    @mcp.tool()
    def list_pcg_nodes(
        graph_name: str,
        graph_path: str,
        query: str = "",
        settings_class: str = ""
    ) -> Dict[str, Any]:
        """List nodes in PCG Graph with optional filters by name or settings class."""
        try:
            return get_unreal_client().execute_command("list_pcg_nodes", {
                "graph_name": graph_name,
                "query": query,
                "settings_class": settings_class,
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"list_pcg_nodes exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    # =========================================================================
    # PCG Node Connection Tools
    # =========================================================================

    @mcp.tool()
    def connect_pcg_nodes(
        graph_name: str,
        graph_path: str,
        source_node_id: str,
        target_node_id: str,
        source_pin: str = "",
        target_pin: str = ""
    ) -> Dict[str, Any]:
        """Connect two PCG nodes. If pins not specified, uses first available."""
        try:
            return get_unreal_client().execute_command("connect_pcg_nodes", {
                "graph_name": graph_name,
                "source_node_id": source_node_id,
                "target_node_id": target_node_id,
                "source_pin": source_pin,
                "target_pin": target_pin,
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"connect_pcg_nodes exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def disconnect_pcg_nodes(
        graph_name: str,
        graph_path: str,
        node_id: str,
        pin_name: str
    ) -> Dict[str, Any]:
        """Disconnect all edges from a specific pin."""
        try:
            return get_unreal_client().execute_command("disconnect_pcg_nodes", {
                "graph_name": graph_name,
                "node_id": node_id,
                "pin_name": pin_name,
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"disconnect_pcg_nodes exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    # =========================================================================
    # PCG Node Deletion Tools
    # =========================================================================

    @mcp.tool()
    def delete_pcg_node(
        graph_name: str,
        graph_path: str,
        node_id: str
    ) -> Dict[str, Any]:
        """Delete a node from PCG Graph."""
        try:
            return get_unreal_client().execute_command("delete_pcg_node", {
                "graph_name": graph_name,
                "node_id": node_id,
                "graph_path": graph_path
            })
        except Exception as e:
            log_error(f"delete_pcg_node exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    log_info("PCG tools registered successfully")
