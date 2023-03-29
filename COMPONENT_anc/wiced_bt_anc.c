/*
 * Copyright 2016-2021, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
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
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/** @file
 *
 * Alert Notification Client library.
 *
 *
 * Features demonstrated
 *  - GATT discovery and registration for the ANP notifications from the ANP server.
 *  - Processing to receive various notifications from the server on the peer device.
 *  - Passing notifications to the client application.
 *  - Passing ANP action commands to peer device.
 */
#include "wiced_bt_uuid.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_trace.h"
#include "wiced_memory.h"
#include "wiced_timer.h"
#include "wiced_bt_anc.h"
#include "wiced_result.h"
#include "string.h"
#include "wiced_bt_gatt_util.h"

#ifdef WICED_BT_TRACE_ENABLE
#define     ANC_LIB_TRACE                          WICED_BT_TRACE
#else
#define     ANC_LIB_TRACE(...)
#endif
#define WICED_BT_GATT_NOT_FOUND WICED_BT_GATT_ATTRIBUTE_NOT_FOUND
#define MAX_READ_LEN	(256)

/******************************************************
 *                      Constants
 ******************************************************/
// service discovery states
enum
{
    ANC_CLIENT_STATE_IDLE                                 = 0x00,
    ANC_CLIENT_STATE_CONNECTED                            = 0x01,
    ANC_CLIENT_STATE_DISCOVER_NEW_ALERT_CCCD              = 0x02,
    ANC_CLIENT_STATE_DISCOVER_UNREAD_ALERT_CCCD           = 0x03,
    ANC_CLIENT_STATE_SET_REQUIRED_CONTROL_ALERTS          = 0x04,
    ANC_CLIENT_STATE_SET_NEW_ALERT_CCCD                   = 0x05,
    ANC_CLIENT_STATE_SET_UNREAD_ALERT_CCCD                = 0x06,
    ANC_CLIENT_STATE_RESET_NEW_ALERT_CCCD                 = 0x07,
    ANC_CLIENT_STATE_RESET_UNREAD_ALERT_CCCD              = 0x08,
};

//#define MAX_SIMULTANIOUS_CONTROL_POINT_WRITES               5

/******************************************************
 *                     Structures
 ******************************************************/

typedef struct {
    wiced_bt_anc_callback_t *p_callback;/* Application's callback function */
    uint16_t conn_id;                   /* connection identifier */
    uint16_t anc_e_handle;              /* ANC discovery end handle */
    uint8_t  anc_current_state;         /* to avoid other requests during execution of previous request*/

    uint8_t control_alert_cmd_id; //[MAX_SIMULTANIOUS_CONTROL_POINT_WRITES];
    uint8_t control_alert_catergory_id; //[MAX_SIMULTANIOUS_CONTROL_POINT_WRITES];
//    uint8_t oldest_cp_write_req; /* To find the oldest control point write request.Used to find when processing the control point write repsonse */

    uint8_t enabled_new_alerts;
    uint8_t enabled_unread_alerts;

    /* during discovery below gets populated and gets used later on application request in connection state */

    uint16_t new_alert_char_handle;       /* new alerts characteristic handle */
    uint16_t new_alert_char_value_handle; /* new alerts characteristic value handle */
    uint16_t new_alert_cccd_handle;       /* new alerts descriptor handle */

    uint16_t unread_alert_char_handle;       /* unread alerts characteristic handle */
    uint16_t unread_alert_char_value_handle; /* unread alerts characteristic value handle */
    uint16_t unread_alert_cccd_handle;       /* unread alerts descriptor handle */

    uint16_t alert_notify_control_point_char_handle;    /* Control point to control notifications characteristic handle */
    uint16_t alert_notify_control_point_value_handle;   /* Control point to control notifications characteristic value handle */

    uint16_t supported_new_alert_category_handle;        /* Supported new alert category handle */
    uint16_t supported_new_alert_category_value_handle;  /* Supported new alert category value handle */

    uint16_t supported_unread_alert_category_handle;         /* Supported unread alert category handle */
    uint16_t supported_unread_alert_category_value_handle;   /* Supported unread alert category handle */

} anc_lib_cb_t;

