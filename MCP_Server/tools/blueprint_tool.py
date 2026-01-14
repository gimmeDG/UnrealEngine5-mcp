"""
Blueprint Tools - Blueprint asset creation and component management.

Modifies Blueprint assets (CDO), affecting all instances spawned from it.
For level actor instances, use editor_tool instead.
"""
from typing import List, Dict, Any, Optional, Literal, Union, Annotated
from pydantic import BaseModel, Field, model_validator, TypeAdapter
from mcp.server.fastmcp import FastMCP
from ..unreal_client import get_unreal_client
from ..utils import (
    log_info, log_error,
    validate_vector2, validate_vector3, ensure_floats, create_error_response,
    validate_vectors
)
from ..helpers import get_unique_actor_name


def _search_parent_class_candidates(client, parent_class: str, base_class: str) -> List[str]:
    """Search for parent class candidates when class not found."""
    search_result = client.execute_command("search_assets", {
        "name": parent_class,
        "search_scope": "all",
        "base_class": base_class,
        "search_path": "/",
        "limit": 5
    })
    candidates = []
    if search_result.get("status") == "success":
        result = search_result.get("result", {})
        for c in result.get("classes", []):
            if c.get("path"):
                candidates.append(c.get("path"))
        for a in result.get("assets", []):
            if a.get("path"):
                candidates.append(a.get("path"))
    return candidates


def _add_parent_class_suggestion(response: Dict[str, Any], parent_class: str, candidates: List[str]):
    """Add suggestion to response when parent class not found."""
    if candidates:
        response["suggested_paths"] = candidates
        response["suggestion"] = (
            f"Parent class '{parent_class}' not found. "
            "Please confirm which path to use from suggested_paths."
        )
    else:
        response["suggestion"] = (
            f"Parent class '{parent_class}' not found and no candidates discovered. "
            "Please provide the full asset path."
        )


# =============================================================================
# GAS Ability Graph Node Types
# =============================================================================

class _NodeBase(BaseModel):
    """Base fields shared by all node types"""
    name: str = Field(..., description="Unique node name for referencing in connections")
    position: Optional[List[float]] = Field(None, description="[X, Y] position, optional if auto_layout=True")


class CallFunctionNode(_NodeBase):
    """Call a Blueprint/C++ function"""
    type: Literal["CallFunction"] = "CallFunction"
    function_name: str = Field(..., description="Function name (e.g., 'K2_EndAbility', 'PrintString')")
    target_class: Optional[str] = Field(None, description="Target class if calling on specific class")


class AbilityTaskNode(_NodeBase):
    """Create a GAS AbilityTask (async latent action)"""
    type: Literal["AbilityTask"] = "AbilityTask"
    task_class: str = Field(..., description="AbilityTask class (e.g., 'UAbilityTask_PlayMontageAndWait')")
    function_name: str = Field(..., description="Static factory function (e.g., 'CreatePlayMontageAndWaitProxy')")


class SpawnActorNode(_NodeBase):
    """Spawn an actor in the world"""
    type: Literal["SpawnActor"] = "SpawnActor"
    actor_class: Optional[str] = Field(None, description="Actor class to spawn (can be set via pin)")


class BranchNode(_NodeBase):
    """Conditional branch (if/else)"""
    type: Literal["Branch"] = "Branch"


class SequenceNode(_NodeBase):
    """Execute multiple outputs in sequence"""
    type: Literal["Sequence"] = "Sequence"


class SelfNode(_NodeBase):
    """Reference to self (the owning ability)"""
    type: Literal["Self"] = "Self"


class VariableGetNode(_NodeBase):
    """Get a Blueprint variable value"""
    type: Literal["VariableGet"] = "VariableGet"
    variable_name: str = Field(..., description="Blueprint variable name to read")


class VariableSetNode(_NodeBase):
    """Set a Blueprint variable value"""
    type: Literal["VariableSet"] = "VariableSet"
    variable_name: str = Field(..., description="Blueprint variable name to write")


class CastNode(_NodeBase):
    """Cast to a specific class"""
    type: Literal["Cast"] = "Cast"
    target_class: str = Field(..., description="Class to cast to (e.g., 'Character', 'PlayerController')")


class MakeStructNode(_NodeBase):
    """Construct a struct from individual values"""
    type: Literal["MakeStruct"] = "MakeStruct"
    struct_type: str = Field(..., description="Struct type (e.g., 'Transform', 'GameplayEventData')")


class BreakStructNode(_NodeBase):
    """Break a struct into individual values"""
    type: Literal["BreakStruct"] = "BreakStruct"
    struct_type: str = Field(..., description="Struct type (e.g., 'Transform', 'GameplayEventData')")


class EntryNode(_NodeBase):
    """Reference existing function entry point (ActivateAbility entry)"""
    type: Literal["Entry"] = "Entry"


class CommitAbilityNode(_NodeBase):
    """Commit ability - consume cost and start cooldown. Returns bool success."""
    type: Literal["CommitAbility"] = "CommitAbility"


class EndAbilityNode(_NodeBase):
    """End the ability. Call when ability is complete or cancelled."""
    type: Literal["EndAbility"] = "EndAbility"


class CheckCooldownNode(_NodeBase):
    """Check if ability is on cooldown. Returns bool (true = can activate)."""
    type: Literal["CheckCooldown"] = "CheckCooldown"


class CheckCostNode(_NodeBase):
    """Check if ability cost can be paid. Returns bool (true = can pay)."""
    type: Literal["CheckCost"] = "CheckCost"


