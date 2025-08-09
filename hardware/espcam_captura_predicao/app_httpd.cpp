#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"

static const char* TAG = "app_httpd";

// Manipulador para "/status"
esp_err_t status_handler(httpd_req_t *req) {
    const char* resp = "{\"status\": \"ok\"}";
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// Inicializa servidor HTTP simples
httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_status = {
            .uri      = "/status",
            .method   = HTTP_GET,
            .handler  = status_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_status);
        ESP_LOGI(TAG, "Servidor HTTP iniciado");
    }
    return server;
}