/******************************************************
 *                Variables Definitions
 ******************************************************/
static anc_lib_cb_t anc_lib_data;

/******************************************************
 *               Function Prototypes
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
#if 0
wiced_bool_t control_point_cache_alloc( uint8_t *index )
{
    uint8_t i;

    for (i = 0; i < MAX_SIMULTANIOUS_CONTROL_POINT_WRITES; i++)
    {
        if (anc_lib_data.control_alert_cmd_id[i++] == 0)
            break;
        if (i == MAX_SIMULTANIOUS_CONTROL_POINT_WRITES)
            return FALSE;
    }

    *index = i;

    return TRUE;
}
#endif
void wiced_bt_anc_utils_sort( uint16_t char_arr[], uint8_t len )
{
    uint8_t i,j;
    uint16_t temp;

    for(i=0;i<len-1;i++)
    {
        for(j=0;j<len-i-1;j++)
        {
            if( char_arr[j] > char_arr[j+1] )
            {
                temp = char_arr[j];
                char_arr[j] = char_arr[j+1];
                char_arr[j+1] = temp;
            }
        }
    }
}

wiced_result_t wiced_bt_anc_init(wiced_bt_anc_callback_t *p_callback)
{
    memset(&anc_lib_data , 0, sizeof(anc_lib_data) );

    anc_lib_data.p_callback = p_callback;

    return WICED_SUCCESS;
}

void wiced_bt_anc_client_connection_up(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    anc_lib_data.conn_id = p_conn_status->conn_id;
    anc_lib_data.anc_current_state = ANC_CLIENT_STATE_CONNECTED;
}

void wiced_bt_anc_client_connection_down(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_anc_init(anc_lib_data.p_callback);
}

wiced_bt_gatt_status_t wiced_bt_anc_discover( uint16_t conn_id, uint16_t start_handle, uint16_t end_handle)
{
    if ((start_handle == 0) || (end_handle == 0))
        return WICED_BT_GATT_INVALID_HANDLE;

    anc_lib_data.anc_e_handle = end_handle;
    anc_lib_data.anc_current_state = ANC_CLIENT_STATE_CONNECTED;

    return wiced_bt_util_send_gatt_discover(conn_id, GATT_DISCOVER_CHARACTERISTICS, 0, start_handle, end_handle);
}

/*
 * While application performs GATT discovery it shall pass discovery results for
 * for the handles that belong to ANP service, to this function.
 * Process discovery results from the stack.  We are looking for 3 characteristics
 * new alert source, unread alert source, and control point.  First 2 have client
 * configuration descriptors (CCCD).
 */
