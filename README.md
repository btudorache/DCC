# RDMA Client-Server Library

## Library Location

The RDMA library is located at `/project/rdma`. This directory contains all the necessary source files and headers for building and using the library (rdma_lib.c and rdma_lib.h).

## Introduction

This library provides a high-level abstraction for RDMA (Remote Direct Memory Access) communication in distributed systems. It implements a client-server architecture that allows for efficient, low-latency communication between nodes using InfiniBand/RoCE hardware. The library handles the complexity of RDMA setup, connection management, and data transfer operations while providing a simple and intuitive API.

Key features:
- Easy-to-use API for RDMA communication
- Support for multiple concurrent peer connections
- Built-in connection management and error handling
- Efficient memory management with pre-registered buffers
- Implementation of common communication patterns (send/receive, broadcast, all-to-all)

## Architecture

The library is built around several key components:

1. **RDMA Context**: The main structure that holds all RDMA resources and connection state
2. **Peer Connections**: Management of individual QP (Queue Pair) connections between nodes
3. **Memory Management**: Pre-registered memory buffers for efficient RDMA operations
4. **Communication Patterns**: Implementation of common distributed communication patterns

## API Reference

### Initialization and Cleanup

```c
// Initialize RDMA context
rdma_context* rdma_init(const char *ip, int port, size_t buf_size, bool is_server);

// Clean up RDMA context and resources
void rdma_cleanup(rdma_context *ctx);
```

### Connection Management

```c
// Connect to a peer (client side)
int rdma_connect_peer(rdma_context *ctx, const char *peer_ip, int peer_port);

// Accept a peer connection (server side)
int rdma_accept_peer(rdma_context *ctx);

// Disconnect a peer
int rdma_disconnect_peer(rdma_context *ctx, int peer_idx);
```

### Communication Operations

```c
// Send data to a peer
int rdma_send(rdma_context *ctx, int peer_idx, const void *data, size_t len);

// Receive data from a peer
int rdma_recv(rdma_context *ctx, int peer_idx, void *data, size_t max_len);

// Broadcast data to all peers
int rdma_broadcast(rdma_context *ctx, const void *data, size_t len);

// Perform sequential all-to-all communication
int rdma_sequential_alltoall(rdma_context *ctx, const void *send_buf, 
                           void *recv_buf, size_t msg_size);
```

### Error Handling

```c
// Get last error message
const char* rdma_get_error(void);
```

## Usage Examples

### Server Example

Here's a complete example of a server that accepts two clients and performs all-to-all communication:

```c
#include "rdma_lib.h"
#include <stdio.h>
#include <string.h>

#define PORT 5555
#define MSG_SIZE 64
#define NUM_CLIENTS 2

int main(void) {
    const char *server_ip = "192.168.50.59";
    printf("Server starting on IP %s...\n", server_ip);

    // Initialize server context
    rdma_context *ctx = rdma_init(server_ip, PORT, BUFFER_SIZE * 4, true);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize RDMA: %s\n", rdma_get_error());
        return 1;
    }

    // Accept client connections
    for (int i = 0; i < NUM_CLIENTS; i++) {
        printf("Waiting for client %d...\n", i + 1);
        if (rdma_accept_peer(ctx) < 0) {
            fprintf(stderr, "Failed to accept client %d: %s\n", 
                    i + 1, rdma_get_error());
            rdma_cleanup(ctx);
            return 1;
        }
        printf("Client %d connected\n", i + 1);
    }

    // Send start signal to all clients
    const char *start_msg = "START";
    for (int i = 0; i < NUM_CLIENTS; i++) {
        if (rdma_send(ctx, i, start_msg, strlen(start_msg) + 1) < 0) {
            fprintf(stderr, "Failed to send start message\n");
            rdma_cleanup(ctx);
            return 1;
        }
    }

    // Perform all-to-all communication
    char send_msg[MSG_SIZE];
    char recv_buf[BUFFER_SIZE];

    snprintf(send_msg, sizeof(send_msg), "Server Message");
    printf("Sending: '%s'\n", send_msg);

    if (rdma_sequential_alltoall(ctx, send_msg, recv_buf, MSG_SIZE) < 0) {
        fprintf(stderr, "All-to-all failed: %s\n", rdma_get_error());
        rdma_cleanup(ctx);
        return 1;
    }

    printf("Received combined messages: '%s'\n", recv_buf);

    // Cleanup
    rdma_cleanup(ctx);
    return 0;
}
```

### Client Example

Here's a complete example of a client that connects to the server and participates in all-to-all communication:

```c
#include "rdma_lib.h"
#include <stdio.h>
#include <string.h>

#define PORT 5555
#define MSG_SIZE 64

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <client_id> <client_ip>\n", argv[0]);
        return 1;
    }

    int client_id = atoi(argv[1]);
    const char *client_ip = argv[2];
    const char *server_ip = "192.168.50.59";

    printf("Client %d starting on IP %s...\n", client_id, client_ip);

    // Initialize client context
    rdma_context *ctx = rdma_init(client_ip, PORT, BUFFER_SIZE * 4, false);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize RDMA: %s\n", rdma_get_error());
        return 1;
    }

    // Connect to server
    printf("Connecting to server %s...\n", server_ip);
    int server_idx = rdma_connect_peer(ctx, server_ip, PORT);
    if (server_idx < 0) {
        fprintf(stderr, "Failed to connect to server: %s\n", rdma_get_error());
        rdma_cleanup(ctx);
        return 1;
    }

    // Wait for start signal
    char start_msg[MSG_SIZE];
    if (rdma_recv(ctx, server_idx, start_msg, MSG_SIZE) < 0) {
        fprintf(stderr, "Failed to receive start signal\n");
        rdma_cleanup(ctx);
        return 1;
    }
    printf("Received start signal: %s\n", start_msg);

    // Participate in all-to-all communication
    char send_msg[MSG_SIZE];
    char recv_buf[BUFFER_SIZE];

    snprintf(send_msg, sizeof(send_msg), "Client %d Message", client_id);
    printf("Sending: '%s'\n", send_msg);

    if (rdma_sequential_alltoall(ctx, send_msg, recv_buf, MSG_SIZE) < 0) {
        fprintf(stderr, "All-to-all failed: %s\n", rdma_get_error());
        rdma_cleanup(ctx);
        return 1;
    }

    printf("Received combined messages: '%s'\n", recv_buf);

    // Cleanup
    rdma_cleanup(ctx);
    return 0;
}
```

## Configuration

The library uses several predefined constants that can be adjusted in `rdma_lib.h`:

```c
#define MAX_PEERS 64        // Maximum number of concurrent peer connections
#define MAX_WR 128         // Maximum number of outstanding work requests
#define CQ_DEPTH 256      // Completion queue depth
#define BUFFER_SIZE 4096  // Size of pre-registered communication buffer
```

## Future Enhancements

The library is designed to be extensible and will include additional features in future releases:
- RDMA Write operations
- RDMA Read operations
- Asynchronous communication interfaces
- Support for more collective operations
- Quality of Service (QoS) configurations
- Enhanced error handling and recovery mechanisms

## Implementation Notes

- The library uses RC (Reliable Connection) QPs for all communications
- Memory buffers are pre-registered with the RDMA device for optimal performance
- All operations are currently synchronous, waiting for completion
- The implementation supports both InfiniBand and RoCE (RDMA over Converged Ethernet)
- Error handling includes detailed error messages for debugging
