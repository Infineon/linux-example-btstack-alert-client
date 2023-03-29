/******************************************************************************
 * (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * This software, including source code, documentation and related materials
 * ("Software"), is owned by Cypress Semiconductor Corporation or one of its
 * subsidiaries ("Cypress") and is protected by and subject to worldwide patent
 * protection (United States and foreign), United States copyright laws and
 * international treaty provisions. Therefore, you may use this Software only
 * as provided in the license agreement accompanying the software package from
 * which you obtained this Software ("EULA").
 *
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software source
 * code solely for use in connection with Cypress's integrated circuit products.
 * Any reproduction, modification, translation, compilation, or representation
 * of this Software except as specified above is prohibited without the express
 * written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer of such
 * system or application assumes all risk of such use and in doing so agrees to
 * indemnify Cypress against all liability.
 ******************************************************************************/
/******************************************************************************
 * File Name: bt_app_ans.c
 *
 * Description:
 * This is the source code for the LE Alert Notification Server Example
 *
 * Related Document: See README.md
 *******************************************************************************/

/*******************************************************************************
 *                                   INCLUDES
 *******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "wiced_memory.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_memory.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_gatt_util.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_trace.h"
#include "wiced_hal_nvram.h"
#include "wiced_bt_stack_platform.h"
#include "app_bt_utils/app_bt_utils.h"
#include "COMPONENT_anc/wiced_bt_anp.h"
#include "COMPONENT_anc/wiced_bt_anc.h"
#include "COMPONENT_anc/wiced_bt_gatt_util.h"
#include "app_bt_config/anc_gatt_db.h"
#include "app_bt_config/anc_bt_settings.h"
#include "app_bt_config/anc_gap.h"
#include "bt_app_anc.h"

/*******************************************************************************
 *                                   MACROS
 *******************************************************************************/
#define BT_STACK_HEAP_SIZE (0xF000)
#define ANC_LOCAL_KEYS_NVRAM_ID (WICED_NVRAM_VSID_START)
#define ANC_PAIRED_KEYS_NVRAM_ID (WICED_NVRAM_VSID_START + 1)
#define MAX_KEY_SIZE (0x10U)
#define ANC_DISCOVERY_STATE_SERVICE (0)
#define ANC_DISCOVERY_STATE_ANC (1)

/*******************************************************************************
 *                    STRUCTURES AND ENUMERATIONS
 *******************************************************************************/
typedef struct
{
    uint16_t conn_id;
    uint8_t discovery_state;
    uint16_t anc_s_handle;
    uint16_t anc_e_handle;
    wiced_bt_device_address_t remote_addr;
    wiced_bt_ble_address_type_t addr_type;
} bt_app_anc_app_state_t;

/******************************************************************************
 *                                EXTERNS
 ******************************************************************************/
extern wiced_bt_heap_t *p_default_heap;
extern const wiced_bt_cfg_settings_t wiced_bt_cfg_settings;

/*******************************************************************************
 *                           GLOBAL VARIABLES
 *******************************************************************************/
bt_app_anc_app_state_t anc_app_state;

/* Context of command that is failed due to gatt insufficient authentication.
 * Need to re trigger the command after anc establish authentication with ans
 *  anc_pending_cmd_context[0] is the opcode saved
 *  anc_pending_cmd_context[1] and anc_pending_cmd_context[2] used in case of
 *  only Control Alerts respectively hold
 *   wiced_bt_anp_alert_control_cmd_id_t and
 *   wiced_bt_anp_alert_category_id_t
 */
uint8_t anc_pending_cmd_context[3] = {0};

/*******************************************************************************
 *                           FUNCTION DECLARATIONS
 *******************************************************************************/
static wiced_result_t bt_app_anc_management_callback(wiced_bt_management_evt_t event,
                                                     wiced_bt_management_evt_data_t *p_event_data);
static void bt_app_anc_connection_up(wiced_bt_gatt_connection_status_t *p_conn_status);
static void bt_app_anc_connection_down(wiced_bt_gatt_connection_status_t *p_conn_status);

static wiced_bt_gatt_status_t bt_app_anc_gatts_callback(wiced_bt_gatt_evt_t event,
                                                        wiced_bt_gatt_event_data_t *p_data);
static void bt_app_anc_load_keys_to_addr_resolution_db(void);
static wiced_bool_t bt_app_anc_save_link_keys(wiced_bt_device_link_keys_t *p_keys);
static wiced_bool_t bt_app_anc_read_link_keys(wiced_bt_device_link_keys_t *p_keys);

static void bt_app_anc_callback(wiced_bt_anc_event_t event, wiced_bt_anc_event_data_t *p_data);

static void bt_app_anc_set_advertisement_data();
static wiced_bt_gatt_status_t bt_app_anc_gatt_operation_complete(wiced_bt_gatt_operation_complete_t *p_data);
static wiced_bt_gatt_status_t bt_app_anc_gatt_discovery_result(wiced_bt_gatt_discovery_result_t *p_data);
static wiced_bt_gatt_status_t bt_app_anc_gatt_discovery_complete(wiced_bt_gatt_discovery_complete_t *p_data);
static void bt_app_anc_start_pair(void);
static void bt_app_anc_process_write_rsp(wiced_bt_gatt_operation_complete_t *p_data);
static void bt_app_anc_process_read_rsp(wiced_bt_gatt_operation_complete_t *p_data);
static void bt_app_anc_notification_handler(wiced_bt_gatt_operation_complete_t *p_data);
static void bt_app_anc_trigger_pending_action(void);
static void bt_app_clear_anc_pending_cmd_context(void);
static const char *bt_app_alert_type_name(wiced_bt_anp_alert_category_id_t id);