class ApplyEffectNode(_NodeBase):
    """Apply GameplayEffect to ability owner (self)"""
    type: Literal["ApplyEffect"] = "ApplyEffect"


class ApplyEffectToTargetNode(_NodeBase):
    """Apply GameplayEffect to target actor/data"""
    type: Literal["ApplyEffectToTarget"] = "ApplyEffectToTarget"


class PlayMontageNode(_NodeBase):
    """PlayMontageAndWait task - play animation montage asynchronously"""
    type: Literal["PlayMontage"] = "PlayMontage"


class WaitGameplayEventNode(_NodeBase):
    """WaitGameplayEvent task - wait for gameplay event by tag"""
    type: Literal["WaitGameplayEvent"] = "WaitGameplayEvent"


class ForEachLoopNode(_NodeBase):
    """ForEach loop macro - iterate over array elements"""
    type: Literal["ForEachLoop"] = "ForEachLoop"


class ForEachLoopWithBreakNode(_NodeBase):
    """ForEach loop with break capability"""
    type: Literal["ForEachLoopWithBreak"] = "ForEachLoopWithBreak"


AbilityGraphNode = Annotated[
    Union[
        CallFunctionNode, AbilityTaskNode, SpawnActorNode,
        BranchNode, SequenceNode, SelfNode,
        VariableGetNode, VariableSetNode, CastNode,
        MakeStructNode, BreakStructNode,
        EntryNode, CommitAbilityNode, EndAbilityNode, CheckCooldownNode, CheckCostNode,
        ApplyEffectNode, ApplyEffectToTargetNode,
        PlayMontageNode, WaitGameplayEventNode,
        ForEachLoopNode, ForEachLoopWithBreakNode,
    ],
    Field(discriminator="type")
]


class NodeConnection(BaseModel):
    """Connection between two node pins"""
    source: str = Field(..., description="Source: 'NodeName.PinName' (e.g., 'Entry.then', 'GetHealth.ReturnValue')")
    target: str = Field(..., description="Target: 'NodeName.PinName' (e.g., 'EndAbility.execute')")

    @model_validator(mode='after')
    def validate_pin_format(self):
        if '.' not in self.source:
            raise ValueError(f"source '{self.source}' must be 'NodeName.PinName' format")
        if '.' not in self.target:
            raise ValueError(f"target '{self.target}' must be 'NodeName.PinName' format")
        return self


class PinDefault(BaseModel):
    """Default value for a node pin"""
    node: str = Field(..., description="Node name")
    pin: str = Field(..., description="Pin name")
    value: str = Field(..., description="Default value as string")


_NodeAdapter = TypeAdapter(AbilityGraphNode)