void wiced_bt_anc_discovery_result(wiced_bt_gatt_discovery_result_t *p_data)
{
    ANC_LIB_TRACE("[%s]\n",__FUNCTION__);

    uint16_t an_cp_uuid = UUID_CHARACTERISTIC_ALERT_NOTIFICATION_CONTROL_POINT;
    uint16_t an_na_uuid = UUID_CHARACTERISTIC_NEW_ALERT;
    uint16_t an_ua_uuid = UUID_CHARACTERISTIC_UNREAD_ALERT_STATUS;
    uint16_t sup_na_uuid = UUID_CHARACTERISTIC_SUPPORTED_NEW_ALERT_CATEGORY;
    uint16_t sup_ua_uuid = UUID_CHARACTERISTIC_SUPPORTED_UNREAD_ALERT_CATEGORY;

    if (p_data->discovery_type == GATT_DISCOVER_CHARACTERISTICS)
    {
        // Result for characteristic discovery.  Save appropriate handle based on the UUID.
        wiced_bt_gatt_char_declaration_t *p_char = &p_data->discovery_data.characteristic_declaration;
        if(p_char->char_uuid.len == LEN_UUID_16)
        {
            if (memcmp(&p_char->char_uuid.uu.uuid16, &an_cp_uuid, 2) == 0)
            {
                anc_lib_data.alert_notify_control_point_char_handle = p_char->handle;
                anc_lib_data.alert_notify_control_point_value_handle = p_char->val_handle;
                ANC_LIB_TRACE("control hdl:%04x-%04x", anc_lib_data.alert_notify_control_point_char_handle, anc_lib_data.alert_notify_control_point_value_handle);
            }
            else if(memcmp(&p_char->char_uuid.uu.uuid16, &an_ua_uuid, 2) == 0)
            {
                anc_lib_data.unread_alert_char_handle = p_char->handle;
                anc_lib_data.unread_alert_char_value_handle = p_char->val_handle;
                ANC_LIB_TRACE("unread alert hdl:%04x-%04x", anc_lib_data.unread_alert_char_handle, anc_lib_data.unread_alert_char_value_handle);
            }
            else if(memcmp(&p_char->char_uuid.uu.uuid16, &an_na_uuid, 2) == 0)
            {
                anc_lib_data.new_alert_char_handle = p_char->handle;
                anc_lib_data.new_alert_char_value_handle = p_char->val_handle;
                ANC_LIB_TRACE("new alert hdl:%04x-%04x", anc_lib_data.new_alert_char_handle, anc_lib_data.new_alert_char_value_handle);
            }
            else if(memcmp(&p_char->char_uuid.uu.uuid16, &sup_na_uuid, 2) == 0)
            {
                anc_lib_data.supported_new_alert_category_handle = p_char->handle;
                anc_lib_data.supported_new_alert_category_value_handle = p_char->val_handle;
                ANC_LIB_TRACE("supported new alert category hdl:%04x-%04x", anc_lib_data.supported_new_alert_category_handle, anc_lib_data.supported_new_alert_category_value_handle);
            }
            else if(memcmp(&p_char->char_uuid.uu.uuid16, &sup_ua_uuid, 2) == 0)
            {
                anc_lib_data.supported_unread_alert_category_handle = p_char->handle;
                anc_lib_data.supported_unread_alert_category_value_handle = p_char->val_handle;
                ANC_LIB_TRACE("supported unread alert hdl:%04x-%04x", anc_lib_data.supported_unread_alert_category_handle, anc_lib_data.supported_unread_alert_category_value_handle);
            }
        }
    }
    else if((p_data->discovery_type == GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS) &&
            (p_data->discovery_data.char_descr_info.type.len == 2) &&
            (p_data->discovery_data.char_descr_info.type.uu.uuid16 == UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION))
    {
        // result for descriptor discovery, save appropriate handle based on the state
        if( anc_lib_data.anc_current_state == ANC_CLIENT_STATE_DISCOVER_NEW_ALERT_CCCD )
        {
            anc_lib_data.new_alert_cccd_handle = p_data->discovery_data.char_descr_info.handle;
            ANC_LIB_TRACE("new alert cccd_hdl hdl:%04x", anc_lib_data.new_alert_cccd_handle);
        }
        else if( anc_lib_data.anc_current_state == ANC_CLIENT_STATE_DISCOVER_UNREAD_ALERT_CCCD )
        {
            anc_lib_data.unread_alert_cccd_handle = p_data->discovery_data.char_descr_info.handle;
            ANC_LIB_TRACE("unread alert cccd_hdl hdl:%04x", anc_lib_data.unread_alert_cccd_handle);
        }
    }
}

/*
 * While application performs GATT discovery it shall pass discovery complete callbacks
 * for the ANC service to the ANC Library. This function initiates the next discovery
 * request or write request to configure the ANC service on the iOS device.
 */
