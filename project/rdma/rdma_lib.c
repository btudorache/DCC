#include "rdma_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

// Global error buffer
static char error_buf[1024];

// Internal helper functions
static void set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(error_buf, sizeof(error_buf), fmt, args);
    va_end(args);
}

// Get the last error message
const char* rdma_get_error(void) {
    return error_buf;
}

// Create Queue Pair (QP)
static struct ibv_qp* create_qp(rdma_context *ctx) {
    struct ibv_qp_init_attr qp_attr = {
        .send_cq = ctx->cq,
        .recv_cq = ctx->cq,
        .cap = {
            .max_send_wr = MAX_WR,
            .max_recv_wr = MAX_WR,
            .max_send_sge = MAX_SGE,
            .max_recv_sge = MAX_SGE,
            .max_inline_data = MAX_INLINE_DATA
        },
        .qp_type = IBV_QPT_RC,
        .sq_sig_all = 1  // Generate completion for all send operations
    };

    struct ibv_qp *qp = ibv_create_qp(ctx->pd, &qp_attr);
    if (!qp) {
        set_error("Failed to create QP: %s", strerror(errno));
        return NULL;
    }
    return qp;
}

// Transition QP to INIT state
static int modify_qp_to_init(struct ibv_qp *qp, int port) {
    struct ibv_qp_attr attr = {
        .qp_state = IBV_QPS_INIT,
        .pkey_index = 0,
        .port_num = port,
        .qp_access_flags = IBV_ACCESS_LOCAL_WRITE |
                          IBV_ACCESS_REMOTE_READ |
                          IBV_ACCESS_REMOTE_WRITE
    };

    int flags = IBV_QP_STATE |
                IBV_QP_PKEY_INDEX |
                IBV_QP_PORT |
                IBV_QP_ACCESS_FLAGS;

    if (ibv_modify_qp(qp, &attr, flags)) {
        set_error("Failed to modify QP to INIT: %s", strerror(errno));
        return -1;
    }
    return 0;
}


// Transition QP to RTR (Ready to Receive) state
static int modify_qp_to_rtr(struct ibv_qp *qp, rdma_conn_info *remote_info, int port) {
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    
    // Basic QP attributes
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_1024;
    attr.dest_qp_num = remote_info->qp_num;
    attr.rq_psn = remote_info->psn;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 12;
    
    // Address handle attributes
    attr.ah_attr.is_global = 1;
    attr.ah_attr.dlid = 0;  // Use 0 for RoCE
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = port;
    
    // Global routing attributes
    attr.ah_attr.grh.flow_label = 0;
    attr.ah_attr.grh.sgid_index = 0;
    attr.ah_attr.grh.hop_limit = 1;
    attr.ah_attr.grh.traffic_class = 0;
    
    // Copy remote GID
    memcpy(&attr.ah_attr.grh.dgid, remote_info->gid, sizeof(union ibv_gid));

    int flags = IBV_QP_STATE |
                IBV_QP_AV |
                IBV_QP_PATH_MTU |
                IBV_QP_DEST_QPN |
                IBV_QP_RQ_PSN |
                IBV_QP_MAX_DEST_RD_ATOMIC |
                IBV_QP_MIN_RNR_TIMER;

    if (ibv_modify_qp(qp, &attr, flags)) {
        set_error("Failed to modify QP to RTR: %s", strerror(errno));
        return -1;
    }
    return 0;
}

// Transition QP to RTS state
static int modify_qp_to_rts(struct ibv_qp *qp, uint32_t psn) {
    struct ibv_qp_attr attr = {
        .qp_state = IBV_QPS_RTS,
        .timeout = 14,
        .retry_cnt = 7,
        .rnr_retry = 7,
        .sq_psn = psn,
        .max_rd_atomic = 1
    };

    int flags = IBV_QP_STATE |
                IBV_QP_TIMEOUT |
                IBV_QP_RETRY_CNT |
                IBV_QP_RNR_RETRY |
                IBV_QP_SQ_PSN |
                IBV_QP_MAX_QP_RD_ATOMIC;

    if (ibv_modify_qp(qp, &attr, flags)) {
        set_error("Failed to modify QP to RTS: %s", strerror(errno));
        return -1;
    }
    return 0;
}

