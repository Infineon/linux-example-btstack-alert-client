/******************************************************************************
 * (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
 ******************************************************************************
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
 *****************************************************************************/
/******************************************************************************
* File Name: anc_gap.c
*
* Description:
* BLE device's GAP configuration.
*
******************************************************************************/

#include "anc_gap.h"

/* Device address */
const wiced_bt_device_address_t cy_bt_device_address = {0x43, 0x01, 0x2C, 0x00, 0x1F, 0xAB};

const uint8_t cy_bt_adv_packet_elem_0[1] = { 0x06 };
const uint8_t cy_bt_adv_packet_elem_1[3] = { 0x41, 0x4E, 0x43 };
const uint8_t cy_bt_adv_packet_elem_2[2] = { 0x00, 0x02 };
const uint8_t cy_bt_adv_packet_elem_3[1] = { 0x00 };
wiced_bt_ble_advert_elem_t cy_bt_adv_packet_data[] = 
{
    /* Flags */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_FLAG, 
        .len = 1, 
        .p_data = (uint8_t*)cy_bt_adv_packet_elem_0, 
    },
    /* Complete local name */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, 
        .len = 3, 
        .p_data = (uint8_t*)cy_bt_adv_packet_elem_1, 
    },
    /* Appearance */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE, 
        .len = 2, 
        .p_data = (uint8_t*)cy_bt_adv_packet_elem_2, 
    },
    /* LE Role */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_LE_ROLE, 
        .len = 1, 
        .p_data = (uint8_t*)cy_bt_adv_packet_elem_3, 
    },
};
const uint8_t cy_bt_scan_resp_packet_elem_0[3] = { 0x41, 0x4E, 0x43 };
const uint8_t cy_bt_scan_resp_packet_elem_1[7] = { 0x00, 0x43, 0x01, 0x2C, 0x00, 0x1F, 0xAC };
wiced_bt_ble_advert_elem_t cy_bt_scan_resp_packet_data[] = 
{
    /* Complete local name */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, 
        .len = 3, 
        .p_data = (uint8_t*)cy_bt_scan_resp_packet_elem_0, 
    },
    /* LE Bluetooth Device Address */
    {
        .advert_type = BTM_BLE_ADVERT_TYPE_LE_BD_ADDR, 
        .len = 7, 
        .p_data = (uint8_t*)cy_bt_scan_resp_packet_elem_1, 
    },
};
