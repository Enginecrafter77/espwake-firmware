#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "espwake.h"

#define ESPWAKE_SCK_PASSWORD_LENGTH 16
#define ESPWAKE_SCK_SERVICE_PORT 9999

#define ESPWAKE_SCK_REQ_LENGTH 40
#define ESPWAKE_SCK_REPLY_LENGTH 4

#define ESPWAKE_PROTO_VERSION 1

typedef enum {
    ESPWAKE_PROTO_COMMAND_STATUS = 0,
    ESPWAKE_PROTO_COMMAND_START = 1,
    ESPWAKE_PROTO_COMMAND_SHUTDOWN = 2,
    ESPWAKE_PROTO_COMMAND_FORCEOFF = 3
} espwake_proto_req_cmd_t;

typedef enum {
    ESPWAKE_PROTO_DEV_STATUS_POWERED_OFF = 0,
    ESPWAKE_PROTO_DEV_STATUS_POWERING_OFF = 1,
    ESPWAKE_PROTO_DEV_STATUS_POWERING_ON = 2,
    ESPWAKE_PROTO_DEV_STATUS_POWERED_ON = 3,
    ESPWAKE_PROTO_DEV_STATUS_SUSPENDED = 4
} espwake_proto_device_status_t;

typedef enum {
    ESPWAKE_PROTO_REPLY_STATUS_OK = 0,
    ESPWAKE_PROTO_REPLY_STATUS_ERR_UNAUTHORIZED = 1,
    ESPWAKE_PROTO_REPLY_STATUS_ERR_INCOMPAT = 2,
    ESPWAKE_PROTO_REPLY_STATUS_ERR_UNKNOWN_CMD = 3,
    ESPWAKE_PROTO_REPLY_STATUS_ERR_UNKNOWN = 15,
} espwake_proto_reply_status_t;

typedef struct {
    uint8_t version;
    espwake_proto_req_cmd_t cmd;
    char password[ESPWAKE_SCK_PASSWORD_LENGTH];
} espwake_proto_req_t;

typedef struct {
    uint8_t version;
    espwake_proto_reply_status_t status;
    espwake_proto_device_status_t device_status;
} espwake_proto_reply_t;

typedef struct {
    espwake_t *espwake;
    char password[ESPWAKE_SCK_PASSWORD_LENGTH];
    xTaskHandle service_handle;
    union {
        uint8_t req[ESPWAKE_SCK_REQ_LENGTH];
        uint8_t reply[ESPWAKE_SCK_REPLY_LENGTH];
    } __buffers;
    espwake_proto_req_t request;
    espwake_proto_reply_t reply;
} espwake_sck_service_t;

void espwake_sck_service_create(espwake_sck_service_t *service, espwake_t *espwake);
void espwake_sck_service_set_password(espwake_sck_service_t *service, const char *password);
void espwake_sck_service_destroy(espwake_sck_service_t *service);
