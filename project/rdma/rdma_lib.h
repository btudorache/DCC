#ifndef RDMA_LIB_H
#define RDMA_LIB_H

#include <infiniband/verbs.h>
#include <stdbool.h>
#include <stdint.h>

// Constants for RDMA settings
#define MAX_PEERS 64
#define DEFAULT_PORT 1
#define MAX_SGE 1
#define MAX_WR 128
#define CQ_DEPTH 256
#define MAX_INLINE_DATA 256
#define BUFFER_SIZE 4096

// Connection states
typedef enum {
    RDMA_CONN_INIT,
    RDMA_CONN_ROUTE,
    RDMA_CONN_CONNECTED,
    RDMA_CONN_ERROR
} rdma_conn_state;

// Connection information structure
typedef struct {
    uint32_t qp_num;
    uint16_t lid;
    uint32_t psn;
    uint8_t gid[16];
    char ip[16];
    int port;
} rdma_conn_info;

// Per-peer connection context
typedef struct {
    struct ibv_qp *qp;
    rdma_conn_info local_info;
    rdma_conn_info remote_info;
    rdma_conn_state state;
    int sock;
} rdma_peer_conn;

// Main RDMA context
typedef struct {
    struct ibv_context *context;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_port_attr port_attr;
    struct ibv_mr *mr;
    rdma_peer_conn peers[MAX_PEERS];
    void *comm_buf;
    size_t buf_size;
    int num_peers;
    bool is_server;
    char ip[16];
    int port;
    int dev_port;
} rdma_context;

// Public API Functions

// Initialize RDMA context
rdma_context* rdma_init(const char *ip, int port, size_t buf_size, bool is_server);

// Connect to a peer (client side)
int rdma_connect_peer(rdma_context *ctx, const char *peer_ip, int peer_port);

// Accept a peer connection (server side)
int rdma_accept_peer(rdma_context *ctx);

// Send data to a peer
int rdma_send(rdma_context *ctx, int peer_idx, const void *data, size_t len);

// Receive data from a peer
int rdma_recv(rdma_context *ctx, int peer_idx, void *data, size_t max_len);

// Broadcast data to all peers
int rdma_broadcast(rdma_context *ctx, const void *data, size_t len);

// Perform sequential all-to-all communication
int rdma_sequential_alltoall(rdma_context *ctx, const void *send_buf, void *recv_buf, size_t msg_size);

// Disconnect a peer
int rdma_disconnect_peer(rdma_context *ctx, int peer_idx);

// Clean up RDMA context and resources
void rdma_cleanup(rdma_context *ctx);

// Get last error message
const char* rdma_get_error(void);

#endif /* RDMA_LIB_H */