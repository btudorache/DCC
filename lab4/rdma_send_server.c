#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 12345
#define MSG_SIZE 1024

struct rdma_conn_info {
    uint32_t qp_num;
    uint16_t lid;
    uint32_t psn;
};

struct conn_context {
    struct ibv_context *context;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    struct ibv_mr *mr;
    char *buf;
};

static void setup_connection(struct conn_context *ctx, struct rdma_conn_info *remote_info) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sockfd, 1);
    printf("Waiting for client connection...\n");
    
    int client_fd = accept(sockfd, NULL, NULL);
    
    struct rdma_conn_info local_info = {
        .qp_num = ctx->qp->qp_num,
        .lid = 1,
        .psn = rand() & 0xffffff
    };
    
    write(client_fd, &local_info, sizeof(local_info));
    read(client_fd, remote_info, sizeof(*remote_info));
    
    close(client_fd);
    close(sockfd);
}

int main() {
    struct conn_context ctx = {};
    
    int num_devices;
    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
    ctx.context = ibv_open_device(dev_list[0]);
    ctx.pd = ibv_alloc_pd(ctx.context);
    ctx.cq = ibv_create_cq(ctx.context, 10, NULL, NULL, 0);
    
    struct ibv_qp_init_attr qp_init_attr = {
        .send_cq = ctx.cq,
        .recv_cq = ctx.cq,
        .qp_type = IBV_QPT_RC,
        .cap = {
            .max_send_wr = 10,
            .max_recv_wr = 10,
            .max_send_sge = 1,
            .max_recv_sge = 1
        }
    };
    
    ctx.qp = ibv_create_qp(ctx.pd, &qp_init_attr);
    ctx.buf = malloc(MSG_SIZE);
    ctx.mr = ibv_reg_mr(ctx.pd, ctx.buf, MSG_SIZE, 
                        IBV_ACCESS_LOCAL_WRITE | 
                        IBV_ACCESS_REMOTE_WRITE | 
                        IBV_ACCESS_REMOTE_READ);

    struct ibv_qp_attr qp_attr = {
        .qp_state = IBV_QPS_INIT,
        .pkey_index = 0,
        .port_num = 1,
        .qp_access_flags = IBV_ACCESS_REMOTE_WRITE
    };
    
    ibv_modify_qp(ctx.qp, &qp_attr,
                  IBV_QP_STATE |
                  IBV_QP_PKEY_INDEX |
                  IBV_QP_PORT |
                  IBV_QP_ACCESS_FLAGS);

    struct rdma_conn_info remote_info;
    setup_connection(&ctx, &remote_info);
    
    qp_attr.qp_state = IBV_QPS_RTR;
    qp_attr.path_mtu = IBV_MTU_1024;
    qp_attr.dest_qp_num = remote_info.qp_num;
    qp_attr.rq_psn = remote_info.psn;
    qp_attr.max_dest_rd_atomic = 1;
    qp_attr.min_rnr_timer = 12;
    qp_attr.ah_attr.dlid = remote_info.lid;
    qp_attr.ah_attr.sl = 0;
    qp_attr.ah_attr.src_path_bits = 0;
    qp_attr.ah_attr.port_num = 1;
    
    ibv_modify_qp(ctx.qp, &qp_attr,
                  IBV_QP_STATE |
                  IBV_QP_AV |
                  IBV_QP_PATH_MTU |
                  IBV_QP_DEST_QPN |
                  IBV_QP_RQ_PSN |
                  IBV_QP_MAX_DEST_RD_ATOMIC |
                  IBV_QP_MIN_RNR_TIMER);
    
    qp_attr.qp_state = IBV_QPS_RTS;
    qp_attr.timeout = 14;
    qp_attr.retry_cnt = 7;
    qp_attr.rnr_retry = 7;
    qp_attr.sq_psn = remote_info.psn;
    qp_attr.max_rd_atomic = 1;
    
    ibv_modify_qp(ctx.qp, &qp_attr,
                  IBV_QP_STATE |
                  IBV_QP_TIMEOUT |
                  IBV_QP_RETRY_CNT |
                  IBV_QP_RNR_RETRY |
                  IBV_QP_SQ_PSN |
                  IBV_QP_MAX_QP_RD_ATOMIC);

    strcpy(ctx.buf, "Hello from server!");
    
    struct ibv_sge sge = {
        .addr = (uint64_t)ctx.buf,
        .length = strlen(ctx.buf) + 1,
        .lkey = ctx.mr->lkey
    };

    struct ibv_send_wr wr = {
        .wr_id = 1,
        .sg_list = &sge,
        .num_sge = 1,
        .opcode = IBV_WR_SEND,
        .send_flags = IBV_SEND_SIGNALED
    };

    struct ibv_send_wr *bad_wr;
    ibv_post_send(ctx.qp, &wr, &bad_wr);

    struct ibv_wc wc;
    while (ibv_poll_cq(ctx.cq, 1, &wc) == 0);
    printf("Message sent\n");

    ibv_dereg_mr(ctx.mr);
    free(ctx.buf);
    ibv_destroy_qp(ctx.qp);
    ibv_destroy_cq(ctx.cq);
    ibv_dealloc_pd(ctx.pd);
    ibv_close_device(ctx.context);
    ibv_free_device_list(dev_list);

    return 0;
}