/*******************************************************************************
 *                       FUNCTION DEFINITIONS
 *******************************************************************************/

/*******************************************************************************
 * Function Name: application_start
 ********************************************************************************
 * Summary:
 *  Set device configuration and start BT stack initialization. The actual
 *  application initialization will happen when stack reports that BT device
 *  is ready.
 *
 * Parameters: NONE
 *
 * Return: NONE
 *
 *******************************************************************************/
void application_start( void )
{
    wiced_result_t wiced_result;

    WICED_BT_TRACE("Bluetooth Alert Server Application\n");

    /* Register call back and configuration with stack */
    wiced_result = wiced_bt_stack_init(bt_app_anc_management_callback, 
                                                    &wiced_bt_cfg_settings);

    /* Check if stack initialization was successful */
    if (WICED_BT_SUCCESS == wiced_result)
    {
        WICED_BT_TRACE("Bluetooth Stack Initialization Successful \n");
        /* Create default heap */
        p_default_heap = wiced_bt_create_heap("default_heap", NULL, 
                                    BT_STACK_HEAP_SIZE, NULL, WICED_TRUE);
        if (p_default_heap == NULL)
        {
            WICED_BT_TRACE("Create default heap error: size %d\n", 
                                                    BT_STACK_HEAP_SIZE);
            exit(EXIT_FAILURE);
        }
        memset(&anc_app_state, 0, sizeof(anc_app_state));
        memset(anc_pending_cmd_context, 0, sizeof(anc_pending_cmd_context));
    }
    else
    {
        WICED_BT_TRACE("Bluetooth Stack Initialization Failed!! \n");
        exit(EXIT_FAILURE);
    }
}

/*******************************************************************************
 * Function Name: bt_app_anc_application_init
 ********************************************************************************
 * Summary:
 *   This function handles application level initialization tasks and is called
 *   from the BT management callback once the LE stack enabled event
 *   (BTM_ENABLED_EVT) is triggered This function is executed in the
 *   BTM_ENABLED_EVT management callback.
 *
 * Parameters:
 *   None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
void bt_app_anc_application_init( void )
{
    wiced_bt_gatt_status_t gatt_status = 0;

    WICED_BT_TRACE("wiced_bt_gatt_register: %d\n", gatt_status);
    /* Register with stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(bt_app_anc_gatts_callback);

    WICED_BT_TRACE("wiced_bt_gatt_db_init %d\n", gatt_status);

    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(WICED_TRUE, 0);

    /* Load the address resolution DB with the keys stored in the NVRAM */
    bt_app_anc_load_keys_to_addr_resolution_db();

    /* Set the advertising params and make the device discoverable */
    bt_app_anc_set_advertisement_data();
}

/*******************************************************************************
 * Function Name: bt_app_anc_set_advertisement_data
 ********************************************************************************
 * Summary:
 *   Sets the advertisement data for ANC
 *
 * Parameters:
 *  None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void bt_app_anc_set_advertisement_data( void )
{
    wiced_bt_ble_advert_elem_t adv_elem[3];
    uint8_t num_elem = 0;
    uint8_t flag = BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED;
    uint8_t power = 0;

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_FLAG;
    adv_elem[num_elem].len = sizeof(uint8_t);
    adv_elem[num_elem].p_data = &flag;
    num_elem++;

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len = strlen((char *)wiced_bt_cfg_settings.device_name);
    adv_elem[num_elem].p_data = (uint8_t *)wiced_bt_cfg_settings.device_name;
    num_elem++;

    wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_elem);
}

/*******************************************************************************
 * Function Name: bt_app_anc_start_advertisement
 ********************************************************************************
 * Summary:
 *   Sets the advertisement data for ANC
 *
 * Parameters:
 *   None
 *
 * Return:
 *   None
 *
 *******************************************************************************/
void bt_app_anc_start_advertisement()
{
    wiced_result_t result;
    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, 
                                                                    0, NULL);
    WICED_BT_TRACE("wiced_bt_start_advertisements:%d\n", result);
}