def register_blueprint_tools(mcp: FastMCP):
    """Register Blueprint manipulation tools with MCP server"""

    # =========================================================================
    # Core Blueprint Tools
    # =========================================================================

    @mcp.tool()
    def create_blueprint(
        name: str,
        blueprint_path: str,
        parent_class: str = "Actor",
        auto_unique_name: bool = True
    ) -> Dict[str, Any]:
        """Create a Blueprint asset. parent_class must be AActor subclass."""
        try:
            client = get_unreal_client()
            original_name = name

            if auto_unique_name:
                name = get_unique_actor_name(name, client)
                if name != original_name:
                    log_info(f"Blueprint name changed: '{original_name}' -> '{name}'")

            params = {
                "name": name,
                "parent_class": parent_class,
                "path": blueprint_path
            }
            response = client.execute_command("create_blueprint", params)

            if response.get("status") == "success":
                if "result" in response and isinstance(response["result"], dict):
                    response["result"]["final_name"] = name
                    response["result"]["original_name"] = original_name

            return response

        except Exception as e:
            log_error(f"create_blueprint exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    @mcp.tool()
    def compile_blueprint(
        blueprint_name: str,
        blueprint_path: str,
        validate_only: bool = False
    ) -> Dict[str, Any]:
        """Compile a Blueprint. Validates graph first, returns validation_issues on failure."""
        return get_unreal_client().execute_command("compile_blueprint", {
            "blueprint_name": blueprint_name,
            "blueprint_path": blueprint_path,
            "validate_only": validate_only
        })

    # =========================================================================
    # Component Tools
    # =========================================================================

    @mcp.tool()
    def add_component_to_blueprint(
        blueprint_name: str,
        blueprint_path: str,
        component_type: str,
        component_name: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0],
        scale: List[float] = [1.0, 1.0, 1.0]
    ) -> Dict[str, Any]:
        """Add a component to a Blueprint."""
        if err := validate_vectors("add_component_to_blueprint", log_error, [
            (location, "location", validate_vector3),
            (rotation, "rotation", validate_vector3),
            (scale, "scale", validate_vector3)
        ]):
            return err

        return get_unreal_client().execute_command("add_component_to_blueprint", {
            "blueprint_name": blueprint_name,
            "component_type": component_type,
            "component_name": component_name,
            "location": ensure_floats(location),
            "rotation": ensure_floats(rotation),
            "scale": ensure_floats(scale),
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def set_component_property(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str,
        property_name: str,
        value: str
    ) -> Dict[str, Any]:
        """Set a property on a Blueprint component. Searches SCS, CDO, and inherited components."""
        return get_unreal_client().execute_command("set_component_property", {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "property_name": property_name,
            "value": value,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def set_physics_properties(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str,
        simulate_physics: bool = True,
        mass: float = 100.0,
        enable_gravity: bool = True
    ) -> Dict[str, Any]:
        """Configure physics settings for a Blueprint component."""
        return get_unreal_client().execute_command("set_physics_properties", {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "simulate_physics": simulate_physics,
            "mass": float(mass),
            "enable_gravity": enable_gravity,
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # Material Tools
    # =========================================================================

    @mcp.tool()
    def apply_material_to_blueprint(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str,
        material_path: str,
        material_slot: int = 0
    ) -> Dict[str, Any]:
        """Apply material to Blueprint CDO."""
        return get_unreal_client().execute_command("apply_material_to_blueprint", {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "material_path": material_path,
            "material_slot": material_slot,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def get_blueprint_material_info(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str
    ) -> Dict[str, Any]:
        """Get material info for Blueprint CDO component."""
        return get_unreal_client().execute_command("get_blueprint_material_info", {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def set_mesh_material_color(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str,
        color: List[float],
        material_path: str = "/Engine/BasicShapes/BasicShapeMaterial",
        parameter_name: str = "BaseColor",
        material_slot: int = 0
    ) -> Dict[str, Any]:
        """Set material color on a mesh component."""
        try:
            if not isinstance(color, (list, tuple)):
                log_error("set_mesh_material_color: color must be a list")
                return create_error_response("Invalid color format. Must be a list of 4 float values [R, G, B, A].")

            if len(color) == 3:
                color = list(color) + [1.0]
            elif len(color) != 4:
                return create_error_response("Invalid color format. Must be a list of 3 or 4 float values.")

            color = [float(min(1.0, max(0.0, val))) for val in color]

            client = get_unreal_client()
            response_base = client.execute_command("set_mesh_material_color", {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "color": color,
                "material_path": material_path,
                "parameter_name": "BaseColor",
                "material_slot": material_slot,
                "blueprint_path": blueprint_path
            }, log_errors=False)

            response_color = client.execute_command("set_mesh_material_color", {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "color": color,
                "material_path": material_path,
                "parameter_name": "Color",
                "material_slot": material_slot,
                "blueprint_path": blueprint_path
            }, log_errors=False)

            base_success = response_base and response_base.get("status") == "success"
            color_success = response_color and response_color.get("status") == "success"

            if base_success or color_success:
                log_info(f"set_mesh_material_color: Color applied to {blueprint_name}.{component_name}")
                return {
                    "status": "success",
                    "result": {
                        "component": component_name,
                        "material_slot": material_slot,
                        "color": color,
                        "base_color_result": response_base,
                        "color_result": response_color
                    }
                }
            else:
                log_error(f"set_mesh_material_color: Failed. BaseColor: {response_base}, Color: {response_color}")
                return create_error_response(
                    f"Failed to set color. BaseColor: {response_base}, Color: {response_color}"
                )

        except Exception as e:
            log_error(f"set_mesh_material_color exception: {e}", include_traceback=True)
            return create_error_response(str(e))

    # =========================================================================
    # Variable Tools
    # =========================================================================

    @mcp.tool()
    def add_blueprint_variable(
        blueprint_name: str,
        blueprint_path: str,
        variable_name: str,
        variable_type: str,
        sub_type: Optional[str] = None,
        default_value: Any = None,
        metadata: Optional[Dict[str, str]] = None
    ) -> Dict[str, Any]:
        """Add Blueprint variable with UPROPERTY specifiers via metadata dict.

        Supported metadata keys (unknown keys will cause error):
        - Visibility: BlueprintReadOnly, BlueprintReadWrite, VisibleAnywhere, VisibleDefaultsOnly, VisibleInstanceOnly
        - Editing: EditAnywhere, EditDefaultsOnly, EditInstanceOnly, AdvancedDisplay
        - Spawning: ExposeOnSpawn, Instanced
        - Replication: Replicated, ReplicatedUsing (value=FuncName), NotReplicated, ReplicationCondition (InitialOnly/OwnerOnly/etc)
        - Serialization: SaveGame, Transient, DuplicateTransient, SkipSerialization
        - Delegates: BlueprintAssignable, BlueprintCallable, BlueprintAuthorityOnly
        - Editor: Category, Interp, SimpleDisplay, NoClear, NonTransactional
        - MetaData: Tooltip, DisplayName, ClampMin, ClampMax, UIMin, UIMax, MakeEditWidget, AllowPrivateAccess, GetOptions, EditCondition, EditConditionHides, Units, TitleProperty, NoResetToDefault, HideAlphaChannel
        """
        params = {
            "blueprint_name": blueprint_name,
            "variable_name": variable_name,
            "variable_type": variable_type,
            "default_value": default_value,
            "blueprint_path": blueprint_path
        }
        if sub_type:
            params["sub_type"] = sub_type
        if metadata:
            params["metadata"] = metadata
        return get_unreal_client().execute_command("add_blueprint_variable", params)

    @mcp.tool()
    def add_blueprint_variable_node(
        blueprint_name: str,
        blueprint_path: str,
        variable_name: str,
        node_type: Literal["get", "set"],
        node_position: List[float] = [0.0, 0.0],
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """Get/Set node for Blueprint's own variables (Variables panel).

        Uses SetSelfMember internally - no Target pin, references self automatically.
        For UClass properties, use add_property_get_set_node instead.
        """
        if err := validate_vectors("add_blueprint_variable_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        cmd_params = {
            "blueprint_name": blueprint_name,
            "variable_name": variable_name,
            "node_type": node_type,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        }
        if graph_name:
            cmd_params["graph_name"] = graph_name

        return get_unreal_client().execute_command("add_blueprint_variable_node", cmd_params)

    # =========================================================================
    # Node Creation Tools
    # =========================================================================

    @mcp.tool()
    def add_blueprint_event_node(
        blueprint_name: str,
        blueprint_path: str,
        event_name: str,
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add engine/parent event override node. For user-defined events use add_custom_event_node."""
        if err := validate_vectors("add_blueprint_event_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ]):
            return err

        return get_unreal_client().execute_command("add_blueprint_event_node", {
            "blueprint_name": blueprint_name,
            "event_name": event_name,
            "node_position": ensure_floats(node_position),
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def add_custom_event_node(
        blueprint_name: str,
        blueprint_path: str,
        event_name: str,
        action: Literal["define", "call"],
        node_position: List[float] = [0.0, 0.0],
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """Add custom event node. 'define' creates new event, 'call' invokes existing one."""
        if err := validate_vectors("add_custom_event_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        params = {
            "blueprint_name": blueprint_name,
            "event_name": event_name,
            "action": action,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        }
        if graph_name:
            params["graph_name"] = graph_name

        return get_unreal_client().execute_command("add_custom_event_node", params)

    @mcp.tool()
    def add_blueprint_function_node(
        blueprint_name: str,
        blueprint_path: str,
        function_name: str,
        target_class: str,
        params: Optional[Dict[str, Any]] = None,
        node_position: List[float] = [0.0, 0.0],
        graph_name: Optional[str] = None,
        auto_connect_self: bool = False
    ) -> Dict[str, Any]:
        """Add UFunction call node. Searches function in target_class first, then Blueprint's GeneratedClass."""
        if err := validate_vectors("add_blueprint_function_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ]):
            return err

        cmd_params = {
            "blueprint_name": blueprint_name,
            "function_name": function_name,
            "target_class": target_class,
            "params": params or {},
            "node_position": ensure_floats(node_position),
            "blueprint_path": blueprint_path,
            "auto_connect_self": auto_connect_self
        }
        if graph_name:
            cmd_params["graph_name"] = graph_name

        return get_unreal_client().execute_command("add_blueprint_function_node", cmd_params)

    @mcp.tool()
    def add_blueprint_flow_control_node(
        blueprint_name: str,
        blueprint_path: str,
        control_type: str,
        node_position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """Add flow control node by control_type.

        Supported: branch, sequence, forloop, foreachloop, foreachloopwithbreak,
                   whileloop, doonce, multigate, flipflop, gate

        Note: IsValid is NOT a flow control node. Use add_blueprint_function_node
        with function_name='IsValid', target_class='KismetSystemLibrary' instead,
        then connect its output to a Branch node.
        """
        if err := validate_vectors("add_blueprint_flow_control_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        return get_unreal_client().execute_command("add_blueprint_flow_control_node", {
            "blueprint_name": blueprint_name,
            "control_type": control_type,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def add_function_override(
        blueprint_name: str,
        function_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Create a function override graph. Returns entry node ID."""
        return get_unreal_client().execute_command("add_function_override", {
            "blueprint_name": blueprint_name,
            "function_name": function_name,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def add_blueprint_input_action_node(
        blueprint_name: str,
        blueprint_path: str,
        action_name: str,
        node_position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """Add Enhanced Input action event node."""
        if err := validate_vectors("add_blueprint_input_action_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        return get_unreal_client().execute_command("add_blueprint_input_action_node", {
            "blueprint_name": blueprint_name,
            "action_name": action_name,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def add_component_getter_node(
        blueprint_name: str,
        blueprint_path: str,
        component_name: str,
        node_position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """Get node for Blueprint's own components (Components panel).

        Uses SetSelfMember internally - no Target pin, references self automatically.
        For accessing properties of this component, use add_property_get_set_node.
        """
        if err := validate_vectors("add_component_getter_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        return get_unreal_client().execute_command("add_component_getter_node", {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def add_property_get_set_node(
        blueprint_name: str,
        blueprint_path: str,
        owner_class: str,
        property_name: str,
        node_type: Literal["get", "set"],
        node_position: List[float] = [0.0, 0.0],
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """Get/Set node for UClass properties.

        Uses SetExternalMember internally - creates Target pin that must be connected.
        Requires owner_class to specify which class owns the property.

        Workflow:
        1. Get component/object reference (add_component_getter_node or other)
        2. Create property node with this tool
        3. Connect reference output to this node's Target pin
        """
        if err := validate_vectors("add_property_get_set_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        cmd_params = {
            "blueprint_name": blueprint_name,
            "owner_class": owner_class,
            "property_name": property_name,
            "node_type": node_type,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        }
        if graph_name:
            cmd_params["graph_name"] = graph_name

        return get_unreal_client().execute_command("add_property_get_set_node", cmd_params)

    @mcp.tool()
    def add_blueprint_self_reference(
        blueprint_name: str,
        blueprint_path: str,
        node_position: Optional[List[float]] = None,
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """Add a 'Get Self' node."""
        if err := validate_vectors("add_blueprint_self_reference", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        cmd_params = {
            "blueprint_name": blueprint_name,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        }
        if graph_name:
            cmd_params["graph_name"] = graph_name

        return get_unreal_client().execute_command("add_blueprint_self_reference", cmd_params)

    @mcp.tool()
    def add_blueprint_generic_node(
        blueprint_name: str,
        blueprint_path: str,
        node_class: str,
        node_position: List[float] = [0.0, 0.0],
        graph_name: str = "EventGraph",
        actor_class: Optional[str] = None,
        object_class: Optional[str] = None,
        struct_type: Optional[str] = None,
        target_type: Optional[str] = None,
        enum: Optional[str] = None
    ) -> Dict[str, Any]:
        """Add K2Node by class name. Use actor_class, object_class, struct_type,
        target_type, or enum param to set class pin for supported nodes."""
        if err := validate_vectors("add_blueprint_generic_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        params = {
            "blueprint_name": blueprint_name,
            "node_class": node_class,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "graph_name": graph_name,
            "blueprint_path": blueprint_path
        }
        if actor_class:
            params["actor_class"] = actor_class
        if object_class:
            params["object_class"] = object_class
        if struct_type:
            params["struct_type"] = struct_type
        if target_type:
            params["target_type"] = target_type
        if enum:
            params["enum"] = enum

        return get_unreal_client().execute_command("add_blueprint_generic_node", params)

    # =========================================================================
    # Node Search Tools
    # =========================================================================

    @mcp.tool()
    def list_blueprint_nodes(
        blueprint_name: str,
        blueprint_path: str,
        node_type: Optional[str] = None,
        event_name: Optional[str] = None,
        node_title: Optional[str] = None,
        node_class: Optional[str] = None,
        has_unconnected_pins: bool = False,
        has_unconnected_exec_pins: bool = False,
        has_unconnected_data_pins: bool = False,
        sort_by: Optional[str] = None,
        limit: int = 50,
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """List nodes in a Blueprint graph with optional filters. Returns node_id, pins with connection status."""
        params = {
            "blueprint_name": blueprint_name,
            "blueprint_path": blueprint_path,
            "limit": limit
        }
        if graph_name:
            params["graph_name"] = graph_name
        if node_type:
            params["node_type"] = node_type
        if event_name:
            params["event_name"] = event_name
        if node_title:
            params["node_title"] = node_title
        if node_class:
            params["node_class"] = node_class
        if has_unconnected_pins:
            params["has_unconnected_pins"] = True
        if has_unconnected_exec_pins:
            params["has_unconnected_exec_pins"] = True
        if has_unconnected_data_pins:
            params["has_unconnected_data_pins"] = True
        if sort_by:
            params["sort_by"] = sort_by

        return get_unreal_client().execute_command("list_blueprint_nodes", params)

    # =========================================================================
    # Node Connection Tools
    # =========================================================================

    @mcp.tool()
    def connect_blueprint_nodes(
        blueprint_name: str,
        blueprint_path: str,
        source_pin: str,
        target_pin: str,
        source_node_id: Optional[str] = None,
        target_node_id: Optional[str] = None,
        source_search: Optional[Dict[str, Any]] = None,
        target_search: Optional[Dict[str, Any]] = None,
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """Connect two nodes by pin names. Use node_id or search criteria (node_title, event_name, node_class, newest)."""
        params: Dict[str, Any] = {
            "blueprint_name": blueprint_name,
            "source_pin": source_pin,
            "target_pin": target_pin,
            "blueprint_path": blueprint_path
        }

        if graph_name:
            params["graph_name"] = graph_name

        # GUID mode
        if source_node_id:
            params["source_node_id"] = source_node_id
        # Search mode
        elif source_search:
            params["source_search"] = source_search

        if target_node_id:
            params["target_node_id"] = target_node_id
        elif target_search:
            params["target_search"] = target_search

        return get_unreal_client().execute_command("connect_blueprint_nodes", params)

    @mcp.tool()
    def connect_nodes(
        blueprint_name: str,
        blueprint_path: str,
        source_node_id: str,
        target_node_id: str,
        connect_exec: bool = True,
        connect_data: bool = False
    ) -> Dict[str, Any]:
        """Auto-connect compatible pins between two nodes by GUID. Searches all graphs."""
        return get_unreal_client().execute_command("connect_nodes", {
            "blueprint_name": blueprint_name,
            "source_node_id": source_node_id,
            "target_node_id": target_node_id,
            "connect_exec": connect_exec,
            "connect_data": connect_data,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def set_pin_default_value(
        blueprint_name: str,
        blueprint_path: str,
        node_id: str,
        pin_name: str,
        value: Any
    ) -> Dict[str, Any]:
        """Set pin default value. Returns value_set and value_changed for verification."""
        return get_unreal_client().execute_command("set_pin_default_value", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "pin_name": pin_name,
            "value": value,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def get_pin_value(
        blueprint_name: str,
        blueprint_path: str,
        node_id: str,
        pin_name: str
    ) -> Dict[str, Any]:
        """Get current pin value. Useful for verifying set_pin_default_value results."""
        return get_unreal_client().execute_command("get_pin_value", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "pin_name": pin_name,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def set_node_property(
        blueprint_name: str,
        blueprint_path: str,
        node_id: str,
        property_path: str,
        value: Any
    ) -> Dict[str, Any]:
        """Set a node property by reflection. Supports nested paths (e.g., 'EventReference.MemberName')."""
        return get_unreal_client().execute_command("set_node_property", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "property_path": property_path,
            "value": value,
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # Organization Tools
    # =========================================================================

    @mcp.tool()
    def add_comment_box(
        blueprint_name: str,
        blueprint_path: str,
        comment_text: str,
        position: List[float] = [0.0, 0.0],
        size: List[float] = [400.0, 200.0]
    ) -> Dict[str, Any]:
        """Add a comment box to Blueprint graph."""
        if err := validate_vectors("add_comment_box", log_error, [
            (position, "position", validate_vector2),
            (size, "size", validate_vector2)
        ]):
            return err

        return get_unreal_client().execute_command("add_comment_box", {
            "blueprint_name": blueprint_name,
            "comment_text": comment_text,
            "position": ensure_floats(position),
            "size": ensure_floats(size),
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # Deletion Tools
    # =========================================================================

    @mcp.tool()
    def delete_blueprint_node(
        blueprint_name: str,
        node_id: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Delete a node from Blueprint graph by node ID."""
        return get_unreal_client().execute_command("delete_blueprint_node", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def delete_blueprint_variable(
        blueprint_name: str,
        variable_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Delete a variable from Blueprint."""
        return get_unreal_client().execute_command("delete_blueprint_variable", {
            "blueprint_name": blueprint_name,
            "variable_name": variable_name,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def delete_component_from_blueprint(
        blueprint_name: str,
        component_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Delete a component from Blueprint."""
        return get_unreal_client().execute_command("delete_component_from_blueprint", {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def disconnect_blueprint_nodes(
        blueprint_name: str,
        node_id: str,
        pin_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Disconnect all links from a specific pin."""
        return get_unreal_client().execute_command("disconnect_blueprint_nodes", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "pin_name": pin_name,
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # Dynamic Pin Management
    # =========================================================================

    @mcp.tool()
    def add_pin(
        blueprint_name: str,
        node_id: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Add a pin to dynamic-pin nodes. Node must support AddInputPin."""
        return get_unreal_client().execute_command("add_pin", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def delete_pin(
        blueprint_name: str,
        node_id: str,
        pin_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Delete a pin from dynamic-pin nodes. Node must support RemoveInputPin."""
        return get_unreal_client().execute_command("delete_pin", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "pin_name": pin_name,
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # GAS (Gameplay Ability System) Tools
    # =========================================================================

    @mcp.tool()
    def create_gameplay_effect(
        name: str,
        parent_class: str = "GameplayEffect",
        duration_policy: Literal["Instant", "HasDuration", "Infinite"] = "Instant",
        duration: float = 0.0,
        period: float = 0.0,
        modifiers: Optional[List[Dict[str, Any]]] = None,
        executions: Optional[List[Dict[str, Any]]] = None,
        granted_tags: Optional[List[str]] = None,
        application_required_tags: Optional[List[str]] = None,
        application_blocked_tags: Optional[List[str]] = None,
        asset_path: str = "/Game/GAS/Effects/"
    ) -> Dict[str, Any]:
        """
        Create a GameplayEffect Blueprint with all properties set at creation time.

        IMPORTANT: All GE properties must be set during creation. They cannot be modified afterward.

        Args:
            name: Effect name (e.g., "GE_HealOverTime")
            parent_class: Parent GE class (default: "GameplayEffect")
            duration_policy: "Instant", "HasDuration", or "Infinite"
            duration: Duration in seconds (for HasDuration policy)
            period: Tick period for periodic effects (e.g., 1.0 for every second)
            modifiers: List of attribute modifiers, each with:
                - attribute: Attribute name (e.g., "Health")
                - operation: "Add", "Multiply", "Divide", or "Override"
                - value: Numeric value
            executions: List of custom calculation executions, each with:
                - calculation_class: UGameplayEffectExecutionCalculation class name or path
                - conditional_effects: (optional) List of effects to apply conditionally
                - calculation_modifiers: (optional) Attribute capture definitions
            granted_tags: Tags granted while effect is active
            application_required_tags: Tags required on target for effect to apply
            application_blocked_tags: Tags that block effect application
            asset_path: Asset save path

        Example executions:
            [{"calculation_class": "MyDamageCalculation"}]
            [{"calculation_class": "/Game/GAS/Calculations/DamageCalc.DamageCalc_C"}]
        """
        params = {
            "name": name,
            "parent_class": parent_class,
            "duration_policy": duration_policy,
            "duration": float(duration),
            "period": float(period),
            "asset_path": asset_path
        }

        if modifiers:
            params["modifiers"] = modifiers
        if executions:
            params["executions"] = executions
        if granted_tags:
            params["granted_tags"] = granted_tags
        if application_required_tags:
            params["application_required_tags"] = application_required_tags
        if application_blocked_tags:
            params["application_blocked_tags"] = application_blocked_tags

        client = get_unreal_client()
        response = client.execute_command("create_gameplay_effect", params)

        if response.get("status") == "error":
            error_msg = response.get("error", "").lower()
            if "parent" in error_msg or "class" in error_msg or "not found" in error_msg:
                candidates = _search_parent_class_candidates(client, parent_class, "GameplayEffect")
                _add_parent_class_suggestion(response, parent_class, candidates)

        return response

    @mcp.tool()
    def create_gameplay_ability(
        name: str,
        parent_class: str = "GameplayAbility",
        auto_setup_lifecycle: bool = False,
        ability_tags: Optional[List[str]] = None,
        cancel_abilities_with_tags: Optional[List[str]] = None,
        block_abilities_with_tags: Optional[List[str]] = None,
        activation_required_tags: Optional[List[str]] = None,
        activation_blocked_tags: Optional[List[str]] = None,
        cost_gameplay_effect: Optional[str] = None,
        cooldown_gameplay_effect: Optional[str] = None,
        instancing_policy: str = "InstancedPerActor",
        net_execution_policy: str = "LocalPredicted",
        asset_path: str = "/Game/GAS/Abilities/"
    ) -> Dict[str, Any]:
        """Create a GameplayAbility Blueprint. Tags must be registered in project."""
        params = {
            "name": name,
            "parent_class": parent_class,
            "auto_setup_lifecycle": auto_setup_lifecycle,
            "instancing_policy": instancing_policy,
            "net_execution_policy": net_execution_policy,
            "asset_path": asset_path
        }

        if ability_tags:
            params["ability_tags"] = ability_tags
        if cancel_abilities_with_tags:
            params["cancel_abilities_with_tags"] = cancel_abilities_with_tags
        if block_abilities_with_tags:
            params["block_abilities_with_tags"] = block_abilities_with_tags
        if activation_required_tags:
            params["activation_required_tags"] = activation_required_tags
        if activation_blocked_tags:
            params["activation_blocked_tags"] = activation_blocked_tags
        if cost_gameplay_effect:
            params["cost_gameplay_effect"] = cost_gameplay_effect
        if cooldown_gameplay_effect:
            params["cooldown_gameplay_effect"] = cooldown_gameplay_effect

        client = get_unreal_client()
        response = client.execute_command("create_gameplay_ability", params)

        if response.get("status") == "error":
            error_msg = response.get("error", "").lower()
            if "parent" in error_msg or "class" in error_msg or "not found" in error_msg:
                candidates = _search_parent_class_candidates(client, parent_class, "GameplayAbility")
                _add_parent_class_suggestion(response, parent_class, candidates)

        return response

    @mcp.tool()
    def add_ability_task_node(
        blueprint_name: str,
        blueprint_path: str,
        task_class: str,
        function_name: str,
        node_position: List[float] = [0.0, 0.0],
        graph_name: Optional[str] = None
    ) -> Dict[str, Any]:
        """Add UAbilityTask latent node. Requires task_class and factory function_name."""
        if err := validate_vectors("add_ability_task_node", log_error, [
            (node_position, "node_position", validate_vector2)
        ], optional=True):
            return err

        params = {
            "blueprint_name": blueprint_name,
            "task_class": task_class,
            "function_name": function_name,
            "node_position": ensure_floats(node_position) if node_position else [0.0, 0.0],
            "blueprint_path": blueprint_path
        }
        if graph_name:
            params["graph_name"] = graph_name

        return get_unreal_client().execute_command("add_ability_task_node", params)

    # =========================================================================
    # Reflection Tools
    # =========================================================================

    @mcp.tool()
    def search_functions(
        keyword: str,
        class_filter: Optional[str] = None,
        max_results: int = 20
    ) -> Dict[str, Any]:
        """Search functions by keyword. Useful for finding overridable functions or API discovery."""
        return get_unreal_client().execute_command("search_functions", {
            "keyword": keyword,
            "class_filter": class_filter,
            "max_results": max_results
        })

    @mcp.tool()
    def get_class_functions(
        class_name: str,
        include_inherited: bool = False,
        callable_only: bool = True
    ) -> Dict[str, Any]:
        """Get all functions of a class. Useful for exploring class capabilities."""
        return get_unreal_client().execute_command("get_class_functions", {
            "class_name": class_name,
            "include_inherited": include_inherited,
            "callable_only": callable_only
        })

    @mcp.tool()
    def get_class_properties(
        class_name: str,
        include_inherited: bool = False,
        blueprint_visible_only: bool = True
    ) -> Dict[str, Any]:
        """Get properties of a UClass. Use with add_property_get_set_node."""
        return get_unreal_client().execute_command("get_class_properties", {
            "class_name": class_name,
            "include_inherited": include_inherited,
            "blueprint_visible_only": blueprint_visible_only
        })

    @mcp.tool()
    def get_blueprint_variables(
        blueprint_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """Get variables in Blueprint's Variables panel. Use with add_blueprint_variable_node."""
        return get_unreal_client().execute_command("get_blueprint_variables", {
            "blueprint_name": blueprint_name,
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # Blueprint Analysis Tools
    # =========================================================================

    @mcp.tool()
    def analyze_blueprint(
        blueprint_name: str,
        blueprint_path: str,
        include_all_graphs: bool = True,
        detailed_pins: bool = True
    ) -> Dict[str, Any]:
        """Analyze Blueprint structure. Returns graphs, nodes, components, variables, overridable_functions."""
        return get_unreal_client().execute_command("analyze_blueprint", {
            "blueprint_name": blueprint_name,
            "include_all_graphs": include_all_graphs,
            "detailed_pins": detailed_pins,
            "blueprint_path": blueprint_path
        })

    @mcp.tool()
    def list_graphs(
        blueprint_name: str,
        blueprint_path: str
    ) -> Dict[str, Any]:
        """List all graphs in a Blueprint (Ubergraph, Function, Macro, Delegate)."""
        return get_unreal_client().execute_command("list_graphs", {
            "blueprint_name": blueprint_name,
            "blueprint_path": blueprint_path
        })

    # =========================================================================
    # Blueprint Inheritance Tools
    # =========================================================================

    @mcp.tool()
    def create_child_blueprint(
        name: str,
        parent_blueprint: str,
        asset_path: str = "/Game/Blueprints/"
    ) -> Dict[str, Any]:
        """Create a child Blueprint inheriting from an existing Blueprint."""
        return get_unreal_client().execute_command("create_child_blueprint", {
            "name": name,
            "parent_blueprint": parent_blueprint,
            "asset_path": asset_path
        })

    # =========================================================================
    # GAS AttributeSet Tools
    # =========================================================================

    @mcp.tool()
    def list_attribute_sets(
        include_engine: bool = False,
        limit: int = 50
    ) -> Dict[str, Any]:
        """List all AttributeSet classes in the project."""
        return get_unreal_client().execute_command("list_attribute_sets", {
            "include_engine": include_engine,
            "limit": int(limit)
        })

    @mcp.tool()
    def get_attribute_set_info(
        attribute_set_name: str
    ) -> Dict[str, Any]:
        """Get attributes of a specific AttributeSet class."""
        return get_unreal_client().execute_command("get_attribute_set_info", {
            "attribute_set_name": attribute_set_name
        })

    # =========================================================================
    # GAS Context Discovery Tools
    # =========================================================================

    @mcp.tool()
    def explore_gas_context(
        keyword: Optional[str] = None,
        reference_ability: Optional[str] = None,
        limit: int = 30
    ) -> Dict[str, Any]:
        """Discover project GAS assets. Call FIRST when creating any GAS ability."""
        client = get_unreal_client()
        result: Dict[str, Any] = {"status": "success"}

        def search_by_class(base_class: str) -> list:
            params = {
                "name": keyword if keyword else "*",
                "search_scope": "all",
                "base_class": base_class,
                "search_path": "/",
                "limit": limit
            }
            resp = client.execute_command("search_assets", params)
            if resp.get("status") == "success":
                return resp.get("result", {}).get("assets", [])
            return []

        result["abilities"] = search_by_class("GameplayAbility")
        result["effects"] = search_by_class("GameplayEffect")
        result["montages"] = search_by_class("AnimMontage")

        tag_resp = client.execute_command("list_gameplay_tags", {
            "prefix": "Ability",
            "limit": limit * 2
        })
        if tag_resp.get("status") == "success":
            result["tags"] = tag_resp.get("result", {}).get("tags", [])

        attr_resp = client.execute_command("list_attribute_sets", {
            "include_engine": False,
            "limit": limit
        })
        if attr_resp.get("status") == "success":
            result["attribute_sets"] = attr_resp.get("result", {}).get("attribute_sets", [])

        if reference_ability:
            ref_resp = client.execute_command("analyze_blueprint", {
                "blueprint_name": reference_ability,
                "include_all_graphs": True,
                "detailed_pins": False,
                "blueprint_path": "/Game/"
            })
            if ref_resp.get("status") == "success":
                result["reference_structure"] = ref_resp.get("result", {})

        result["instruction"] = {
            "type": "planning_required",
            "message": "Context discovery complete. Present the implementation plan to user for approval.",
            "plan_template": [
                "1. Ability Overview: Name, parent class, activation method (Cost/Cooldown/Input)",
                "2. GameplayEffects: List GE assets to create or reuse (damage, buff, debuff)",
                "3. AbilityTasks: Required async tasks (PlayMontage, WaitTargetData, WaitDelay)",
                "4. Variables: Blueprint variables needed (TargetActor, DamageAmount, etc.)",
                "5. Event Flow: ActivateAbility -> Tasks -> ApplyEffects -> EndAbility",
                "6. Connections: Key node connections and execution flow"
            ],
            "next_step": "After user confirms: call build_ability_graph."
        }

        return result

    @mcp.tool()
    def build_ability_graph(
        blueprint_name: str,
        blueprint_path: str,
        nodes: List[AbilityGraphNode],
        connections: List[NodeConnection],
        pin_defaults: Optional[List[PinDefault]] = None,
        graph_name: str = "ActivateAbility",
        auto_layout: bool = True
    ) -> Dict[str, Any]:
        """Preferred tool for GameplayAbility Blueprint creation. Builds entire graph (nodes, connections, pin defaults) atomically in one call with auto-layout."""
        nodes_data = []
        for i, node in enumerate(nodes):
            try:
                if isinstance(node, dict):
                    validated = _NodeAdapter.validate_python(node)
                elif isinstance(node, _NodeBase):
                    validated = node
                else:
                    return create_error_response(f"nodes[{i}] must be a node dict or model")
                nodes_data.append(validated.model_dump(exclude_none=True))
            except Exception as e:
                return create_error_response(f"nodes[{i}]: {str(e)}")

        connections_data = []
        for i, conn in enumerate(connections):
            if isinstance(conn, NodeConnection):
                connections_data.append(conn.model_dump())
            elif isinstance(conn, dict):
                try:
                    validated = NodeConnection(**conn)
                    connections_data.append(validated.model_dump())
                except Exception as e:
                    return create_error_response(f"connections[{i}]: {str(e)}")
            else:
                return create_error_response(f"connections[{i}] must be NodeConnection or dict")

        pin_defaults_data = None
        if pin_defaults:
            pin_defaults_data = []
            for i, pd in enumerate(pin_defaults):
                if isinstance(pd, PinDefault):
                    pin_defaults_data.append(pd.model_dump())
                elif isinstance(pd, dict):
                    try:
                        validated = PinDefault(**pd)
                        pin_defaults_data.append(validated.model_dump())
                    except Exception as e:
                        return create_error_response(f"pin_defaults[{i}]: {str(e)}")
                else:
                    return create_error_response(f"pin_defaults[{i}] must be PinDefault or dict")

        # Build command params with validated data
        params = {
            "blueprint_name": blueprint_name,
            "nodes": nodes_data,
            "connections": connections_data,
            "graph_name": graph_name,
            "blueprint_path": blueprint_path,
            "auto_layout": auto_layout
        }

        if pin_defaults_data:
            params["pin_defaults"] = pin_defaults_data

        return get_unreal_client().execute_command("build_ability_graph", params)

    log_info("Blueprint tools registered successfully")
