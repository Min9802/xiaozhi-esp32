# MQTT + UDP Hybrid Communication Protocol Documentation

Documentation of the MQTT + UDP hybrid communication protocol organized based on code implementation, outlining how the device interacts with the server: transmitting control messages through MQTT and transmitting audio data through UDP.

---

## 1. Protocol Overview

This protocol adopts a hybrid transport method:
- **MQTT**: Used for control messages, status synchronization, and JSON data exchange
- **UDP**: Used for real-time audio data transmission, supports encryption

### 1.1 Protocol Features

- **Dual-channel Design**: Control and data are separated to ensure real-time performance
- **Encrypted Transmission**: UDP audio data uses AES-CTR encryption
- **Sequence Number Protection**: Prevents data packet replay and out-of-order
- **Automatic Reconnection**: Automatically reconnects when MQTT connection is dropped

---

## 2. Overall Process Overview

```mermaid
sequenceDiagram
    participant Device as ESP32 Device
    participant MQTT as MQTT Server
    participant UDP as UDP Server

    Note over Device, UDP: 1. Establish MQTT connection
    Device->>MQTT: MQTT Connect
    MQTT->>Device: Connected

    Note over Device, UDP: 2. Request audio channel
    Device->>MQTT: Hello Message (type: "hello", transport: "udp")
    MQTT->>Device: Hello Response (UDP connection info + encryption key)

    Note over Device, UDP: 3. Establish UDP connection
    Device->>UDP: UDP Connect
    UDP->>Device: Connected

    Note over Device, UDP: 4. Audio data transmission
    loop Audio stream transmission
        Device->>UDP: Encrypted audio data (Opus)
        UDP->>Device: Encrypted audio data (Opus)
    end

    Note over Device, UDP: 5. Control message exchange
    par Control messages
        Device->>MQTT: Listen/TTS/MCP messages
        MQTT->>Device: STT/TTS/MCP responses
    end

    Note over Device, UDP: 6. Close connection
    Device->>MQTT: Goodbye Message
    Device->>UDP: Disconnect
```

---

## 3. MQTT Control Channel

### 3.1 Connection Establishment

The device connects to the server through MQTT with the following connection parameters:
- **Endpoint**: MQTT server address and port
- **Client ID**: Unique device identifier
- **Username/Password**: Authentication credentials
- **Keep Alive**: Heartbeat interval (default 240 seconds)

### 3.2 Hello Message Exchange

#### 3.2.1 Device Sends Hello

```json
{
  "type": "hello",
  "version": 3,
  "transport": "udp",
  "features": {
    "mcp": true
  },
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

#### 3.2.2 Server Responds Hello

```json
{
  "type": "hello",
  "transport": "udp",
  "session_id": "xxx",
  "audio_params": {
    "format": "opus",
    "sample_rate": 24000,
    "channels": 1,
    "frame_duration": 60
  },
  "udp": {
    "server": "192.168.1.100",
    "port": 8888,
    "key": "0123456789ABCDEF0123456789ABCDEF",
    "nonce": "0123456789ABCDEF0123456789ABCDEF"
  }
}
```

**Field Explanation:**
- `udp.server`: UDP server address
- `udp.port`: UDP server port
- `udp.key`: AES encryption key (hexadecimal string)
- `udp.nonce`: AES encryption nonce (hexadecimal string)

### 3.3 JSON Message Types

#### 3.3.1 Device → Server

1. **Listen Message**
   ```json
   {
     "session_id": "xxx",
     "type": "listen",
     "state": "start",
     "mode": "manual"
   }
   ```

2. **Abort Message**
   ```json
   {
     "session_id": "xxx",
     "type": "abort",
     "reason": "wake_word_detected"
   }
   ```

3. **MCP Message**
   ```json
   {
     "session_id": "xxx",
     "type": "mcp",
     "payload": {
       "jsonrpc": "2.0",
       "id": 1,
       "result": {...}
     }
   }
   ```

4. **Goodbye Message**
   ```json
   {
     "session_id": "xxx",
     "type": "goodbye"
   }
   ```

#### 3.3.2 Server → Device

Supported message types are consistent with the WebSocket protocol, including:
- **STT**: Speech recognition results
- **TTS**: Text-to-speech control
- **LLM**: Emotional expression control
- **MCP**: IoT control
- **System**: System control
- **Custom**: Custom messages (optional)

---

## 4. UDP Audio Channel

### 4.1 Connection Establishment

After the device receives the MQTT Hello response, it uses the UDP connection information to establish an audio channel:
1. Parse UDP server address and port
2. Parse encryption key and nonce
3. Initialize AES-CTR encryption context
4. Establish UDP connection

### 4.2 Audio Data Format

#### 4.2.1 Encrypted Audio Packet Structure

```
|type 1byte|flags 1byte|payload_len 2bytes|ssrc 4bytes|timestamp 4bytes|sequence 4bytes|
|payload payload_len bytes|
```

**Field Explanation:**
- `type`: Packet type, fixed as 0x01
- `flags`: Flags, currently unused
- `payload_len`: Payload length (network byte order)
- `ssrc`: Synchronization source identifier
- `timestamp`: Timestamp (network byte order)
- `sequence`: Sequence number (network byte order)
- `payload`: Encrypted Opus audio data

#### 4.2.2 Encryption Algorithm

Uses **AES-CTR** mode encryption:
- **Key**: 128-bit, provided by server
- **Nonce**: 128-bit, provided by server
- **Counter**: Contains information from timestamp and sequence number

### 4.3 Sequence Number Management

- **Sender**: `local_sequence_` increments monotonically
- **Receiver**: `remote_sequence_` verifies continuity
- **Anti-replay**: Rejects data packets with sequence numbers less than expected value
- **Error Tolerance**: Allows minor sequence number jumps, logs warnings

### 4.4 Error Handling

1. **Decryption Failure**: Logs error, discards data packet
2. **Sequence Number Anomaly**: Logs warning, but still processes data packet
3. **Packet Format Error**: Logs error, discards data packet

---

## 5. State Management

### 5.1 Connection Status

```mermaid
stateDiagram
    direction TB
    [*] --> Disconnected
    Disconnected --> MqttConnecting: StartMqttClient()
    MqttConnecting --> MqttConnected: MQTT Connected
    MqttConnecting --> Disconnected: Connect Failed
    MqttConnected --> RequestingChannel: OpenAudioChannel()
    RequestingChannel --> ChannelOpened: Hello Exchange Success
    RequestingChannel --> MqttConnected: Hello Timeout/Failed
    ChannelOpened --> UdpConnected: UDP Connect Success
    UdpConnected --> AudioStreaming: Start Audio Transfer
    AudioStreaming --> UdpConnected: Stop Audio Transfer
    UdpConnected --> ChannelOpened: UDP Disconnect
    ChannelOpened --> MqttConnected: CloseAudioChannel()
    MqttConnected --> Disconnected: MQTT Disconnect