void wiced_bt_anc_client_discovery_complete(wiced_bt_gatt_discovery_complete_t *p_data)
{
    uint16_t start_handle;
    uint16_t end_handle;
    wiced_bt_anc_event_data_t event_data;
    uint16_t char_handle_list[5];
    uint8_t len,i,pos = 0;

    ANC_LIB_TRACE("[%s] state:%d\n", __FUNCTION__, anc_lib_data.anc_current_state);

    /* Maintain an array to get the range of descriptors */
    char_handle_list[0] = anc_lib_data.new_alert_char_handle;
    char_handle_list[1] = anc_lib_data.alert_notify_control_point_char_handle;
    char_handle_list[2] = anc_lib_data.unread_alert_char_handle;
    char_handle_list[3] = anc_lib_data.supported_new_alert_category_handle;
    char_handle_list[4] = anc_lib_data.supported_unread_alert_category_handle;
    len = sizeof(char_handle_list)/sizeof(char_handle_list[0]);

    /* Sort the Array of Characteristic values to get the range */
    wiced_bt_anc_utils_sort( char_handle_list, len );

    if( p_data->discovery_type == GATT_DISCOVER_CHARACTERISTICS )
    {
        // done with ANC characteristics, start reading descriptor handles
        // make sure that all mandatory characteristics are present
        if ((anc_lib_data.new_alert_char_handle == 0) ||
            (anc_lib_data.new_alert_char_value_handle == 0) ||
            (anc_lib_data.alert_notify_control_point_char_handle == 0) ||
            (anc_lib_data.alert_notify_control_point_value_handle == 0) ||
            (anc_lib_data.supported_new_alert_category_handle == 0) ||
            (anc_lib_data.supported_new_alert_category_value_handle == 0) )
        {
            // something is very wrong
            ANC_LIB_TRACE("[%s] failed\n", __FUNCTION__);
            anc_lib_data.anc_current_state = ANC_CLIENT_STATE_IDLE;
            wiced_bt_anc_init(anc_lib_data.p_callback);
            event_data.discovery_result.conn_id = p_data->conn_id;
            event_data.discovery_result.status = WICED_BT_GATT_NOT_FOUND;
            anc_lib_data.p_callback(WICED_BT_ANC_DISCOVER_RESULT, &event_data);
            return;
        }

        /* Get the position of the new_alert handle in the sorted list */
        for(i=0;i<5;i++)
        {
            if( char_handle_list[i] == anc_lib_data.new_alert_char_handle )
            {
                pos = i;
            }
        }

        anc_lib_data.anc_current_state = ANC_CLIENT_STATE_DISCOVER_NEW_ALERT_CCCD;
        start_handle = anc_lib_data.new_alert_char_handle + 1;
        if( pos < 4 )
        {
            end_handle = char_handle_list[pos+1] - 1;
        }
        else
        {
            end_handle = anc_lib_data.anc_e_handle;
        }

        wiced_bt_util_send_gatt_discover(p_data->conn_id, GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS, UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                start_handle , end_handle);
    }
    else if(p_data->discovery_type == GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS)
    {
        if ( (anc_lib_data.anc_current_state == ANC_CLIENT_STATE_DISCOVER_NEW_ALERT_CCCD)
              && (anc_lib_data.supported_unread_alert_category_handle != 0) )
        {
            /* Get the position of the unread_alert handle in the sorted list */
            for(i=0; i<5; i++ )
            {
                if( char_handle_list[i] == anc_lib_data.unread_alert_char_handle )
                {
                    pos = i;
                }
            }

            anc_lib_data.anc_current_state = ANC_CLIENT_STATE_DISCOVER_UNREAD_ALERT_CCCD;
            start_handle = anc_lib_data.unread_alert_char_handle + 1;
            if( pos < 4 )
            {
                end_handle = char_handle_list[pos+1] - 1;
            }
            else
            {
                end_handle = anc_lib_data.anc_e_handle;
            }

            wiced_bt_util_send_gatt_discover(p_data->conn_id, GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS, UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION,
                                start_handle, end_handle);
        }
        else
        {
            // done with descriptor discovery.
            anc_lib_data.anc_current_state = ANC_CLIENT_STATE_CONNECTED;
            event_data.discovery_result.conn_id = p_data->conn_id;
            if (anc_lib_data.new_alert_cccd_handle)
                event_data.discovery_result.status = WICED_BT_GATT_SUCCESS;
            else
                event_data.discovery_result.status = WICED_BT_GATT_NOT_FOUND;
            anc_lib_data.p_callback(WICED_BT_ANC_DISCOVER_RESULT, &event_data);
        }
    }
}