// Post a receive work request
static int post_recv(rdma_context *ctx, int peer_idx, void *buf, size_t len) {
    struct ibv_sge sge = {
        .addr = (uint64_t)buf,
        .length = len,
        .lkey = ctx->mr->lkey
    };

    struct ibv_recv_wr wr = {
        .wr_id = peer_idx,
        .sg_list = &sge,
        .num_sge = 1
    };

    struct ibv_recv_wr *bad_wr;
    if (ibv_post_recv(ctx->peers[peer_idx].qp, &wr, &bad_wr)) {
        set_error("Failed to post receive: %s", strerror(errno));
        return -1;
    }
    return 0;
}

// Wait for work completion
static int wait_for_completion(rdma_context *ctx) {
    struct ibv_wc wc;
    int num_comp;

    do {
        num_comp = ibv_poll_cq(ctx->cq, 1, &wc);
    } while (num_comp == 0);

    if (num_comp < 0) {
        set_error("Failed to poll CQ");
        return -1;
    }

    if (wc.status != IBV_WC_SUCCESS) {
        set_error("Work completion failed with status: %d", wc.status);
        return -1;
    }

    return wc.wr_id;
}

// Initialize RDMA context
rdma_context* rdma_init(const char *ip, int port, size_t buf_size, bool is_server) {
    rdma_context *ctx = calloc(1, sizeof(rdma_context));
    if (!ctx) {
        set_error("Failed to allocate context");
        return NULL;
    }

    // Store basic information
    strncpy(ctx->ip, ip, sizeof(ctx->ip) - 1);
    ctx->ip[sizeof(ctx->ip) - 1] = '\0';
    ctx->port = port;
    ctx->is_server = is_server;
    ctx->dev_port = DEFAULT_PORT;
    ctx->buf_size = buf_size;

    // Get IB device list
    int num_devices;
    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
        set_error("Failed to get IB devices list");
        free(ctx);
        return NULL;
    }

    // Find first available device
    struct ibv_device *ib_dev = NULL;
    for (int i = 0; i < num_devices; i++) {
        if (dev_list[i]) {
            ib_dev = dev_list[i];
            break;
        }
    }

    if (!ib_dev) {
        set_error("No IB devices found");
        ibv_free_device_list(dev_list);
        free(ctx);
        return NULL;
    }

    // Open device
    ctx->context = ibv_open_device(ib_dev);
    if (!ctx->context) {
        set_error("Failed to open device: %s", strerror(errno));
        ibv_free_device_list(dev_list);
        free(ctx);
        return NULL;
    }

    // Query port attributes
    if (ibv_query_port(ctx->context, ctx->dev_port, &ctx->port_attr)) {
        set_error("Failed to query port: %s", strerror(errno));
        goto cleanup_context;
    }

    // Query GID
    union ibv_gid gid;
    if (ibv_query_gid(ctx->context, ctx->dev_port, 0, &gid)) {
        set_error("Failed to query GID: %s", strerror(errno));
        goto cleanup_context;
    }

    printf("Device: %s\n", ibv_get_device_name(ib_dev));
    printf("Port: %d\n", ctx->dev_port);
    printf("Port LID: %d\n", ctx->port_attr.lid);
    printf("GID[0]: %.16lx:%.16lx\n", 
           be64toh(gid.global.subnet_prefix), 
           be64toh(gid.global.interface_id));

    // Rest of initialization...
    ctx->pd = ibv_alloc_pd(ctx->context);
    if (!ctx->pd) {
        set_error("Failed to allocate PD");
        goto cleanup_context;
    }

    ctx->cq = ibv_create_cq(ctx->context, CQ_DEPTH, NULL, NULL, 0);
    if (!ctx->cq) {
        set_error("Failed to create CQ");
        goto cleanup_pd;
    }

    ctx->comm_buf = aligned_alloc(4096, buf_size);
    if (!ctx->comm_buf) {
        set_error("Failed to allocate communication buffer");
        goto cleanup_cq;
    }
    memset(ctx->comm_buf, 0, buf_size);

    ctx->mr = ibv_reg_mr(ctx->pd, ctx->comm_buf, buf_size,
                         IBV_ACCESS_LOCAL_WRITE |
                         IBV_ACCESS_REMOTE_WRITE |
                         IBV_ACCESS_REMOTE_READ);
    if (!ctx->mr) {
        set_error("Failed to register MR");
        goto cleanup_buffer;
    }

    ibv_free_device_list(dev_list);
    return ctx;

