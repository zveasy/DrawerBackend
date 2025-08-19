# Architecture Overview

This document outlines how the Register MVP components interact: the hardware abstraction layer (HAL), device drivers, application logic, HTTP server, and deterministic test harness.

## Component Relationships

```mermaid
flowchart TD
    HAL[Hardware Abstraction Layer] --> Drivers
    Drivers --> App[Application Logic]
    App --> Server[HTTP Server]
    Test[Test Harness] --> HAL
    Client --> Server
```

- **HAL** wraps low level GPIO access and provides a consistent interface for real hardware and simulations.
- **Drivers** implement devices such as the shutter, hopper, and scale on top of the HAL.
- **Application logic** coordinates drivers to perform transactions and other high level tasks.
- **Server** exposes the application logic over an HTTP API.
- **Test harness** exercises the application through the HAL with deterministic inputs to validate behavior.

## Transaction Processing

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    participant A as App
    participant D as Drivers
    participant H as HAL
    participant HW as Hardware

    C->>S: HTTP request
    S->>A: Route and validate
    A->>D: Command execution
    D->>H: GPIO operations
    H->>HW: Signals
    HW-->>H: Sensor data
    H-->>D: Driver result
    D-->>A: Outcome
    A-->>S: Response payload
    S-->>C: HTTP 200
```

The sequence above shows a typical transaction. Client requests are handled by the server, routed through application logic to the drivers and HAL, and results propagate back to the client.