wiced_bt_gatt_status_t wiced_bt_anc_read_server_supported_new_alerts( uint16_t conn_id )
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    uint8_t *p_read = NULL;
    p_read = wiced_bt_get_buffer(MAX_READ_LEN);

    if( ( anc_lib_data.conn_id == conn_id ) && ( anc_lib_data.supported_new_alert_category_value_handle ) && (NULL != p_read) )
    {
		status = wiced_bt_gatt_client_send_read_handle( conn_id, anc_lib_data.supported_new_alert_category_value_handle, 
														0, p_read, MAX_READ_LEN, GATT_AUTH_REQ_NONE );
    }

    return status;
}

wiced_bt_gatt_status_t wiced_bt_anc_read_server_supported_unread_alerts( uint16_t conn_id )
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    uint8_t *p_read = NULL;
    p_read = wiced_bt_get_buffer(MAX_READ_LEN);

    if( ( anc_lib_data.conn_id == conn_id ) && ( anc_lib_data.supported_unread_alert_category_value_handle ) && (NULL != p_read) )
    {
        status = wiced_bt_gatt_client_send_read_handle( conn_id, anc_lib_data.supported_unread_alert_category_value_handle, 
														0, p_read, MAX_READ_LEN, GATT_AUTH_REQ_NONE );
    }

    return status;
}

wiced_bt_gatt_status_t wiced_bt_anc_enable_new_alerts( uint16_t conn_id )
{
    wiced_bt_gatt_status_t status;
    // verify that CCCD has been discovered
    if ((anc_lib_data.new_alert_cccd_handle == 0))
    {
        return WICED_BT_GATT_NOT_FOUND;
    }

    if( (anc_lib_data.anc_current_state != ANC_CLIENT_STATE_SET_NEW_ALERT_CCCD) &&
        ( anc_lib_data.anc_current_state != ANC_CLIENT_STATE_RESET_NEW_ALERT_CCCD ) )
    {
        // Register for notifications
        anc_lib_data.enabled_new_alerts = 1;
        anc_lib_data.anc_current_state = ANC_CLIENT_STATE_SET_NEW_ALERT_CCCD;
        status = wiced_bt_util_set_gatt_client_config_descriptor( conn_id, anc_lib_data.new_alert_cccd_handle, GATT_CLIENT_CONFIG_NOTIFICATION );
    }
    else
    {
        status = WICED_BT_GATT_BUSY;
    }
    return status;
}

wiced_bt_gatt_status_t wiced_bt_anc_disable_new_alerts( uint16_t conn_id )
{
    wiced_bt_gatt_status_t status;

    // verify that CCCD has been discovered
    if ((anc_lib_data.new_alert_cccd_handle == 0))
    {
        return WICED_BT_GATT_NOT_FOUND;
    }

    if( (anc_lib_data.anc_current_state != ANC_CLIENT_STATE_SET_NEW_ALERT_CCCD)
        && ( anc_lib_data.anc_current_state != ANC_CLIENT_STATE_RESET_NEW_ALERT_CCCD ))
    {
        // Register for notifications
        anc_lib_data.enabled_new_alerts = 0;
        anc_lib_data.anc_current_state = ANC_CLIENT_STATE_RESET_NEW_ALERT_CCCD;
        status = wiced_bt_util_set_gatt_client_config_descriptor( conn_id, anc_lib_data.new_alert_cccd_handle, GATT_CLIENT_CONFIG_NONE );
    }
    else
    {
        status = WICED_BT_GATT_BUSY;
    }
    return status;
}