/*******************************************************************************
 * Function Name: bt_app_anc_callback
 ********************************************************************************
 * Summary:
 *   This is a Callback from the ANC Profile layer
 *
 * Parameters:
 *   event: ANC callback event from profile
 *   p_data: Pointer to the result of the operation initiated by ANC
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void bt_app_anc_callback(wiced_bt_anc_event_t event, 
                            wiced_bt_anc_event_data_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_SUCCESS;

    if (p_data == NULL)
    {
        WICED_BT_TRACE("GATT Callback Event Data is pointing to NULL \n");
        return;
    }
    switch (event)
    {
    case WICED_BT_ANC_DISCOVER_RESULT:
        WICED_BT_TRACE("ANC discover result: %d ", 
                                        p_data->discovery_result.status);
        result = p_data->discovery_result.status;
        break;

    case WICED_BT_ANC_READ_SUPPORTED_NEW_ALERTS_RESULT:
        WICED_BT_TRACE("ANC read supported new alerts: %d ", 
                            p_data->supported_new_alerts_result.status);
        WICED_BT_TRACE("Supported New Alerts on ANS: %d ",
                       p_data->supported_new_alerts_result.supported_alerts);
        result = p_data->supported_new_alerts_result.status;
        break;

    case WICED_BT_ANC_READ_SUPPORTED_UNREAD_ALERTS_RESULT:
        WICED_BT_TRACE("ANC read supported unread alerts: %d ", 
                                p_data->supported_unread_alerts_result.status);
        WICED_BT_TRACE("Supported Unread Alerts on ANS: %d ",
                       p_data->supported_unread_alerts_result.supported_alerts);
        result = p_data->supported_unread_alerts_result.status;
        break;

    case WICED_BT_ANC_CONTROL_ALERTS_RESULT:
        result = p_data->control_alerts_result.status;
        WICED_BT_TRACE("ANC control alerts result: %d ", 
                                        p_data->control_alerts_result.status);
        break;

    case WICED_BT_ANC_ENABLE_NEW_ALERTS_RESULT:
        WICED_BT_TRACE("ANC enable new alerts result: %d ", 
                                p_data->enable_disable_alerts_result.status);
        result = p_data->enable_disable_alerts_result.status;
        break;

    case WICED_BT_ANC_DISABLE_NEW_ALERTS_RESULT:
        WICED_BT_TRACE("ANC disable new alerts result: %d ", 
                                p_data->enable_disable_alerts_result.status);
        result = p_data->enable_disable_alerts_result.status;
        break;

    case WICED_BT_ANC_ENABLE_UNREAD_ALERTS_RESULT:
        WICED_BT_TRACE("ANC enable unread alerts result: %d ", 
                                p_data->enable_disable_alerts_result.status);
        result = p_data->enable_disable_alerts_result.status;
        break;

    case WICED_BT_ANC_DISABLE_UNREAD_ALERTS_RESULT:
        WICED_BT_TRACE("ANC disable unread alerts result: %d ", 
                                p_data->enable_disable_alerts_result.status);
        result = p_data->enable_disable_alerts_result.status;
        break;

    case WICED_BT_ANC_EVENT_NEW_ALERT_NOTIFICATION:
        WICED_BT_TRACE("New Alert type:%s Count:%d Last Alert Data:%s\n",
            bt_app_alert_type_name(p_data->new_alert_notification.new_alert_type),
            p_data->new_alert_notification.new_alert_count,
            p_data->new_alert_notification.p_last_alert_data);
        break;

    case WICED_BT_ANC_EVENT_UNREAD_ALERT_NOTIFICATION:
        WICED_BT_TRACE("Unread Alert type: %s Count: %d \n",
            bt_app_alert_type_name(p_data->unread_alert_notification.unread_alert_type),
            p_data->unread_alert_notification.unread_count);
        break;

    default:
        break;
    }
    if (result == WICED_BT_GATT_INSUF_AUTHENTICATION)
    {
        bt_app_anc_start_pair();
    }
    else
    {
        /* Pending command no more valid other 
           than authentication failure cases */
        bt_app_clear_anc_pending_cmd_context();
    }
}

/******************************************************************************
 * Function Name: bt_app_anc_management_callback
 ******************************************************************************
 * Summary:
 *   This is a Bluetooth stack event handler function to receive management
 *   events from the LE stack and process as per the application.
 *
 * Parameters:
 *   wiced_bt_management_evt_t event             : LE event code of one byte
 *                                                 length
 *   wiced_bt_management_evt_data_t *p_event_data: Pointer to LE management
 *                                                 event structures
 *
 * Return:
 *  wiced_result_t: Error code from WICED_RESULT_LIST or BT_RESULT_LIST
 *
 *****************************************************************************/
