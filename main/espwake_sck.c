#include "espwake_sck.h"
#include <lwip/sockets.h>
#include <string.h>

static espwake_proto_device_status_t espwake_proto_map_state(espwake_state_t state) {
    switch (state) {
        case ESPWAKE_STATE_POWERED_OFF:
            return ESPWAKE_PROTO_DEV_STATUS_POWERED_OFF;
        case ESPWAKE_STATE_POWERING_OFF:
            return ESPWAKE_PROTO_DEV_STATUS_POWERING_OFF;
        case ESPWAKE_STATE_POWERING_ON:
            return ESPWAKE_PROTO_DEV_STATUS_POWERING_ON;
        case ESPWAKE_STATE_POWERED_ON:
            return ESPWAKE_PROTO_DEV_STATUS_POWERED_ON;
    }
    return ESPWAKE_PROTO_DEV_STATUS_POWERED_OFF;
}

static void espwake_req_read(espwake_proto_req_t *dest, const uint8_t *buffer) {
    memset(dest->password, 0, ESPWAKE_SCK_PASSWORD_LENGTH); // erase the password
    dest->version = (buffer[0] >> 0) & 0xF;
    dest->cmd = (buffer[0] >> 4) & 0xF;
    strncpy(dest->password, (const char*)(buffer + 1), ESPWAKE_SCK_PASSWORD_LENGTH);
}

static void espwake_reply_write(uint8_t *buffer, const espwake_proto_reply_t *reply) {
    memset(buffer, 0, 4); // erase the password
    buffer[0] = (((uint8_t)reply->status & 0xF) << 4) | (((uint8_t)reply->version & 0xF) << 0);
    buffer[1] = reply->device_status;
}

static inline void espwake_sck_read_request(espwake_sck_service_t *service) { espwake_req_read(&service->request, service->__buffers.req); }
static inline void espwake_sck_write_reply(espwake_sck_service_t *service) { espwake_reply_write(service->__buffers.reply, &service->reply); }

static bool espwake_sck_check_req_password(espwake_sck_service_t *service) {
    if (service->password[0] == 0) {
        return true;
    }
    return strncmp(service->request.password, service->password, ESPWAKE_SCK_PASSWORD_LENGTH) == 0;
}

#define ESPWAKE_SCK_REQUIRE_AUTH(S) do { if (!espwake_sck_check_req_password(S)) { (S)->reply.status = ESPWAKE_PROTO_REPLY_STATUS_ERR_UNAUTHORIZED; return; } } while(0)

static void espwake_sck_handle_request(espwake_sck_service_t *service) {
    service->reply.version = ESPWAKE_PROTO_VERSION;
    service->reply.device_status = espwake_proto_map_state(espwake_get_state(service->espwake));

    if (service->request.version != ESPWAKE_PROTO_VERSION) {
        service->reply.status = ESPWAKE_PROTO_REPLY_STATUS_ERR_INCOMPAT;
        return;
    }

    service->reply.status = ESPWAKE_PROTO_REPLY_STATUS_OK;
    switch(service->request.cmd) {
        case ESPWAKE_PROTO_COMMAND_START:
            ESPWAKE_SCK_REQUIRE_AUTH(service);
            espwake_power_on(service->espwake);
            break;
        case ESPWAKE_PROTO_COMMAND_SHUTDOWN:
            ESPWAKE_SCK_REQUIRE_AUTH(service);
            espwake_shutdown(service->espwake);
            break;
        case ESPWAKE_PROTO_COMMAND_FORCEOFF:
            ESPWAKE_SCK_REQUIRE_AUTH(service);
            espwake_force_off(service->espwake);
            break;
        case ESPWAKE_PROTO_COMMAND_STATUS:
            break;
        default:
            service->reply.status = ESPWAKE_PROTO_REPLY_STATUS_ERR_UNKNOWN_CMD;
            break;
    }
}

static void espwake_sck_service_task(void *arg) {
    espwake_sck_service_t *service = (espwake_sck_service_t*)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(struct sockaddr_in));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(ESPWAKE_SCK_SERVICE_PORT);
    bind(sock, (const struct sockaddr*)&listen_addr, sizeof(struct sockaddr_in));

    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(struct sockaddr_in);

    while(1) {
        recvfrom(sock, &service->__buffers.req, ESPWAKE_SCK_REQ_LENGTH, 0, (struct sockaddr*)&peer, &peer_len);
        espwake_sck_read_request(service);
        espwake_sck_handle_request(service);
        espwake_sck_write_reply(service);
        sendto(sock, &service->__buffers.reply, ESPWAKE_SCK_REPLY_LENGTH, 0, (const struct sockaddr*)&peer, peer_len);
    }
}

void espwake_sck_service_create(espwake_sck_service_t *service, espwake_t *espwake) {
    service->espwake = espwake;
    memset(&service->password, 0, ESPWAKE_SCK_PASSWORD_LENGTH);
    xTaskCreate(espwake_sck_service_task, "espwake_sck_service_task", 2048, service, 1, &service->service_handle);
}

void espwake_sck_service_set_password(espwake_sck_service_t *service, const char *password) {
    strncpy((char*)&service->password, password, ESPWAKE_SCK_PASSWORD_LENGTH);
}

void espwake_sck_service_destroy(espwake_sck_service_t *service) {
    vTaskDelete(service->service_handle);
}