cleanup_buffer:
    free(ctx->comm_buf);
cleanup_cq:
    ibv_destroy_cq(ctx->cq);
cleanup_pd:
    ibv_dealloc_pd(ctx->pd);
cleanup_context:
    ibv_close_device(ctx->context);
    ibv_free_device_list(dev_list);
    free(ctx);
    return NULL;
}

// Client connection to peer
int rdma_connect_peer(rdma_context *ctx, const char *peer_ip, int peer_port) {
    if (ctx->num_peers >= MAX_PEERS) {
        set_error("Maximum number of peers reached");
        return -1;
    }

    rdma_peer_conn *peer = &ctx->peers[ctx->num_peers];

    // Create socket and connect to server
    peer->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (peer->sock < 0) {
        set_error("Failed to create socket");
        return -1;
    }

    // Set socket options
    int option = 1;
    setsockopt(peer->sock, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));

    // Connect to server
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(peer_port)
    };
    if (inet_pton(AF_INET, peer_ip, &server_addr.sin_addr) != 1) {
        set_error("Invalid server IP address");
        close(peer->sock);
        return -1;
    }
// [Previous code remains the same until the connect() call in rdma_connect_peer]

    if (connect(peer->sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        set_error("Failed to connect to server");
        close(peer->sock);
        return -1;
    }

    // Create QP for this peer
    peer->qp = create_qp(ctx);
    if (!peer->qp) return -1;

    // Initialize QP
    if (modify_qp_to_init(peer->qp, ctx->dev_port)) return -1;

    // Exchange connection information
    union ibv_gid gid;
    if (ibv_query_gid(ctx->context, ctx->dev_port, 0, &gid)) {
        set_error("Failed to query GID");
        return -1;
    }

    // Prepare local connection info
    peer->local_info = (rdma_conn_info){
        .qp_num = peer->qp->qp_num,
        .lid = ctx->port_attr.lid,
        .psn = rand() & 0xFFFFFF
    };
    memcpy(peer->local_info.gid, &gid, sizeof(gid));
    memcpy(peer->local_info.ip, ctx->ip, sizeof(peer->local_info.ip) - 1);
    peer->local_info.ip[sizeof(peer->local_info.ip) - 1] = '\0';
    peer->local_info.port = ctx->port;

    // Send our info first
    if (write(peer->sock, &peer->local_info, sizeof(peer->local_info)) != sizeof(peer->local_info)) {
        set_error("Failed to send local info");
        return -1;
    }

    // Then receive server's info
    if (read(peer->sock, &peer->remote_info, sizeof(peer->remote_info)) != sizeof(peer->remote_info)) {
        set_error("Failed to receive remote info");
        return -1;
    }

    // Move QP to RTR and RTS states
    if (modify_qp_to_rtr(peer->qp, &peer->remote_info, ctx->dev_port)) return -1;
    if (modify_qp_to_rts(peer->qp, peer->local_info.psn)) return -1;

    peer->state = RDMA_CONN_CONNECTED;
    ctx->num_peers++;
    return ctx->num_peers - 1;
}