wiced_result_t bt_app_anc_management_callback(wiced_bt_management_evt_t event,
                                wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_bt_device_address_t bda = {0};
    wiced_bt_ble_advert_mode_t *p_adv_mode = NULL;
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_device_link_keys_t link_keys;
    uint8_t *p_keys;

    WICED_BT_TRACE("Bluetooth Management Event: 0x%x %s\n", event, 
                                        (char *)get_bt_event_name(event));
    switch (event)
    {
    case BTM_ENABLED_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        /* Bluetooth Controller and Host Stack Enabled */
        if (WICED_BT_SUCCESS == p_event_data->enabled.status)
        {
            wiced_bt_set_local_bdaddr(anc_bd_address, BLE_ADDR_PUBLIC);
            wiced_bt_dev_read_local_addr(bda);

            WICED_BT_TRACE("Bluetooth Enabled \n");
            WICED_BT_TRACE("Local Bluetooth Address: ");
            print_bd_address(bda);

            /* Perform application-specific initialization */
            wiced_bt_anc_init(&bt_app_anc_callback);
            bt_app_anc_application_init();
        }
        else
        {
            WICED_BT_TRACE("Bluetooth Enabling Failed \n");
        }
        break;

    case BTM_DISABLED_EVT:
        WICED_BT_TRACE("Bluetooth Disabled \n");
        break;

    case BTM_USER_CONFIRMATION_REQUEST_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        WICED_BT_TRACE("Numeric_value: %d \n",
                    p_event_data->user_confirmation_request.numeric_value);
        wiced_bt_dev_confirm_req_reply(WICED_BT_SUCCESS, 
                            p_event_data->user_confirmation_request.bd_addr);
        break;

    case BTM_PASSKEY_NOTIFICATION_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        WICED_BT_TRACE("PassKey Notification. BDA %B, Key %d \n", 
                            p_event_data->user_passkey_notification.bd_addr,
                            p_event_data->user_passkey_notification.passkey);
        wiced_bt_dev_confirm_req_reply(WICED_BT_SUCCESS, 
                            p_event_data->user_passkey_notification.bd_addr);
        break;

    case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        p_event_data->pairing_io_capabilities_ble_request.local_io_cap = BTM_IO_CAPABILITIES_NONE;
        p_event_data->pairing_io_capabilities_ble_request.oob_data = BTM_OOB_NONE;
        p_event_data->pairing_io_capabilities_ble_request.auth_req = BTM_LE_AUTH_REQ_SC_BOND;
        p_event_data->pairing_io_capabilities_ble_request.max_key_size = MAX_KEY_SIZE;
        p_event_data->pairing_io_capabilities_ble_request.init_keys =
            BTM_LE_KEY_PENC | BTM_LE_KEY_PID | BTM_LE_KEY_PCSRK | BTM_LE_KEY_LENC;
        p_event_data->pairing_io_capabilities_ble_request.resp_keys =
            BTM_LE_KEY_PENC | BTM_LE_KEY_PID | BTM_LE_KEY_PCSRK | BTM_LE_KEY_LENC;
        break;

    case BTM_PAIRING_COMPLETE_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        WICED_BT_TRACE("Pairing Complete: %d", 
                    p_event_data->pairing_complete.pairing_complete_info.ble.reason);
        break;

    case BTM_ENCRYPTION_STATUS_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        WICED_BT_TRACE("Encryption Status Event: bd (%B) res %d", 
                                        p_event_data->encryption_status.bd_addr,
                                        p_event_data->encryption_status.result);
        if (p_event_data->encryption_status.result == WICED_BT_SUCCESS)
            bt_app_anc_trigger_pending_action();
        else /* pending command no more valid to send if authentication fails */
            bt_app_clear_anc_pending_cmd_context();
        break;

    case BTM_SECURITY_REQUEST_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        wiced_bt_ble_security_grant(p_event_data->security_request.bd_addr, 
                                                            WICED_BT_SUCCESS);
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        bt_app_anc_save_link_keys(&p_event_data->paired_device_link_keys_update);
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        p_keys = (uint8_t *)&p_event_data->paired_device_link_keys_request;
        result = bt_app_anc_save_link_keys(&link_keys);
        /* Break if link key retrieval is failed or link key is not available. */
        if (result != WICED_SUCCESS)
        {
            result = WICED_BT_ERROR;
            WICED_BT_TRACE("\n Reading keys from NVRAM failed or link key not available.");
            break;
        }

        WICED_BT_TRACE("Keys read from NVRAM ");
        WICED_BT_TRACE("Result: %d \n", result);

        /* Compare the BDA */
        if (memcmp(&(link_keys.bd_addr), &(p_event_data->paired_device_link_keys_request.bd_addr),
                   sizeof(wiced_bt_device_address_t)) == 0)
        {
            memcpy(p_keys, (uint8_t *)&link_keys, sizeof(wiced_bt_device_link_keys_t));
            result = WICED_SUCCESS;
        }
        else
        {
            result = WICED_BT_ERROR;
            WICED_BT_TRACE("Key retrieval failure\n");
        }
        break;

    case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        /* save keys to NVRAM */
        p_keys = (uint8_t *)&p_event_data->local_identity_keys_update;
        wiced_hal_write_nvram(ANC_LOCAL_KEYS_NVRAM_ID, 
                    sizeof(wiced_bt_local_identity_keys_t), p_keys, &result);
        WICED_BT_TRACE("Local keys save to NVRAM result: %d \n", result);
        break;

    case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        /* read keys from NVRAM */
        p_keys = (uint8_t *)&p_event_data->local_identity_keys_request;
        wiced_hal_read_nvram(ANC_LOCAL_KEYS_NVRAM_ID, 
                    sizeof(wiced_bt_local_identity_keys_t), p_keys, &result);
        WICED_BT_TRACE("Local keys read from NVRAM result: %d \n", result);
        break;

    case BTM_BLE_SCAN_STATE_CHANGED_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        WICED_BT_TRACE("Scan State Change: %d\n", 
                                    p_event_data->ble_scan_state_changed);
        break;

    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        /* Advertisement State Changed */
        p_adv_mode = &p_event_data->ble_advert_state_changed;
        WICED_BT_TRACE("Advertisement State Change: %s\n", 
                                        get_bt_advert_mode_name(*p_adv_mode));

        if (BTM_BLE_ADVERT_OFF == *p_adv_mode)
        {
            /* Advertisement Stopped */
            WICED_BT_TRACE("Advertisement stopped\n");
        }
        else
        {
            /* Advertisement Started */
            WICED_BT_TRACE("Advertisement started\n");
        }
        break;

    case BTM_BLE_CONNECTION_PARAM_UPDATE:
        if (p_event_data == NULL)
        {
            WICED_BT_TRACE("Callback data pointer p_event_data is NULL \n");
            break;
        }
        WICED_BT_TRACE("Connection parameter update status:%d, Connection Interval: %d, \
                                       Connection Latency: %d, Connection Timeout: %d\n",
                       p_event_data->ble_connection_param_update.status,
                       p_event_data->ble_connection_param_update.conn_interval,
                       p_event_data->ble_connection_param_update.conn_latency,
                       p_event_data->ble_connection_param_update.supervision_timeout);
        break;

    default:
        break;
    }

    return result;
}