wiced_bt_gatt_status_t wiced_bt_anc_enable_unread_alerts( uint16_t conn_id )
{
    wiced_bt_gatt_status_t status;

    // verify that CCCD has been discovered
    if ((anc_lib_data.unread_alert_cccd_handle == 0))
    {
        return WICED_BT_GATT_NOT_FOUND;
    }

    if( (anc_lib_data.anc_current_state != ANC_CLIENT_STATE_SET_UNREAD_ALERT_CCCD)
        && ( anc_lib_data.anc_current_state != ANC_CLIENT_STATE_RESET_UNREAD_ALERT_CCCD ))
    {
        // Register for notifications
        anc_lib_data.enabled_unread_alerts = 1;
        status = wiced_bt_util_set_gatt_client_config_descriptor( conn_id, anc_lib_data.unread_alert_cccd_handle, GATT_CLIENT_CONFIG_NOTIFICATION );
    }
    else
    {
        status = WICED_BT_GATT_BUSY;
    }
    return status;

}

wiced_bt_gatt_status_t wiced_bt_anc_disable_unread_alerts( uint16_t conn_id )
{
    wiced_bt_gatt_status_t status;

    // verify that CCCD has been discovered
    if ((anc_lib_data.unread_alert_cccd_handle == 0))
    {
        return WICED_BT_GATT_NOT_FOUND;
    }

    if( (anc_lib_data.anc_current_state != ANC_CLIENT_STATE_SET_UNREAD_ALERT_CCCD)
        && ( anc_lib_data.anc_current_state != ANC_CLIENT_STATE_RESET_UNREAD_ALERT_CCCD ))
    {
        // Register for notifications
        anc_lib_data.enabled_unread_alerts = 0;
        status = wiced_bt_util_set_gatt_client_config_descriptor( conn_id, anc_lib_data.unread_alert_cccd_handle, GATT_CLIENT_CONFIG_NONE );
    }
    else
    {
        status = WICED_BT_GATT_BUSY;
    }
    return status;
}


wiced_bt_gatt_status_t wiced_bt_anc_control_required_alerts( uint16_t conn_id , wiced_bt_anp_alert_control_cmd_id_t cmd_id, wiced_bt_anp_alert_category_id_t category)
{
    uint8_t  *p_write_req;
    wiced_bt_gatt_write_hdr_t p_write_header = { 0 };
    uint8_t value[2];
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;

    if( ( anc_lib_data.conn_id == conn_id ) && ( anc_lib_data.alert_notify_control_point_value_handle ) )
    {
        p_write_req = wiced_bt_get_buffer(sizeof(value));
        if (p_write_req == NULL)
        {
            return WICED_BT_GATT_NO_RESOURCES;
        }
        memset( p_write_req, 0, sizeof(value) );

        p_write_header.handle = anc_lib_data.alert_notify_control_point_value_handle;
        p_write_header.offset   = 0;
        p_write_header.len = sizeof(value);
        p_write_header.auth_req = GATT_AUTH_REQ_NONE;

        value[0] = cmd_id;
        value[1] = category;
        memcpy( p_write_req, value, sizeof(value) );

        ANC_LIB_TRACE("Control Point Value handle :0x%02x Command %d Category %d\n", p_write_header.handle, cmd_id, category);
        anc_lib_data.control_alert_cmd_id       = cmd_id;
        anc_lib_data.control_alert_catergory_id = category;
        status = wiced_bt_gatt_client_send_write( conn_id, GATT_REQ_WRITE, &p_write_header, p_write_req, NULL );

        wiced_bt_free_buffer(p_write_req);
    }
    return status;
}

/*
 * Process write response from the stack.
 * Application passes it here if handle belongs to our service.
 */