```

### 5.2 Status Check

Device determines whether the audio channel is available through the following conditions:
```cpp
bool IsAudioChannelOpened() const {
    return udp_ != nullptr && !error_occurred_ && !IsTimeout();
}
```

---

## 6. Configuration Parameters

### 6.1 MQTT Configuration

Configuration items read from settings:
- `endpoint`: MQTT server address
- `client_id`: Client identifier
- `username`: Username
- `password`: Password
- `keepalive`: Heartbeat interval (default 240 seconds)
- `publish_topic`: Publish topic

### 6.2 Audio Parameters

- **Format**: Opus
- **Sample Rate**: 16000 Hz (device) / 24000 Hz (server)
- **Channels**: 1 (mono)
- **Frame Duration**: 60ms

---

## 7. Error Handling and Reconnection

### 7.1 MQTT Reconnection Mechanism

- Automatically retries on connection failure
- Supports error reporting control
- Triggers cleanup process on disconnection

### 7.2 UDP Connection Management

- Does not automatically retry on connection failure
- Depends on MQTT channel for renegotiation
- Supports connection status queries

### 7.3 Timeout Handling

Base class `Protocol` provides timeout detection:
- Default timeout: 120 seconds
- Calculated based on last received time
- Automatically marked as unavailable on timeout

---

## 8. Security Considerations

### 8.1 Transport Encryption

- **MQTT**: Supports TLS/SSL encryption (port 8883)
- **UDP**: Uses AES-CTR encryption for audio data

### 8.2 Authentication Mechanism

- **MQTT**: Username/password authentication
- **UDP**: Key distribution through MQTT channel

### 8.3 Anti-replay Protection

- Sequence number increments monotonically
- Rejects expired data packets
- Timestamp verification

---

## 9. Performance Optimization

### 9.1 Concurrency Control

Uses mutex to protect UDP connection:
```cpp
std::lock_guard<std::mutex> lock(channel_mutex_);
```

### 9.2 Memory Management

- Dynamically create/destroy network objects
- Smart pointer management for audio data packets
- Timely release of encryption context

### 9.3 Network Optimization

- UDP connection reuse
- Audio packet size optimization
- Sequence number continuity check

---

## 10. Comparison with WebSocket Protocol

| Feature | MQTT + UDP | WebSocket |
|---------|------------|-----------|
| Control Channel | MQTT | WebSocket |
| Audio Channel | UDP (encrypted) | WebSocket (binary) |
| Real-time Performance | High (UDP) | Medium |
| Reliability | Medium | High |
| Complexity | High | Low |
| Encryption | AES-CTR | TLS |
| Firewall Friendliness | Low | High |

---

## 11. Deployment Recommendations

### 11.1 Network Environment

- Ensure UDP port is reachable
- Configure firewall rules
- Consider NAT traversal

### 11.2 Server Configuration

- MQTT Broker configuration
- UDP server deployment
- Key management system

### 11.3 Monitoring Metrics

- Connection success rate
- Audio transmission latency
- Packet loss rate
- Decryption failure rate

---

## 12. Summary

The MQTT + UDP hybrid protocol achieves efficient audio/video communication through the following design:

- **Separated Architecture**: Control and data channels are separated, each performing its function
- **Encryption Protection**: AES-CTR ensures secure audio data transmission
- **Serialization Management**: Prevents replay attacks and data out-of-order
- **Automatic Recovery**: Supports automatic reconnection after connection drops
- **Performance Optimization**: UDP transmission ensures real-time audio data

This protocol is suitable for voice interaction scenarios with high real-time requirements, but requires trade-offs between network complexity and transmission performance.
