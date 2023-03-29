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
 * Set of utility functions used by various applications
 *
 */

#include "wiced_bt_gatt.h"
#include "wiced_bt_gatt_util.h"
#include "wiced_memory.h"
#include "string.h"

/*
 * Format and send GATT Write Request to set value of a G
 */
wiced_bt_gatt_status_t wiced_bt_util_set_gatt_client_config_descriptor(uint16_t conn_id, uint16_t handle, uint16_t value)
{
    uint8_t  *p_write_req;
    wiced_bt_gatt_write_hdr_t p_write_header = { 0 };
    uint8_t write_value[2];
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;

    if( ( conn_id ) && ( handle ) )
    {
        p_write_req = wiced_bt_get_buffer(sizeof(value));
        if (p_write_req == NULL)
        {
            return WICED_BT_GATT_NO_RESOURCES;
        }
        memset( p_write_req, 0, sizeof(value) );

        p_write_header.handle = handle;
        p_write_header.offset   = 0;
        p_write_header.len = sizeof(value);
        p_write_header.auth_req = GATT_AUTH_REQ_NONE;

        write_value[0] = value & 0xff;
        write_value[1] = (value >> 8) & 0xff;;
        memcpy( p_write_req, write_value, sizeof(value) );

        status = wiced_bt_gatt_client_send_write(conn_id, GATT_REQ_WRITE, &p_write_header, p_write_req, NULL);

        wiced_bt_free_buffer(p_write_req);
    }
    return status;
}

/*
 * Format and send GATT Discover request
 */
wiced_bt_gatt_status_t wiced_bt_util_send_gatt_discover(uint16_t conn_id, wiced_bt_gatt_discovery_type_t type, uint16_t uuid, uint16_t s_handle, uint16_t e_handle)
{
    wiced_bt_gatt_discovery_param_t param;
    wiced_bt_gatt_status_t          status;

    memset(&param, 0, sizeof(param));
    if (uuid != 0)
    {
        param.uuid.len = LEN_UUID_16;
        param.uuid.uu.uuid16 = uuid;
    }
    param.s_handle = s_handle;
    param.e_handle = e_handle;

    status = wiced_bt_gatt_client_send_discover(conn_id, type, &param);
    return status;
}

/*
 * Format and send GATT Read by Handle request
 */
wiced_bt_gatt_status_t wiced_bt_util_send_gatt_read_by_handle(uint16_t conn_id, uint16_t handle)
{
#if 0
    wiced_bt_gatt_read_param_t param;
    wiced_bt_gatt_status_t     status;

    memset(&param, 0, sizeof(param));
    param.by_handle.handle = handle;

    status = wiced_bt_gatt_send_read(conn_id, GATT_READ_BY_HANDLE, &param);
    return status;
#endif
    return WICED_BT_GATT_SUCCESS;
}

/*
 * Format and send GATT Read by Type request
 */
wiced_bt_gatt_status_t wiced_bt_util_send_gatt_read_by_type(uint16_t conn_id, uint16_t s_handle, uint16_t e_handle, uint16_t uuid)
{
#if 0
    wiced_bt_gatt_read_param_t param;
    wiced_bt_gatt_status_t     status;

    memset(&param, 0, sizeof(param));
    param.char_type.s_handle        = s_handle;
    param.char_type.e_handle        = e_handle;
    param.char_type.uuid.len        = 2;
    param.char_type.uuid.uu.uuid16  = uuid;

    status = wiced_bt_gatt_send_read(conn_id, GATT_READ_BY_TYPE, &param);
    return status;
#endif
    return WICED_BT_GATT_SUCCESS;
}

/*
 * wiced_bt_util_uuid_cpy
 * This utility function copies an UUID
 */
int wiced_bt_util_uuid_cpy(wiced_bt_uuid_t *p_dst, wiced_bt_uuid_t *p_src)
{
    if (p_src->len == LEN_UUID_16)
    {
        p_dst->uu.uuid16 =  p_src->uu.uuid16;
    }
    else if (p_src->len == LEN_UUID_32)
    {
        p_dst->uu.uuid32 =  p_src->uu.uuid32;
    }
    else if (p_src->len == LEN_UUID_128)
    {
        memcpy(p_dst->uu.uuid128, p_src->uu.uuid128, LEN_UUID_128);
    }
    else
    {
        return - 1;
    }
    p_dst->len = p_src->len;
    return 0;
}

/*
 * wiced_bt_util_uuid_cmp
 * This utility function Compares two UUIDs.
 * Note: This function can only compare UUIDs of same length
 * Return value: 0 if UUID are equal; -1 if error, 1 otherwise
 */
int wiced_bt_util_uuid_cmp(wiced_bt_uuid_t *p_uuid1, wiced_bt_uuid_t *p_uuid2)
{
    if (p_uuid1 == p_uuid2)
        return 0;

    /* Different UUID length */
    if (p_uuid1->len != p_uuid2->len)
        return -1;

    if (p_uuid1->len == LEN_UUID_16)
    {
        return (p_uuid1->uu.uuid16 != p_uuid2->uu.uuid16);
    }
    if (p_uuid1->len == LEN_UUID_32)
    {
        return (p_uuid1->uu.uuid32 != p_uuid2->uu.uuid32);
    }
    if (p_uuid1->len == LEN_UUID_128)
    {
        return memcmp(p_uuid1->uu.uuid128, p_uuid2->uu.uuid128, LEN_UUID_128);
    }
    return -1;
}