void wiced_bt_anc_write_rsp(wiced_bt_gatt_operation_complete_t *p_data)
{
    wiced_bt_anc_event_data_t event_data;
    uint8_t i;

    ANC_LIB_TRACE("[%s] state:%02x rc:%d\n", __FUNCTION__, anc_lib_data.anc_current_state, p_data->status);

    anc_lib_data.anc_current_state = ANC_CLIENT_STATE_CONNECTED;

    memset(&event_data, 0, sizeof(event_data));

    if (p_data->response_data.handle == anc_lib_data.alert_notify_control_point_value_handle)
    {
        event_data.control_alerts_result.conn_id = p_data->conn_id;
        event_data.control_alerts_result.status = p_data->status;
        event_data.control_alerts_result.category_id = anc_lib_data.control_alert_catergory_id;
        event_data.control_alerts_result.control_point_cmd_id = anc_lib_data.control_alert_cmd_id;
        anc_lib_data.control_alert_catergory_id = 0;
        anc_lib_data.control_alert_cmd_id = 0;
#if 0
        if (++anc_lib_data.oldest_cp_write_req == MAX_SIMULTANIOUS_CONTROL_POINT_WRITES)
        {
            anc_lib_data.oldest_cp_write_req = 0;
        }
#endif
        anc_lib_data.p_callback(WICED_BT_ANC_CONTROL_ALERTS_RESULT, &event_data);
    }
    else if (p_data->response_data.handle == anc_lib_data.new_alert_cccd_handle)
    {
        event_data.enable_disable_alerts_result.conn_id = p_data->conn_id;
        event_data.enable_disable_alerts_result.status = p_data->status;
        if (anc_lib_data.enabled_new_alerts)
            anc_lib_data.p_callback(WICED_BT_ANC_ENABLE_NEW_ALERTS_RESULT, &event_data);
        else
            anc_lib_data.p_callback(WICED_BT_ANC_DISABLE_NEW_ALERTS_RESULT, &event_data);
    }
    else if (p_data->response_data.handle == anc_lib_data.unread_alert_cccd_handle)
    {
        event_data.enable_disable_alerts_result.conn_id = p_data->conn_id;
        event_data.enable_disable_alerts_result.status = p_data->status;
        if (anc_lib_data.enabled_new_alerts)
            anc_lib_data.p_callback(WICED_BT_ANC_ENABLE_UNREAD_ALERTS_RESULT, &event_data);
        else
            anc_lib_data.p_callback(WICED_BT_ANC_DISABLE_UNREAD_ALERTS_RESULT, &event_data);
    }
}

/*
 * Process read response from the stack.
 * Application passes it here if handle belongs to our service.
 */
void wiced_bt_anc_read_rsp(wiced_bt_gatt_operation_complete_t *p_data)
{
    wiced_bt_anc_event_data_t event_data;

    ANC_LIB_TRACE("[%s] state:%02x rc:%d\n", __FUNCTION__, anc_lib_data.anc_current_state, p_data->status);

    if( anc_lib_data.anc_current_state != ANC_CLIENT_STATE_CONNECTED )
    {
        ANC_LIB_TRACE("Illegal State: %d\n",anc_lib_data.anc_current_state);
        anc_lib_data.anc_current_state = ANC_CLIENT_STATE_IDLE;
    }
    else
    {
        if( p_data->response_data.att_value.handle == anc_lib_data.supported_new_alert_category_value_handle )
        {
            ANC_LIB_TRACE(" [%s] Read Supported New Alerts: handle: %x\n",__FUNCTION__,p_data->response_data.att_value.handle);

            if( p_data->response_data.att_value.len == 2 )
            {
                event_data.supported_new_alerts_result.supported_alerts =
                    p_data->response_data.att_value.p_data[0] + (p_data->response_data.att_value.p_data[1] << 8);
                event_data.supported_new_alerts_result.conn_id = p_data->conn_id;
                event_data.supported_new_alerts_result.status = p_data->status;
                anc_lib_data.p_callback(WICED_BT_ANC_READ_SUPPORTED_NEW_ALERTS_RESULT, &event_data);
            }
        }
        else if( p_data->response_data.att_value.handle == anc_lib_data.supported_unread_alert_category_value_handle )
        {
            ANC_LIB_TRACE(" [%s] Read Supported Unread Alerts: handle: %x\n",__FUNCTION__,p_data->response_data.att_value.handle);

            if( p_data->response_data.att_value.len == 2 )
            {
                event_data.supported_unread_alerts_result.conn_id = p_data->conn_id;
                event_data.supported_unread_alerts_result.status = p_data->status;
                event_data.supported_unread_alerts_result.supported_alerts =
                    p_data->response_data.att_value.p_data[0] + (p_data->response_data.att_value.p_data[1] << 8);
                anc_lib_data.p_callback(WICED_BT_ANC_READ_SUPPORTED_UNREAD_ALERTS_RESULT, &event_data);
            }
        }
    }
}

wiced_bt_gatt_status_t wiced_bt_anc_recover_new_alerts_from_conn_loss(uint16_t conn_id, wiced_bt_anp_alert_control_cmd_id_t cmd_id, wiced_bt_anp_alert_category_id_t category)
{
    wiced_bt_gatt_status_t result;

    result = wiced_bt_anc_enable_new_alerts(conn_id);
    if( result == WICED_SUCCESS )
    {
        result = wiced_bt_anc_control_required_alerts( conn_id , cmd_id, category);
    }
    return result;
}

wiced_bt_gatt_status_t wiced_bt_anc_recover_new_unread_alerts_from_conn_loss(uint16_t conn_id, wiced_bt_anp_alert_control_cmd_id_t cmd_id, wiced_bt_anp_alert_category_id_t category)
{
    wiced_bt_gatt_status_t result;

    result = wiced_bt_anc_enable_unread_alerts(conn_id);
    if( result == WICED_SUCCESS )
    {
        result = wiced_bt_anc_control_required_alerts( conn_id , cmd_id, category);
    }
    return result;
}

void wiced_bt_anc_client_process_notification(wiced_bt_gatt_operation_complete_t *p_data)
{
    uint16_t handle = p_data->response_data.att_value.handle;
    uint8_t  *data  = p_data->response_data.att_value.p_data;
    uint16_t len    = p_data->response_data.att_value.len;
    wiced_bt_anc_event_data_t event_data;

    char buffer[20];

    if( handle == anc_lib_data.new_alert_char_value_handle )
    {
        event_data.new_alert_notification.conn_id = p_data->conn_id;

        if( p_data->response_data.att_value.len )
        {
            /* check if the received alert category is valid */
            if(p_data->response_data.att_value.p_data[0] <= ANP_ALERT_CATEGORY_ID_INSTANT_MESSAGE)
            {
                event_data.new_alert_notification.new_alert_type = p_data->response_data.att_value.p_data[0];
                /* notify new alert */
                event_data.new_alert_notification.new_alert_count = p_data->response_data.att_value.p_data[1];
                event_data.new_alert_notification.p_last_alert_data = buffer;
                memcpy(event_data.new_alert_notification.p_last_alert_data,
                        &p_data->response_data.att_value.p_data[2], p_data->response_data.att_value.len - 2 );
                event_data.new_alert_notification.p_last_alert_data[p_data->response_data.att_value.len - 2] = '\0';
                anc_lib_data.p_callback(WICED_BT_ANC_EVENT_NEW_ALERT_NOTIFICATION, &event_data);
            }
         }
    }
    else if( handle == anc_lib_data.unread_alert_char_value_handle )
    {
        event_data.unread_alert_notification.conn_id = p_data->conn_id;

        if( p_data->response_data.att_value.len == 2 )
        {
            /* check if the received alert category is valid */
            if(p_data->response_data.att_value.p_data[0] <= ANP_ALERT_CATEGORY_ID_INSTANT_MESSAGE)
            {
                /* notify unread alert */
                event_data.unread_alert_notification.unread_alert_type = p_data->response_data.att_value.p_data[0];
                event_data.unread_alert_notification.unread_count = p_data->response_data.att_value.p_data[1];
                anc_lib_data.p_callback(WICED_BT_ANC_EVENT_UNREAD_ALERT_NOTIFICATION, &event_data);
            }
         }
    }
    else
    {
        ANC_LIB_TRACE("ANC Notification bad handle:%02x, %d\n", (uint16_t)handle, len);
    }
}
