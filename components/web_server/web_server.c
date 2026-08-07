#include "web_server.h"
#include <string.h>
#include "DHT22.h"
#include <esp_http_server.h>
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "zh_bh1750.h"
#include "conf.h"
#define TAG "web_server"
/* An HTTP GET handler */
static int s_pwm_value = 0; /* 0..100 (%) */
static esp_err_t send_json_response(httpd_req_t *req, cJSON *root)
{
    char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t ret = httpd_resp_sendstr(req, json_str);
    free(json_str);
    cJSON_Delete(root);
    return ret;
}
static esp_err_t send_error_json(httpd_req_t *req, const char *msg)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "message", msg);
    httpd_resp_set_status(req, "400 Bad Request");
    return send_json_response(req, root);
}
/* Lấy giá trị 1 query param từ query string. Trả 0 nếu thành công. */
static int get_query_param(httpd_req_t *req, const char *key, char *out, size_t out_len)
{
    char query[256];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
    {
        return -1;
    }
    if (httpd_query_key_value(query, key, out, out_len) != ESP_OK)
    {
        return -1;
    }
    return 0;
}
static esp_err_t get_sensors_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    int ret = readDHT();
    errorHandler(ret);
    // Đọc cảm biến thực tế
    float temp = getTemperature();
    float humi = getHumidity();
    float lux = 0.0;
    esp_err_t err = zh_bh1750_read(&bh1750_handle, &lux);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read BH1750");
    }
    int soil = 78; // adc_read_soil();

    char json_response[128];
    snprintf(json_response, sizeof(json_response),
             "{\"temp\":%.1f,\"humi\":%.1f,\"light\": %0.2f,\"soil\":%d}",
             temp, humi, lux, soil);

    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t set_control_handler(httpd_req_t *req)
{
    char buf[100];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK)
    {
        char device[16], state_str[8];
        if (httpd_query_key_value(buf, "device", device, sizeof(device)) == ESP_OK &&
            httpd_query_key_value(buf, "state", state_str, sizeof(state_str)) == ESP_OK)
        {

            int state = atoi(state_str);
            if (strcmp(device, "relay1") == 0)
            {
                gpio_set_level(RELAY1_GPIO, state); // Water Pump Pin
            }
            else if (strcmp(device, "relay2") == 0)
            {
                gpio_set_level(RELAY2_GPIO, state); // Growth Light Pin
            }
        }
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
}
static esp_err_t send_html(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)dashboard_html_start, dashboard_html_end - dashboard_html_start);
    return ESP_OK;
}
static esp_err_t pwm_handler(httpd_req_t *req)
{
    char value_str[8] = {0};

    if (get_query_param(req, "value", value_str, sizeof(value_str)) != 0)
    {
        return send_error_json(req, "Thieu tham so 'value'");
    }

    int value = atoi(value_str);
    if (value < 0 || value > 100)
    {
        return send_error_json(req, "'value' phai trong khoang 0..100");
    }

    s_pwm_value = value;

    /* Quy doi 0..100 (%) sang do phan giai timer (VD: 10 bit -> 0..1023) */
    uint32_t max_duty = (1 << PWM_RESOLUTION) - 1;
    uint32_t duty = (max_duty * value) / 100;

    ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);

    ESP_LOGI(TAG, "Set PWM -> %d%% (duty=%lu)", value, (unsigned long)duty);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "pwm", value);

    return send_json_response(req, root);
}
static esp_err_t system_get_handler(httpd_req_t *req)
{
    uint32_t heap = esp_get_free_heap_size();
    int64_t uptime_sec = esp_timer_get_time() / 1000000LL;

    int8_t rssi = 0;
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        rssi = ap_info.rssi;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "heap", heap);
    cJSON_AddNumberToObject(root, "uptime", (double)uptime_sec);
    cJSON_AddNumberToObject(root, "rssi", rssi);

    return send_json_response(req, root);
}

static const httpd_uri_t sensor = {
    .uri = "/api/sensors",
    .method = HTTP_GET,
    .handler = get_sensors_handler,
    .user_ctx = NULL};
static const httpd_uri_t control = {
    .uri = "/api/control",
    .method = HTTP_GET,
    .handler = set_control_handler,
    .user_ctx = NULL};
static const httpd_uri_t hello = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = send_html,
    .user_ctx = NULL};
static const httpd_uri_t system_uri = {
    .uri = "/api/system",
    .method = HTTP_GET,
    .handler = system_get_handler,
    .user_ctx = NULL};
static const httpd_uri_t pwm = {
    .uri = "/api/pwm",
    .method = HTTP_GET,
    .handler = pwm_handler,
    .user_ctx = NULL};
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &sensor);
        httpd_register_uri_handler(server, &control);
        httpd_register_uri_handler(server, &system_uri);
        httpd_register_uri_handler(server, &pwm);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

void disconnect_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

void connect_handler(void *arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}