// Server accepting peer connection
int rdma_accept_peer(rdma_context *ctx) {
    if (!ctx->is_server) {
        set_error("Not a server context");
        return -1;
    }

    if (ctx->num_peers >= MAX_PEERS) {
        set_error("Maximum number of peers reached");
        return -1;
    }

    // Create listening socket
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        set_error("Failed to create socket");
        return -1;
    }

    // Set socket options
    int option = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));

    // Bind and listen
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(ctx->port)
    };
    if (inet_pton(AF_INET, ctx->ip, &addr.sin_addr) != 1) {
        set_error("Invalid IP address");
        close(listen_sock);
        return -1;
    }

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr))) {
        set_error("Failed to bind socket");
        close(listen_sock);
        return -1;
    }

    if (listen(listen_sock, 1)) {
        set_error("Failed to listen");
        close(listen_sock);
        return -1;
    }

    // Accept connection
    rdma_peer_conn *peer = &ctx->peers[ctx->num_peers];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    peer->sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
    close(listen_sock);

    if (peer->sock < 0) {
        set_error("Failed to accept connection");
        return -1;
    }

    // Create QP for this peer
    peer->qp = create_qp(ctx);
    if (!peer->qp) return -1;

    // Initialize QP
    if (modify_qp_to_init(peer->qp, ctx->dev_port)) return -1;

    // Exchange connection information
    union ibv_gid gid;
    if (ibv_query_gid(ctx->context, ctx->dev_port, 0, &gid)) {
        set_error("Failed to query GID");
        return -1;
    }

    peer->local_info = (rdma_conn_info){
        .qp_num = peer->qp->qp_num,
        .lid = ctx->port_attr.lid,
        .psn = rand() & 0xFFFFFF
    };
    memcpy(peer->local_info.gid, &gid, sizeof(gid));
    memcpy(peer->local_info.ip, ctx->ip, sizeof(peer->local_info.ip) - 1);
    peer->local_info.ip[sizeof(peer->local_info.ip) - 1] = '\0';
    peer->local_info.port = ctx->port;

    // Receive client's info first
    if (read(peer->sock, &peer->remote_info, sizeof(peer->remote_info)) != sizeof(peer->remote_info)) {
        set_error("Failed to receive remote info");
        return -1;
    }

    // Then send our info
    if (write(peer->sock, &peer->local_info, sizeof(peer->local_info)) != sizeof(peer->local_info)) {
        set_error("Failed to send local info");
        return -1;
    }

    // Move QP to RTR and RTS states
    if (modify_qp_to_rtr(peer->qp, &peer->remote_info, ctx->dev_port)) return -1;
    if (modify_qp_to_rts(peer->qp, peer->local_info.psn)) return -1;

    peer->state = RDMA_CONN_CONNECTED;
    ctx->num_peers++;
    return ctx->num_peers - 1;
}

// Send data to peer
int rdma_send(rdma_context *ctx, int peer_idx, const void *data, size_t len) {
    if (peer_idx >= ctx->num_peers || peer_idx < 0) {
        set_error("Invalid peer index");
        return -1;
    }

    rdma_peer_conn *peer = &ctx->peers[peer_idx];
    if (peer->state != RDMA_CONN_CONNECTED) {
        set_error("Peer not connected");
        return -1;
    }

    // Copy data to registered memory
    memcpy(ctx->comm_buf, data, len);

    // Post send
    struct ibv_sge sge = {
        .addr = (uint64_t)ctx->comm_buf,
        .length = len,
        .lkey = ctx->mr->lkey
    };

    struct ibv_send_wr wr = {
        .wr_id = peer_idx,
        .sg_list = &sge,
        .num_sge = 1,
        .opcode = IBV_WR_SEND,
        .send_flags = IBV_SEND_SIGNALED
    };

    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(peer->qp, &wr, &bad_wr)) {
        set_error("Failed to post send");
        return -1;
    }

    // Wait for completion
    if (wait_for_completion(ctx) < 0) return -1;

    return len;
}

// Receive data from peer
int rdma_recv(rdma_context *ctx, int peer_idx, void *data, size_t max_len) {
    if (peer_idx >= ctx->num_peers || peer_idx < 0) {
        set_error("Invalid peer index");
        return -1;
    }

    rdma_peer_conn *peer = &ctx->peers[peer_idx];
    if (peer->state != RDMA_CONN_CONNECTED) {
        set_error("Peer not connected");
        return -1;
    }

    // Post receive buffer
    if (post_recv(ctx, peer_idx, ctx->comm_buf, max_len) < 0) {
        return -1;
    }

    // Wait for completion
    if (wait_for_completion(ctx) < 0) {
        return -1;
    }

    // Copy received data to user buffer
    memcpy(data, ctx->comm_buf, max_len);
    return max_len;
}

