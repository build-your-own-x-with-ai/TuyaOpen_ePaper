/*****************************************************************************
* | File      	:   http_image_display.c
* | Author      :   TuyaOpen Example
* | Function    :   Download image from HTTP and display on e-Paper
* | Info        :   Combines HTTP client with e-Paper display
*----------------
* | This version:   V1.0
* | Date        :   2025-12-15
******************************************************************************/

#include "tuya_cloud_types.h"
#include "http_client_interface.h"

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif

#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
#include "netconn_wired.h"
#endif

#include "EPD_4in26.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"

// HTTP Configuration
#define IMAGE_URL_HOST      "74.82.197.217"           // 修改为你的图片服务器地址
#define IMAGE_URL_PATH      "/image.bin"            // 修改为你的图片路径
#define HTTP_REQUEST_TIMEOUT 10000                  // 10 seconds

// WiFi Configuration
#define DEFAULT_WIFI_SSID   "1519"        // 修改为你的 WiFi SSID
#define DEFAULT_WIFI_PSWD   "15889629702"    // 修改为你的 WiFi 密码

// E-Paper Configuration
#define EPD_IMAGE_SIZE      ((EPD_4in26_WIDTH / 8) * EPD_4in26_HEIGHT)  // 48000 bytes for 800x480

static UBYTE *image_buffer = NULL;
static THREAD_HANDLE ty_app_thread = NULL;

/**
 * @brief Download image data from HTTP server
 * 
 * @param url_host HTTP server host
 * @param url_path HTTP request path
 * @param buffer Buffer to store downloaded data
 * @param buffer_size Maximum buffer size
 * @param downloaded_size Actual downloaded size
 * @return OPERATE_RET OPRT_OK on success
 */
static OPERATE_RET download_image(const char *url_host, const char *url_path, 
                                   UBYTE *buffer, uint32_t buffer_size, 
                                   uint32_t *downloaded_size)
{
    OPERATE_RET rt = OPRT_OK;
    http_client_response_t http_response = {0};
    http_client_header_t headers[] = {
        {.key = "Accept", .value = "application/octet-stream"}
    };

    PR_DEBUG("Downloading image from http://%s%s", url_host, url_path);

    // Send HTTP GET request
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .host = url_host,
            .method = "GET",
            .path = url_path,
            .headers = headers,
            .headers_count = sizeof(headers) / sizeof(http_client_header_t),
            .body = "",
            .body_length = 0,
            .timeout_ms = HTTP_REQUEST_TIMEOUT
        },
        &http_response
    );

    if (HTTP_CLIENT_SUCCESS != http_status) {
        PR_ERR("HTTP request failed with status: %d", http_status);
        rt = OPRT_LINK_CORE_HTTP_CLIENT_SEND_ERROR;
        goto exit;
    }

    PR_DEBUG("HTTP response status: %d", http_response.status_code);
    PR_DEBUG("Response body length: %d bytes", http_response.body_length);

    // Check response status
    if (http_response.status_code != 200) {
        PR_ERR("HTTP server returned error: %d", http_response.status_code);
        rt = OPRT_COM_ERROR;
        goto exit;
    }

    // Check buffer size
    if (http_response.body_length > buffer_size) {
        PR_ERR("Downloaded data (%d bytes) exceeds buffer size (%d bytes)", 
               http_response.body_length, buffer_size);
        rt = OPRT_MALLOC_FAILED;
        goto exit;
    }

    // Copy image data to buffer
    memcpy(buffer, http_response.body, http_response.body_length);
    *downloaded_size = http_response.body_length;
    
    PR_DEBUG("Successfully downloaded %d bytes", *downloaded_size);

exit:
    http_client_free(&http_response);
    return rt;
}

/**
 * @brief Display image on e-Paper
 * 
 * @param image_data Image data buffer
 * @param data_size Size of image data
 * @return OPERATE_RET OPRT_OK on success
 */