/******************************************************************************
 * Function Name: bt_app_anc_gatts_callback
 ******************************************************************************
 * Summary:
 *   Callback for various GATT events.  As this application performs only as a
 *   GATT server, some of the events are omitted.
 *
 * Parameters:
 *   wiced_bt_gatt_evt_t event                : LE GATT event code of one
 *                                              byte length
 *   wiced_bt_gatt_event_data_t *p_event_data : Pointer to LE GATT event
 *                                              structures
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *  in wiced_bt_gatt.h
 *
 *****************************************************************************/
static wiced_bt_gatt_status_t bt_app_anc_gatts_callback(wiced_bt_gatt_evt_t event,
                                                wiced_bt_gatt_event_data_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_SUCCESS;

    if (p_data == NULL)
    {
        WICED_BT_TRACE("GATT Callback Event Data is pointing to NULL \n");
        return result;
    }
    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        if (p_data->connection_status.connected)
        {
            bt_app_anc_connection_up(&p_data->connection_status);
        }
        else
        {
            bt_app_anc_connection_down(&p_data->connection_status);
        }
        break;

    case GATT_OPERATION_CPLT_EVT:
        result = bt_app_anc_gatt_operation_complete(&p_data->operation_complete);
        break;

    case GATT_DISCOVERY_RESULT_EVT:
        result = bt_app_anc_gatt_discovery_result(&p_data->discovery_result);
        break;

    case GATT_DISCOVERY_CPLT_EVT:
        result = bt_app_anc_gatt_discovery_complete(&p_data->discovery_complete);
        break;

    case GATT_ATTRIBUTE_REQUEST_EVT:
        /*Not supported for this as this acts as a client role */
        break;

    default:
        break;
    }

    return result;
}

/*******************************************************************************
* Function Name: bt_app_anc_connection_up
********************************************************************************
* Summary:
*   This function will be called when a connection is established
*
* Parameters:
*   p_conn_status  : Current status of the Connection

* Return:
*  None
*
*******************************************************************************/
void bt_app_anc_connection_up(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_device_link_keys_t keys;
    wiced_bt_gatt_status_t status;
    wiced_bt_ble_sec_action_type_t sec_act = BTM_BLE_SEC_ENCRYPT;

    if (p_conn_status->connected == TRUE)
    {
        WICED_BT_TRACE("Connected to ANS \n");

        anc_app_state.conn_id = p_conn_status->conn_id;

        /* save address of the connected device. */
        memcpy(anc_app_state.remote_addr, p_conn_status->bd_addr, 
                                    sizeof(anc_app_state.remote_addr));
        anc_app_state.addr_type = p_conn_status->addr_type;

        /* need to notify ANC library that the connection is up */
        wiced_bt_anc_client_connection_up(p_conn_status);

        /* Initialize WICED BT ANC library Start discovery */
        anc_app_state.discovery_state = ANC_DISCOVERY_STATE_SERVICE;
        anc_app_state.anc_s_handle = 0;
        anc_app_state.anc_e_handle = 0;

        /* perform primary service search */
        status = wiced_bt_util_send_gatt_discover(anc_app_state.conn_id, 
                    GATT_DISCOVER_SERVICES_ALL, UUID_ATTRIBUTE_PRIMARY_SERVICE,
                    1, 0xffff);
        WICED_BT_TRACE("Start discover status:%d\n", status);
    }
    else
    {
        WICED_BT_TRACE("Connection to ANS failed \n");
    }
}

/*******************************************************************************
* Function Name: bt_app_anc_connection_down
********************************************************************************
* Summary:
*   This function will be called when connection goes down
*
* Parameters:
*   p_conn_status  : Current status of the Connection

* Return:
*  None
*
*******************************************************************************/
void bt_app_anc_connection_down(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    anc_app_state.conn_id = 0;
    anc_app_state.anc_s_handle = 0;
    anc_app_state.anc_e_handle = 0;
    anc_app_state.discovery_state = ANC_DISCOVERY_STATE_SERVICE;

    WICED_BT_TRACE("Connection Down \n");
    /* pending command no more valid now */
    bt_app_clear_anc_pending_cmd_context();

    memset(anc_app_state.remote_addr, 0, sizeof(wiced_bt_device_address_t));
    /* tell library that connection is down */
    wiced_bt_anc_client_connection_down(p_conn_status);
}

/*******************************************************************************
 * Function Name: bt_app_anc_gatt_operation_complete
 ********************************************************************************
 * Summary:
 *   GATT operation started by the client has been completed
 *
 * Parameters:
 *  p_data     Pointer to LE operation data
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *  in wiced_bt_gatt.h
 *
 *******************************************************************************/
static wiced_bt_gatt_status_t bt_app_anc_gatt_operation_complete(wiced_bt_gatt_operation_complete_t *p_data)
{
    if (p_data == NULL)
    {
        WICED_BT_TRACE("GATT Operation Complete Callback Event Data is pointing to NULL \n");
        return WICED_BT_GATT_SUCCESS;
    }
    switch (p_data->op)
    {
    case GATTC_OPTYPE_WRITE_WITH_RSP:
    case GATTC_OPTYPE_WRITE_NO_RSP:
        bt_app_anc_process_write_rsp(p_data);
        break;

    case GATTC_OPTYPE_CONFIG_MTU:
        WICED_BT_TRACE("This app does not support op:%d\n", p_data->op);
        break;

    case GATTC_OPTYPE_NOTIFICATION:
        bt_app_anc_notification_handler(p_data);
        break;

    case GATTC_OPTYPE_READ_HANDLE:
    case GATTC_OPTYPE_READ_BY_TYPE:
        bt_app_anc_process_read_rsp(p_data);
        break;

    case GATTC_OPTYPE_INDICATION:
        WICED_BT_TRACE("This app does not support op:%d\n", p_data->op);
        break;
    }
    return WICED_BT_GATT_SUCCESS;
}

