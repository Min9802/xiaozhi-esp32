# MCP Protocol IoT Control Usage Guide

> This document describes how to implement IoT control of ESP32 devices based on the MCP protocol. For detailed protocol flow, please refer to [`mcp-protocol-en.md`](./mcp-protocol-en.md).

## Introduction

MCP (Model Context Protocol) is a new generation recommended protocol for IoT control. Through the standard JSON-RPC 2.0 format, it discovers and calls "tools" (functions) between the backend and devices, enabling flexible device control.

## Typical Usage Process

1. After the device starts, it establishes a connection with the backend through the basic protocol (such as WebSocket/MQTT).
2. The backend initializes the session through the MCP protocol's `initialize` method.
3. The backend obtains all tools (functions) supported by the device and parameter descriptions through `tools/list`.
4. The backend calls specific tools through `tools/call` to implement device control.

For detailed protocol format and interaction, see [`mcp-protocol-en.md`](./mcp-protocol-en.md).

## Device-side Tool Registration Method Guide

Devices register "tools" that can be called by the backend through the `McpServer::AddTool` method. Its commonly used function signature is as follows:

```cpp
void AddTool(
    const std::string& name,           // Tool name, suggested to be unique and hierarchical, e.g., self.dog.forward
    const std::string& description,    // Tool description, briefly explains functionality, convenient for AI to understand
    const PropertyList& properties,    // Input parameter list (can be empty), supported types: boolean, integer, string
    std::function<ReturnValue(const PropertyList&)> callback // Callback implementation when tool is called
);
```
- name: Unique identifier for the tool, suggested naming style is "module.function".
- description: Natural language description, easy for AI/users to understand.
- properties: Parameter list, supported types include boolean, integer, string, can specify range and default values.
- callback: Actual execution logic when a call request is received, return value can be bool/int/string.

## Typical Registration Examples (Using ESP-Hi as Example)

```cpp
void InitializeTools() {
    auto& mcp_server = McpServer::GetInstance();
    // Example 1: No parameters, control robot forward movement
    mcp_server.AddTool("self.dog.forward", "Robot moves forward", PropertyList(), [this](const PropertyList&) -> ReturnValue {
        servo_dog_ctrl_send(DOG_STATE_FORWARD, NULL);
        return true;
    });
    // Example 2: With parameters, set LED RGB color
    mcp_server.AddTool("self.light.set_rgb", "Set RGB color", PropertyList({
        Property("r", kPropertyTypeInteger, 0, 255),
        Property("g", kPropertyTypeInteger, 0, 255),
        Property("b", kPropertyTypeInteger, 0, 255)
    }), [this](const PropertyList& properties) -> ReturnValue {
        int r = properties["r"].value<int>();
        int g = properties["g"].value<int>();
        int b = properties["b"].value<int>();
        led_on_ = true;
        SetLedColor(r, g, b);
        return true;
    });
}
```

## Common Tool Call JSON-RPC Examples

### 1. Get Tool List
```json
{
  "jsonrpc": "2.0",
  "method": "tools/list",
  "params": { "cursor": "" },
  "id": 1
}
```

### 2. Control Chassis Move Forward
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "self.chassis.go_forward",
    "arguments": {}
  },
  "id": 2
}
```

### 3. Switch Light Mode
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "self.chassis.switch_light_mode",
    "arguments": { "light_mode": 3 }
  },
  "id": 3
}
```

### 4. Camera Flip
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "self.camera.set_camera_flipped",
    "arguments": {}
  },
  "id": 4
}
```

## Notes
- Tool names, parameters, and return values should be based on the device-side `AddTool` registration.
- It is recommended that all new projects uniformly adopt the MCP protocol for IoT control.
- For detailed protocol and advanced usage, please refer to [`mcp-protocol-en.md`](./mcp-protocol-en.md).
