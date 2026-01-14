# Unreal Engine MCP

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.6%2B-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.11-yellow)](https://www.python.org)
[![Status](https://img.shields.io/badge/Status-Experimental-red)](https://github.com/gimmeDG/UnrealEngine5-mcp)

MCP tool that enables natural language control of Unreal Engine 5.6+.
Designed for large-scale projects with specialized Blueprint workflow support.
Build node graphs, create GameplayAbilities, and automate editor tasks through MCP client.
Includes RAG-powered Python scripting with Unreal Python API documentation.

## 🎬 Demo

![Homing Cluster Grenade](https://github.com/user-attachments/assets/93849722-ffad-4bab-b54c-ade8af825d65)

*Homing Cluster Grenade - Lyra grenade + MCP-generated GAS Blueprints (GA, GE, skill system)*

## ⭐ Features

| Category | Description |
|----------|-------------|
| **Blueprint Tools** | Create blueprints, add components, build node graphs, manage variables, connect pins |
| **GAS (Gameplay Ability System)** | Create GameplayAbilities, GameplayEffects, AbilityTasks with full graph construction |
| **Editor Tools** | Spawn actors, create materials, manipulate transforms, search assets |
| **RAG Search** | Search Unreal Python API docs with BM25 |
| **Python Execution** | Execute Python code in Unreal with RAG-powered documentation lookup |
| **PCG Tools** | Create PCG graphs, add nodes, connect edges *(Early stage - in development)* |

## 📐 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│              Claude Desktop / Cursor (Client)                │
│            LLM reasoning & tool invocation                   │
└──────────────────────────┬──────────────────────────────────┘
                           │ MCP Protocol (stdio)
┌──────────────────────────┴──────────────────────────────────┐
│                    Python MCP Server                         │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Tools                                                  │ │
│  │  • editor_tool    - Actor/asset manipulation           │ │
│  │  • blueprint_tool - Blueprint/GAS node graphs          │ │
│  │  • pcg_tool       - PCG graph construction             │ │
│  │  • rag_tool       - API search & Python execution      │ │
│  └────────────────────────────────────────────────────────┘ │
└──────────────────────────┬──────────────────────────────────┘
                           │ TCP
┌──────────────────────────┴──────────────────────────────────┐
│                 Unreal Engine C++ Plugin                     │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Command Handlers                                       │ │
│  │  • EditorCommands    - Level actor operations          │ │
│  │  • BlueprintCommands - BP asset & node graph editing   │ │
│  │  • PCGCommands       - PCG graph manipulation          │ │
│  │  • PythonExecutor    - Python script execution         │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘

RAG System (BM25):
  • Provides Unreal Python API documentation search
  • Used by execute_unreal_python on failure for error recovery
  • No external API keys required
```

## 📖 Start Guide

### Requirements

| Component | Version |
|-----------|---------|
| **Unreal Engine** | 5.6+ |
| **Python** | 3.11 |
| **Claude Desktop** or **Cursor** | Latest |

> **Note:** `uv` package manager is recommended but will be installed automatically if not present.

### 1. Clone Repository

```bash
git clone https://github.com/gimmeDG/UnrealEngine5-mcp.git
cd UnrealEngine5-mcp
```

### 2. Setup Unreal Engine Plugin

1. Copy `Plugins/UnrealEngineMCP` folder to your Unreal project's `Plugins/` directory
2. Open your project in Unreal Editor
3. Enable the plugin: **Edit → Plugins → Search "UnrealEngineMCP" → Enable**
4. Restart Unreal Editor
### 3. Configure MCP Client

#### Claude Desktop

**Windows** - Edit `%APPDATA%\Claude\claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "UnrealEngineMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "C:\\path\\to\\unreal-engine-mcp",
        "run",
        "main.py"
      ]
    }
  }
}
```

**macOS** - Edit `~/Library/Application Support/Claude/claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "UnrealEngineMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "/path/to/unreal-engine-mcp",
        "run",
        "main.py"
      ]
    }
  }
}
```

#### Cursor

**Settings → MCP → Add Server** with the same configuration:

```json
{
  "UnrealEngineMCP": {
    "command": "uv",
    "args": [
      "--directory",
      "/path/to/unreal-engine-mcp",
      "run",
      "main.py"
    ]
  }
}
```

### 4. (Optional) Environment Setup

```bash
cp .env.example .env
# Edit .env if you need to change default settings
```

Default configuration works out of the box. The server will:
- Automatically create virtual environment
- Install all dependencies
- Generate BM25 index on first run

## 🛠️ Available Tools

### Editor Tools (21 tools)

| Category | Tools |
|----------|-------|
| **Actor** | `spawn_actor`, `spawn_blueprint_actor`, `delete_actor`, `list_level_actors`, `set_actor_transform`, `get_actor_properties`, `set_actor_property` |
| **Material** | `create_material`, `apply_material_to_actor`, `get_actor_material_info` |
| **Search** | `search_actors`, `search_assets`, `list_folder_assets`, `list_gameplay_tags` |
| **World Partition** | `get_world_partition_info`, `search_actors_in_region`, `load_actor_by_guid`, `set_region_loaded`, `list_level_instances`, `get_level_instance_actors` |
| **Utility** | `get_connection_status` |

### Blueprint Tools (47 tools)

| Category | Tools |
|----------|-------|
| **Core** | `create_blueprint`, `compile_blueprint`, `analyze_blueprint`, `create_child_blueprint`, `list_graphs` |
| **Component** | `add_component_to_blueprint`, `set_component_property`, `set_physics_properties`, `delete_component_from_blueprint`, `apply_material_to_blueprint`, `get_blueprint_material_info`, `set_mesh_material_color` |
| **Variable** | `add_blueprint_variable`, `add_blueprint_variable_node`, `get_blueprint_variables`, `delete_blueprint_variable` |
| **Node Graph** | `add_blueprint_event_node`, `add_custom_event_node`, `add_blueprint_function_node`, `add_blueprint_flow_control_node`, `add_function_override`, `add_blueprint_input_action_node`, `add_component_getter_node`, `add_property_get_set_node`, `add_blueprint_self_reference`, `add_blueprint_generic_node`, `list_blueprint_nodes`, `connect_blueprint_nodes`, `connect_nodes`, `set_pin_default_value`, `get_pin_value`, `set_node_property`, `disconnect_blueprint_nodes`, `delete_blueprint_node`, `add_pin`, `delete_pin`, `add_comment_box` |
| **GAS** | `create_gameplay_effect`, `create_gameplay_ability`, `add_ability_task_node`, `explore_gas_context`, `list_attribute_sets`, `get_attribute_set_info`, `build_ability_graph` |
| **Reflection** | `search_functions`, `get_class_functions`, `get_class_properties` |

### RAG Tools (2 tools)

| Tool | Description |
|------|-------------|
| `search_unreal_api` | Search Unreal Python API documentation using BM25 |
| `execute_unreal_python` | Execute Python code in Unreal Engine with safety validation |

### PCG Tools (13 tools) - *Early Stage*

> **In Development**: PCG tools are functional but still being refined.

| Category | Tools |
|----------|-------|
| **Graph** | `create_pcg_graph`, `analyze_pcg_graph`, `set_pcg_graph_to_component` |
| **Node** | `add_pcg_sampler_node`, `add_pcg_filter_node`, `add_pcg_transform_node`, `add_pcg_spawner_node`, `add_pcg_attribute_node`, `add_pcg_flow_control_node`, `add_pcg_generic_node`, `list_pcg_nodes`, `connect_pcg_nodes`, `disconnect_pcg_nodes`, `delete_pcg_node` |

## 📄 License

MIT License

## 💬 Feedback

Found a bug or have a suggestion? [Open an issue](https://github.com/gimmeDG/UnrealEngine5-mcp/issues).