/*******************************************************************************
 * Function Name: bt_app_anc_gatt_discovery_result
 ********************************************************************************
 * Summary:
 *   Callback function to handle the result of GATT Discovery procedure
 *
 * Parameters:
 *  p_data     Pointer to GATT Discovery Data
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *  in wiced_bt_gatt.h
 *
 *******************************************************************************/
static wiced_bt_gatt_status_t bt_app_anc_gatt_discovery_result(wiced_bt_gatt_discovery_result_t *p_data)
{
    uint16_t alert_service_uuid = UUID_SERVICE_ALERT_NOTIFICATION;
    if (p_data == NULL)
    {
        WICED_BT_TRACE("GATT Discovery Result Data is pointing to NULL \n");
        return WICED_BT_GATT_SUCCESS;
    }
    switch (anc_app_state.discovery_state)
    {
    case ANC_DISCOVERY_STATE_ANC:
        wiced_bt_anc_discovery_result(p_data);
        break;

    default:
        if (p_data->discovery_type == GATT_DISCOVER_SERVICES_ALL)
        {
            if (p_data->discovery_data.group_value.service_type.len == LEN_UUID_16)
            {
                WICED_BT_TRACE("Start Handle: 0x%04x End Handle: 0x%04x \n",
                               p_data->discovery_data.group_value.s_handle,
                               p_data->discovery_data.group_value.e_handle);
                if (memcmp(&p_data->discovery_data.group_value.service_type.uu,
                           &alert_service_uuid, LEN_UUID_16) == 0)
                {
                    WICED_BT_TRACE("ANS Service found. Start Handle: %04x End Handle: %04x\n",
                                   p_data->discovery_data.group_value.s_handle,
                                   p_data->discovery_data.group_value.e_handle);
                    anc_app_state.anc_s_handle = p_data->discovery_data.group_value.s_handle;
                    anc_app_state.anc_e_handle = p_data->discovery_data.group_value.e_handle;
                }
            }
        }
        else
        {
            WICED_BT_TRACE("!!!! Invalid operation: %d\n", p_data->discovery_type);
        }
    }
    return WICED_BT_GATT_SUCCESS;
}

/*******************************************************************************
 * Function Name: bt_app_anc_gatt_discovery_complete
 ********************************************************************************
 * Summary:
 *   Callback function to handle the Completion of GATT Discovery procedure
 *
 * Parameters:
 *  p_data     Pointer to GATT Discovery Data
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *  in wiced_bt_gatt.h
 *
 *******************************************************************************/
static wiced_bt_gatt_status_t bt_app_anc_gatt_discovery_complete(wiced_bt_gatt_discovery_complete_t *p_data)
{
    wiced_result_t result;

    if (p_data == NULL)
    {
        WICED_BT_TRACE("GATT Discovery Complete Data is pointing to NULL \n");
        return WICED_BT_GATT_SUCCESS;
    }

    WICED_BT_TRACE("GATT Discovery: Connection Id %d Type %d State 0x%02x\n",
                   p_data->conn_id, p_data->discovery_type, anc_app_state.discovery_state);

    switch (anc_app_state.discovery_state)
    {
    case ANC_DISCOVERY_STATE_ANC:
        wiced_bt_anc_client_discovery_complete(p_data);
        break;

    default:
        if (p_data->discovery_type == GATT_DISCOVER_SERVICES_ALL)
        {
            WICED_BT_TRACE("ANS:Start Handle 0x%04x- End Handle 0x%04x\n",
                           anc_app_state.anc_s_handle, anc_app_state.anc_e_handle);

            /* If Alert Notification Service found tell
             * WICED BT anc library to start its discovery
             */
            if ((anc_app_state.anc_s_handle != 0) && (anc_app_state.anc_e_handle != 0))
            {
                anc_app_state.discovery_state = ANC_DISCOVERY_STATE_ANC;
                if (wiced_bt_anc_discover(anc_app_state.conn_id, anc_app_state.anc_s_handle,
                                          anc_app_state.anc_e_handle))
                {
                    break;
                }
            }
        }
        else
        {
            WICED_BT_TRACE("!!!! Invalid operation: %d\n", p_data->discovery_type);
        }
        break;
    }
    return WICED_BT_GATT_SUCCESS;
}

/*******************************************************************************
 * Function Name: bt_app_anc_process_write_rsp
 ********************************************************************************
 * Summary:
 *   Pass write response to appropriate client based on the attribute handle
 *
 * Parameters:
 *  p_data     Pointer to GATT Operation Data
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void bt_app_anc_process_write_rsp(wiced_bt_gatt_operation_complete_t *p_data)
{
    WICED_BT_TRACE("Write Response handle:%04x\n", p_data->response_data.handle);

    /* Verify that write response is for our service */
    if ((p_data->response_data.handle >= anc_app_state.anc_s_handle) &&
        (p_data->response_data.handle <= anc_app_state.anc_e_handle))
    {
        wiced_bt_anc_write_rsp(p_data);
    }
}

/*******************************************************************************
 * Function Name: bt_app_anc_process_read_rsp
 ********************************************************************************
 * Summary:
 *   Pass read response to appropriate client based on the attribute handle
 *
 * Parameters:
 *  p_data     Pointer to GATT Operation Data
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *  in wiced_bt_gatt.h
 *
 *******************************************************************************/
static void bt_app_anc_process_read_rsp(wiced_bt_gatt_operation_complete_t *p_data)
{
    WICED_BT_TRACE("Read Response Handle:0x%04x\n", p_data->response_data.handle);

    /* Verify that write response is for our service */
    if ((p_data->response_data.handle >= anc_app_state.anc_s_handle) &&
        (p_data->response_data.handle <= anc_app_state.anc_e_handle))
    {
        wiced_bt_anc_read_rsp(p_data);
    }
}

/*******************************************************************************
 * Function Name: bt_app_anc_notification_handler
 ********************************************************************************
 * Summary:
 *   Pass notification to appropriate client based on the attribute handle
 *
 * Parameters:
 *  p_data     Pointer to GATT Operation Data
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void bt_app_anc_notification_handler(wiced_bt_gatt_operation_complete_t *p_data)
{
    WICED_BT_TRACE("Notification Handle:0x%04x\n", p_data->response_data.att_value.handle);

    /* Verify that notification is for ANCS service and if true pass to
     * the library for processing
     */
    if ((p_data->response_data.att_value.handle >= anc_app_state.anc_s_handle) &&
        (p_data->response_data.att_value.handle < anc_app_state.anc_e_handle))
    {
        wiced_bt_anc_client_process_notification(p_data);
    }
}

/*******************************************************************************
 * Function Name: bt_app_clear_anc_pending_cmd_context
 ********************************************************************************
 * Summary:
 *   Clear the pending context
 *
 * Parameters:
 *  None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void bt_app_clear_anc_pending_cmd_context( void )
{
    memset(anc_pending_cmd_context, 0, sizeof(anc_pending_cmd_context));
}

/*******************************************************************************
 * Function Name: bt_app_anc_trigger_pending_action
 ********************************************************************************
 * Summary:
 *   Trigger the pending command
 *
 * Parameters:
 *  None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
static void bt_app_anc_trigger_pending_action( void )
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    if (!anc_pending_cmd_context[0])
    {
        WICED_BT_TRACE(" Trigger: No commands pending! \n");
        return;
    }
    gatt_status = bt_app_handle_usr_cmd(anc_pending_cmd_context[0],
                                        anc_pending_cmd_context[1],
                                        anc_pending_cmd_context[2]);

    if (gatt_status != WICED_BT_GATT_SUCCESS)
    {
        WICED_BT_TRACE("ANC trigger pending status %d \n", gatt_status);
    }

    /* should not keep trying if fail in sending pending command */
    bt_app_clear_anc_pending_cmd_context();
}

/******************************************************************************
 * Function Name: bt_app_handle_usr_cmd
 ******************************************************************************
 * Summary:
 *   Handles the ANC command and calls corresponding ANC API.
 *
 * Parameters:
 *  cmd: User Command
 *  cmd_id : Control Command ID in case of 'Control Required Alerts' option
 *  alert_category: Alert category in case of 'Control Required Alerts' option
 *
 * Return:
 *  wiced_bt_gatt_status_t: See possible status codes in wiced_bt_gatt_status_e
 *  in wiced_bt_gatt.h
 *
 *****************************************************************************/
wiced_bt_gatt_status_t bt_app_handle_usr_cmd(uint8_t cmd, uint8_t cmd_id, 
                                                            uint8_t alert_categ)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;
    switch (cmd)
    {
    case USR_ANC_COMMAND_READ_SERVER_SUPPORTED_NEW_ALERTS:
        gatt_status = wiced_bt_anc_read_server_supported_new_alerts(anc_app_state.conn_id);
        break;

    case USR_ANC_COMMAND_READ_SERVER_SUPPORTED_UNREAD_ALERTS:
        gatt_status = wiced_bt_anc_read_server_supported_unread_alerts(anc_app_state.conn_id);
        break;

    case USR_ANC_COMMAND_CONTROL_ALERTS:
        gatt_status = wiced_bt_anc_control_required_alerts(anc_app_state.conn_id,
                                    (wiced_bt_anp_alert_control_cmd_id_t)cmd_id,
                                    (wiced_bt_anp_alert_category_id_t)alert_categ);
        break;

    case USR_ANC_COMMAND_ENABLE_NTF_NEW_ALERTS:
        gatt_status = wiced_bt_anc_enable_new_alerts(anc_app_state.conn_id);
        break;

    case USR_ANC_COMMAND_ENABLE_NTF_UNREAD_ALERT_STATUS:
        gatt_status = wiced_bt_anc_enable_unread_alerts(anc_app_state.conn_id);
        break;

    case USR_ANC_COMMAND_DISABLE_NTF_NEW_ALERTS:
        gatt_status = wiced_bt_anc_disable_new_alerts(anc_app_state.conn_id);
        break;

    case USR_ANC_COMMAND_DISABLE_NTF_UNREAD_ALERT_STATUS:
        gatt_status = wiced_bt_anc_disable_unread_alerts(anc_app_state.conn_id);
        break;

    default:
        WICED_BT_TRACE("Unknown pending command \n");
        break;
    }

    if (gatt_status == WICED_BT_GATT_INSUF_AUTHENTICATION)
    {
        bt_app_anc_start_pair();
        WICED_BT_TRACE("Starting Pairing process %d \n", gatt_status);
        anc_pending_cmd_context[0] = cmd;
        anc_pending_cmd_context[1] = cmd_id;
        anc_pending_cmd_context[2] = alert_categ;
        return WICED_BT_GATT_SUCCESS;
    }

    if (gatt_status != WICED_BT_GATT_SUCCESS)
    {
        WICED_BT_TRACE("Operation Result %d \n", gatt_status);
    }
    return gatt_status;
}