// Broadcast data to all peers
int rdma_broadcast(rdma_context *ctx, const void *data, size_t len) {
    for (int i = 0; i < ctx->num_peers; i++) {
        if (rdma_send(ctx, i, data, len) < 0) {
            return -1;
        }
    }
    return len;
}

// Sequential all-to-all communication
int rdma_sequential_alltoall(rdma_context *ctx, const void *send_buf, void *recv_buf, size_t msg_size) {
    if (!ctx || !send_buf || !recv_buf || !msg_size || ctx->num_peers <= 0) {
        set_error("Invalid parameters");
        return -1;
    }

    // Setup buffers in registered memory
    char *rdma_send_buf = ctx->comm_buf;
    char *rdma_recv_buf = rdma_send_buf + BUFFER_SIZE;
    char *rdma_combined = rdma_send_buf + (BUFFER_SIZE * 2);
    
    // Clear all buffers
    memset(rdma_send_buf, 0, BUFFER_SIZE);
    memset(rdma_recv_buf, 0, BUFFER_SIZE);
    memset(rdma_combined, 0, BUFFER_SIZE);

    // Copy our message to send buffer
    memcpy(rdma_send_buf, send_buf, msg_size);

    if (ctx->is_server) {
        printf("SERVER: Starting gather phase from %d peers\n", ctx->num_peers);
        
        // Post receives from all clients
        for (int i = 0; i < ctx->num_peers; i++) {
            struct ibv_sge recv_sge = {
                .addr = (uint64_t)(rdma_recv_buf + (i * msg_size)),
                .length = msg_size,
                .lkey = ctx->mr->lkey
            };

            struct ibv_recv_wr recv_wr = {
                .wr_id = i,
                .sg_list = &recv_sge,
                .num_sge = 1
            };

            struct ibv_recv_wr *bad_recv_wr;
            if (ibv_post_recv(ctx->peers[i].qp, &recv_wr, &bad_recv_wr)) {
                set_error("Failed to post receive");
                return -1;
            }
        }

        // Wait for all receives
        for (int i = 0; i < ctx->num_peers; i++) {
            struct ibv_wc wc;
            while (ibv_poll_cq(ctx->cq, 1, &wc) == 0);
            if (wc.status != IBV_WC_SUCCESS) {
                set_error("Receive failed with status: %d", wc.status);
                return -1;
            }
            printf("SERVER: Received from peer %d: '%s'\n", 
                   (int)wc.wr_id, rdma_recv_buf + (wc.wr_id * msg_size));
        }

        // Combine messages in temporary buffer
        char *temp_buf = malloc(BUFFER_SIZE);
        if (!temp_buf) {
            set_error("Failed to allocate temporary buffer");
            return -1;
        }
        memset(temp_buf, 0, BUFFER_SIZE);

        // Start with server's message
        int pos = snprintf(temp_buf, BUFFER_SIZE, "%s", (char*)send_buf);
        
        // Add each client's message
        for (int i = 0; i < ctx->num_peers; i++) {
            pos += snprintf(temp_buf + pos, BUFFER_SIZE - pos, "; %s", 
                          rdma_recv_buf + (i * msg_size));
        }

        // Add final semicolon
        if (pos < BUFFER_SIZE - 2) {
            strcat(temp_buf, ";");
        }

        memcpy(rdma_combined, temp_buf, BUFFER_SIZE);
        free(temp_buf);
        
        printf("SERVER: Combined message: '%s'\n", rdma_combined);

        // Send combined message to all clients
        for (int i = 0; i < ctx->num_peers; i++) {
            struct ibv_sge send_sge = {
                .addr = (uint64_t)rdma_combined,
                .length = BUFFER_SIZE,
                .lkey = ctx->mr->lkey
            };

            struct ibv_send_wr send_wr = {
                .wr_id = i + ctx->num_peers,
                .sg_list = &send_sge,
                .num_sge = 1,
                .opcode = IBV_WR_SEND,
                .send_flags = IBV_SEND_SIGNALED
            };

            struct ibv_send_wr *bad_send_wr;
            if (ibv_post_send(ctx->peers[i].qp, &send_wr, &bad_send_wr)) {
                set_error("Failed to send");
                return -1;
            }

            struct ibv_wc wc;
            while (ibv_poll_cq(ctx->cq, 1, &wc) == 0);
            if (wc.status != IBV_WC_SUCCESS) {
                set_error("Send failed with status: %d", wc.status);
                return -1;
            }
        }

        // Copy combined result to server's receive buffer
        memcpy(recv_buf, rdma_combined, BUFFER_SIZE);

    } else {
        printf("CLIENT: Sending message: '%s'\n", (char*)send_buf);

        // Post receive for combined message
        struct ibv_sge recv_sge = {
            .addr = (uint64_t)rdma_recv_buf,
            .length = BUFFER_SIZE,
            .lkey = ctx->mr->lkey
        };

        struct ibv_recv_wr recv_wr = {
            .wr_id = 0,
            .sg_list = &recv_sge,
            .num_sge = 1
        };

        struct ibv_recv_wr *bad_recv_wr;
        if (ibv_post_recv(ctx->peers[0].qp, &recv_wr, &bad_recv_wr)) {
            set_error("Failed to post receive");
            return -1;
        }

        // Send our message to server
        struct ibv_sge send_sge = {
            .addr = (uint64_t)rdma_send_buf,
            .length = msg_size,
            .lkey = ctx->mr->lkey
        };

        struct ibv_send_wr send_wr = {
            .wr_id = 1,
            .sg_list = &send_sge,
            .num_sge = 1,
            .opcode = IBV_WR_SEND,
            .send_flags = IBV_SEND_SIGNALED
        };

        struct ibv_send_wr *bad_send_wr;
        if (ibv_post_send(ctx->peers[0].qp, &send_wr, &bad_send_wr)) {
            set_error("Failed to send");
            return -1;
        }

        // Wait for send completion
        struct ibv_wc wc;
        while (ibv_poll_cq(ctx->cq, 1, &wc) == 0);
        if (wc.status != IBV_WC_SUCCESS) {
            set_error("Send failed with status: %d", wc.status);
            return -1;
        }

        // Wait for receive of combined message
        while (ibv_poll_cq(ctx->cq, 1, &wc) == 0);
        if (wc.status != IBV_WC_SUCCESS) {
            set_error("Receive failed with status: %d", wc.status);
            return -1;
        }

        // Copy combined result to client's receive buffer
        memcpy(recv_buf, rdma_recv_buf, BUFFER_SIZE);
        printf("CLIENT: Received combined: '%s'\n", (char*)recv_buf);
    }

    return 0;
}