static OPERATE_RET display_image_on_epaper(UBYTE *image_data, uint32_t data_size)
{
    PR_DEBUG("Initializing e-Paper display...");
    
    // Initialize e-Paper
    if (DEV_Module_Init() != 0) {
        PR_ERR("e-Paper module initialization failed");
        return OPRT_INIT_MORE_THAN_ONCE;
    }

    // Initialize and clear display
    EPD_4in26_Init();
    EPD_4in26_Clear();
    DEV_Delay_ms(500);

    PR_DEBUG("Displaying image on e-Paper...");
    
    // Check if downloaded data size matches expected size
    if (data_size == EPD_IMAGE_SIZE) {
        // Display the downloaded image directly
        EPD_4in26_Display(image_data);
        PR_DEBUG("Image displayed successfully");
    } else {
        PR_WARN("Image size mismatch: expected %d bytes, got %d bytes", 
                EPD_IMAGE_SIZE, data_size);
        
        // Create a new image buffer and show error message
        UBYTE *display_buffer = (UBYTE *)malloc(EPD_IMAGE_SIZE);
        if (display_buffer == NULL) {
            PR_ERR("Failed to allocate display buffer");
            return OPRT_MALLOC_FAILED;
        }

        Paint_NewImage(display_buffer, EPD_4in26_WIDTH, EPD_4in26_HEIGHT, 0, WHITE);
        Paint_SelectImage(display_buffer);
        Paint_Clear(WHITE);
        Paint_DrawString_EN(10, 10, "Image size error!", &Font24, BLACK, WHITE);
        EPD_4in26_Display(display_buffer);
        
        free(display_buffer);
    }

    DEV_Delay_ms(2000);

    // Put display to sleep
    PR_DEBUG("Putting e-Paper to sleep...");
    EPD_4in26_Sleep();
    DEV_Delay_ms(2000);
    
    // Close module
    DEV_Module_Exit();
    
    return OPRT_OK;
}

/**
 * @brief Link status change callback
 */
static OPERATE_RET link_status_cb(void *data)
{
    int rt = OPRT_OK;
    static netmgr_status_e status = NETMGR_LINK_DOWN;
    
    if (status == (netmgr_status_e)data && NETMGR_LINK_UP == (netmgr_status_e)data) {
        return OPRT_OK;
    }
    status = (netmgr_status_e)data;

    if (status != NETMGR_LINK_UP) {
        PR_DEBUG("Network is down, waiting for connection...");
        return OPRT_OK;
    }

    PR_DEBUG("Network is up! Starting image download...");

    uint32_t downloaded_size = 0;

    // Allocate buffer for image data
    image_buffer = (UBYTE *)malloc(EPD_IMAGE_SIZE);
    if (image_buffer == NULL) {
        PR_ERR("Failed to allocate image buffer");
        return OPRT_MALLOC_FAILED;
    }

    // Download image from HTTP server
    rt = download_image(IMAGE_URL_HOST, IMAGE_URL_PATH, 
                       image_buffer, EPD_IMAGE_SIZE, &downloaded_size);
    
    if (rt != OPRT_OK) {
        PR_ERR("Failed to download image: %d", rt);
        free(image_buffer);
        image_buffer = NULL;
        return rt;
    }

    // Display image on e-Paper
    rt = display_image_on_epaper(image_buffer, downloaded_size);
    
    if (rt != OPRT_OK) {
        PR_ERR("Failed to display image: %d", rt);
    }

    // Free buffer
    free(image_buffer);
    image_buffer = NULL;

    return rt;
}

/**
 * @brief User main function
 */
void user_main(void)
{
    /* Basic initialization */
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    
    PR_NOTICE("HTTP Image Display Example");
    PR_NOTICE("Image URL: http://%s%s", IMAGE_URL_HOST, IMAGE_URL_PATH);

    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key = "dflfuap134ddlduq",
    });
    tal_sw_timer_init();
    tal_workq_init();
    
    /* Subscribe to link status change events */
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "http_image_display", link_status_cb, SUBSCRIBE_TYPE_NORMAL);

#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
    TUYA_LwIP_Init();
#endif

    /* Initialize network manager */
    netmgr_type_e type = 0;
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    type |= NETCONN_WIFI;
#endif
#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
    type |= NETCONN_WIRED;
#endif
    netmgr_init(type);

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    /* Connect to WiFi */
    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, DEFAULT_WIFI_SSID);
    strcpy(wifi_info.pswd, DEFAULT_WIFI_PSWD);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
#endif

    PR_DEBUG("Waiting for network connection...");
    return;
}

/**
 * @brief Main entry point for Linux
 */
#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    user_main();
    
    while (1) {
        tal_system_sleep(500);
    }
}
#else

/**
 * @brief Application thread for RTOS
 */
static void tuya_app_thread(void *arg)
{
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    user_main();

    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

/**
 * @brief Application main entry point for RTOS
 */
void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {8192, 4, "http_image_display"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif
