#include "rdma_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MSG_SIZE 64
#define PORT 5555
#define NUM_ROUNDS 100

int main(void) {
    const char *server_ip = "192.168.50.59";
    printf("Server starting on IP %s...\n", server_ip);

    rdma_context *ctx = rdma_init(server_ip, PORT, BUFFER_SIZE * 4, true);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize RDMA: %s\n", rdma_get_error());
        return 1;
    }

    // Accept 2 clients
    for (int i = 0; i < 2; i++) {
        printf("Waiting for client %d...\n", i + 1);
        if (rdma_accept_peer(ctx) < 0) {
            fprintf(stderr, "Failed to accept client %d: %s\n", i + 1, rdma_get_error());
            rdma_cleanup(ctx);
            return 1;
        }
        printf("Client %d connected\n", i + 1);
    }

    // Notify clients all are connected
    const char *start_msg = "START";
    for (int i = 0; i < 2; i++) {
        if (rdma_send(ctx, i, start_msg, strlen(start_msg) + 1) < 0) {
            fprintf(stderr, "Failed to send start message\n");
            rdma_cleanup(ctx);
            return 1;
        }
    }

    char send_msg[MSG_SIZE];
    char recv_buf[BUFFER_SIZE];  // Increased size for combined messages
    
    for (int round = 1; round <= NUM_ROUNDS; round++) {
        printf("\n=== Round %d ===\n", round);
        memset(send_msg, 0, sizeof(send_msg));
        memset(recv_buf, 0, sizeof(recv_buf));
        
        snprintf(send_msg, sizeof(send_msg), "Server Round %d", round);
        printf("Sending: '%s'\n", send_msg);

        if (rdma_sequential_alltoall(ctx, send_msg, recv_buf, MSG_SIZE) < 0) {
            fprintf(stderr, "Broadcast failed: %s\n", rdma_get_error());
            rdma_cleanup(ctx);
            return 1;
        }

        printf("Combined message: '%s'\n", recv_buf);
        // sleep(1);
    }

    printf("\nAll rounds completed. Cleaning up...\n");
    rdma_cleanup(ctx);
    return 0;
}