// Disconnect peer
int rdma_disconnect_peer(rdma_context *ctx, int peer_idx) {
    if (peer_idx >= ctx->num_peers || peer_idx < 0) {
        set_error("Invalid peer index");
        return -1;
    }

    rdma_peer_conn *peer = &ctx->peers[peer_idx];
    
    // Close socket connection
    if (peer->sock >= 0) {
        close(peer->sock);
        peer->sock = -1;
    }

    // Destroy QP
    if (peer->qp) {
        ibv_destroy_qp(peer->qp);
        peer->qp = NULL;
    }

    peer->state = RDMA_CONN_INIT;
    return 0;
}

// Cleanup RDMA resources
void rdma_cleanup(rdma_context *ctx) {
    if (!ctx) return;

    // Disconnect all peers
    for (int i = 0; i < ctx->num_peers; i++) {
        rdma_disconnect_peer(ctx, i);
    }

    // Cleanup RDMA resources
    if (ctx->mr) {
        ibv_dereg_mr(ctx->mr);
    }
    if (ctx->comm_buf) {
        free(ctx->comm_buf);
    }
    if (ctx->cq) {
        ibv_destroy_cq(ctx->cq);
    }
    if (ctx->pd) {
        ibv_dealloc_pd(ctx->pd);
    }
    if (ctx->context) {
        ibv_close_device(ctx->context);
    }

    free(ctx);
}