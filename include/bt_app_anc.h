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
* File Name: bt_app_anc.h
*
* Description: Header file for bt_app_anc.c
*
* Related Document: See README.md
*******************************************************************************/

#ifndef _BT_APP_ANC_H_
#define _BT_APP_ANC_H_

/*******************************************************************************
*                                   INCLUDES
*******************************************************************************/
#include <stdint.h>
#include "wiced_bt_trace.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_gatt.h"
#include "platform_linux.h"
#include "COMPONENT_anc/wiced_bt_anp.h"
#include "COMPONENT_anc/wiced_bt_anc.h"

/*******************************************************************************
*                                   MACROS
*******************************************************************************/
#define LEN_1_BYTE                     (1U)
#define LEN_2_BYTE                     (2U)
#define LOCAL_BDA_LEN                  (6U)

/* User options on the Menu refers this enum
 * The beginning value shall be changed based on the other options in the
 * main menu. Now there are 2 options in the User menu
 * 0 - exit
 * 1 - Start advertising
 * Hence the value here begins at 2
 */
typedef enum
{
     USR_ANC_COMMAND_READ_SERVER_SUPPORTED_NEW_ALERTS = 2,
     USR_ANC_COMMAND_READ_SERVER_SUPPORTED_UNREAD_ALERTS,
     USR_ANC_COMMAND_CONTROL_ALERTS,
     USR_ANC_COMMAND_ENABLE_NTF_NEW_ALERTS,
     USR_ANC_COMMAND_ENABLE_NTF_UNREAD_ALERT_STATUS,
     USR_ANC_COMMAND_DISABLE_NTF_NEW_ALERTS,
     USR_ANC_COMMAND_DISABLE_NTF_UNREAD_ALERT_STATUS,
}bt_app_anc_cmd;

/******************************************************************************
 *                                EXTERNS
 *****************************************************************************/
extern uint8_t anc_bd_address[];

/******************************************************************************
*                           FUNCTION PROTOTYPES
******************************************************************************/
void application_start( void );
void bt_app_anc_start_advertisement();
wiced_bt_gatt_status_t bt_app_handle_usr_cmd(uint8_t cmd, uint8_t cmd_id, uint8_t alert_categ);
#endif /* _BT_APP_ANC_H_ */