/******************************************************************************
 * Function Name: bt_app_anc_start_pair
 ******************************************************************************
 * Summary:
 *   Handles the ANC pairing start request
 *
 * Parameters:
 *  None
 *
 * Return:
 *  None
 *
 *****************************************************************************/
void bt_app_anc_start_pair( void )
{
    wiced_result_t rc;

    rc = wiced_bt_dev_sec_bond(anc_app_state.remote_addr, 
                            anc_app_state.addr_type, BT_TRANSPORT_LE, 0, NULL);
    WICED_BT_TRACE("Start bond result: %d\n", rc);
}

/******************************************************************************
 * Function Name: bt_app_alert_type_name
 ******************************************************************************
 * Summary:
 *   Returns Alert name for the given alert type
 *
 * Parameters:
 *  id: Alert Category
 *
 * Return:
 *  Returns alert type name in string format
 *
 *****************************************************************************/
/*  */
const char *bt_app_alert_type_name(wiced_bt_anp_alert_category_id_t id)
{
    switch (id)
    {
    case ANP_ALERT_CATEGORY_ID_SIMPLE_ALERT:
        return "simple_alert";
        break;

    case ANP_ALERT_CATEGORY_ID_EMAIL:
        return "Email";
        break;

    case ANP_ALERT_CATEGORY_ID_NEWS:
        return "News";
        break;

    case ANP_ALERT_CATEGORY_ID_CALL:
        return "Call";
        break;

    case ANP_ALERT_CATEGORY_ID_MISSED_CALL:
        return "Missed Call";
        break;

    case ANP_ALERT_CATEGORY_ID_SMS_OR_MMS:
        return "SMS/MMS";
        break;

    case ANP_ALERT_CATEGORY_ID_VOICE_MAIL:
        return "Voice Mail";
        break;

    case ANP_ALERT_CATEGORY_ID_SCHEDULE_ALERT:
        return "Scheduled Alert";
        break;

    case ANP_ALERT_CATEGORY_ID_HIGH_PRI_ALERT:
        return "High Priority Alert";
        break;

    case ANP_ALERT_CATEGORY_ID_INSTANT_MESSAGE:
        return "Instant Message";
        break;
    }

    return NULL;
}

/*******************************************************************************
 * Function Name : bt_app_anc_load_keys_to_addr_resolution_db
 * *****************************************************************************
 * Summary :
 *    Read keys from the NVRAM and update address resolution database
 *
 * Parameters:
 *    None
 *
 * Return:
 *    None
 ******************************************************************************/
void bt_app_anc_load_keys_to_addr_resolution_db( void )
{
    uint8_t bytes_read;
    wiced_result_t result;
    wiced_bt_device_link_keys_t keys;

    bytes_read = wiced_hal_read_nvram(ANC_PAIRED_KEYS_NVRAM_ID, sizeof(keys), 
                                                    (uint8_t *)&keys, &result);

    WICED_BT_TRACE(" [%s] Read status %d bytes read %d \n", __FUNCTION__, 
                                                            result, bytes_read);

    /* if failed to read NVRAM, there is nothing saved at that location */
    if (result == WICED_SUCCESS)
    {
        result = wiced_bt_dev_add_device_to_address_resolution_db(&keys);
    }
}

/******************************************************************************
 * Function Name : bt_app_anc_save_link_keys
 * ****************************************************************************
 * Summary :
 *    This function is called to save keys generated as a result of pairing
 *    or keys update
 *
 * Parameters:
 *    p_keys: Link keys to be saved for the bonded device
 *
 * Return:
 *    wiced_bool_t: 1 if True and 0 if false
 *****************************************************************************/
wiced_bool_t bt_app_anc_save_link_keys(wiced_bt_device_link_keys_t *p_keys)
{
    uint8_t bytes_written;
    wiced_result_t result;

    bytes_written = wiced_hal_write_nvram(ANC_PAIRED_KEYS_NVRAM_ID, 
            sizeof(wiced_bt_device_link_keys_t), (uint8_t *)p_keys, &result);
    WICED_BT_TRACE("Saved %d bytes at id:%d \n", bytes_written, 
                                                    ANC_PAIRED_KEYS_NVRAM_ID);
    return (bytes_written == sizeof(wiced_bt_device_link_keys_t));
}

/******************************************************************************
 * Function Name : bt_app_anc_read_link_keys
 * ****************************************************************************
 * Summary :
 *    This function is called to read keys for specific bdaddr
 *
 * Parameters:
 *    p_keys: Link keys to be saved for the bonded device
 *
 * Return:
 *    wiced_bool_t: 1 if True and 0 if false
 *****************************************************************************/
wiced_bool_t bt_app_anc_read_link_keys(wiced_bt_device_link_keys_t *p_keys)
{
    uint8_t bytes_read;
    wiced_result_t result;

    bytes_read = wiced_hal_read_nvram(ANC_PAIRED_KEYS_NVRAM_ID, 
                 sizeof(wiced_bt_device_link_keys_t), (uint8_t *)p_keys, &result);
    WICED_BT_TRACE("Read %d bytes at id:%d \n", bytes_read, ANC_PAIRED_KEYS_NVRAM_ID);
    return (bytes_read == sizeof(wiced_bt_device_link_keys_t));
}
/* END OF FILE [] */
