#include "rdma_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MSG_SIZE 64
#define PORT 5555
#define NUM_ROUNDS 100

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <client_id> <client_ip>\n", argv[0]);
        return 1;
    }

    int client_id = atoi(argv[1]);
    const char *client_ip = argv[2];
    const char *server_ip = "192.168.50.59";

    printf("Client %d starting on IP %s...\n", client_id, client_ip);

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

    // Allocate buffers with proper size
    char send_msg[MSG_SIZE];
    char *recv_buf = malloc(BUFFER_SIZE);  // Large enough for combined message
    if (!recv_buf) {
        fprintf(stderr, "Failed to allocate receive buffer\n");
        rdma_cleanup(ctx);
        return 1;
    }
    
    for (int round = 1; round <= NUM_ROUNDS; round++) {
        printf("\n=== Round %d ===\n", round);
        memset(send_msg, 0, sizeof(send_msg));
        memset(recv_buf, 0, BUFFER_SIZE);
        
        snprintf(send_msg, sizeof(send_msg), "Client %d Round %d", client_id, round);
        printf("Sending: '%s'\n", send_msg);

        if (rdma_sequential_alltoall(ctx, send_msg, recv_buf, MSG_SIZE) < 0) {
            fprintf(stderr, "Broadcast failed: %s\n", rdma_get_error());
            free(recv_buf);
            rdma_cleanup(ctx);
            return 1;
        }

        printf("Received combined: '%s'\n", recv_buf);
        // sleep(1);
    }

    printf("\nAll rounds completed. Cleaning up...\n");
    free(recv_buf);
    rdma_cleanup(ctx);
    return 0;
}