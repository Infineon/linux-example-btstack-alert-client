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
 ******************************************************************************/
/******************************************************************************
 * File Name: main.c
 *
 * Description: Entry file for alert notification client application.
 *
 * Related Document: See README.md
 *
 ******************************************************************************/
/******************************************************************************
 *                               INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "wiced_memory.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_stack.h"
#include "platform_linux.h"
#include "app_bt_utils/app_bt_utils.h"
#include "utils_arg_parser.h"
#include "bt_app_anc.h"

/******************************************************************************
 *                               MACROS
 ******************************************************************************/
#define MAX_PATH (256U)
#define IP_ADDR_LEN (16U)
#define IP_ADDR "000.000.000.000"
#define INVALID_IP_CMD (15)
#define EXP_IP_RET_VAL (1)
#define INVALID_SCAN   (0)

/******************************************************************************
 *                    STRUCTURES AND ENUMERATIONS
 ******************************************************************************/
/* User Menu: the options from 2. onwards are linked to enum bt_app_anc_cmd
 * If there is any addition to main menu other than 0 and 1 as below, then
 * modify the enum bt_app_anc_cmd start value accordingly.
 */
static const char bt_app_anc_app_menu[] = "\n\
============================================================== \n\
  Alert Notification Client Menu \n\
---------------------------------------------------------------\n\
    0.  Exit \n\
    1.  Start Advertising \n\
    2.  Read Supported New Alert Categories \n\
    3.  Read Supported Unread Alert Categories \n\
    4.  Control Required Alerts \n\
    5.  Enable New Alerts Notification \n\
    6.  Enable Unread Alert Status Notification \n\
    7.  Disable New Alerts Notification \n\
    8.  Disable Unread Alerts Status Notification \n\
 =============================================================\n\
 Choose option (0-8): ";

static const char alert_ids[] = "\
    ----------------------------- \n\
    SIMPLE_ALERT          0 \n\
    EMAIL                 1 \n\
    NEWS                  2 \n\
    CALL                  3 \n\
    MISSED_CALL           4 \n\
    SMS_OR_MMS            5 \n\
    VOICE_MAIL            6 \n\
    SCHEDULE_ALERT        7 \n\
    HIGH_PRIORITY_ALERT   8 \n\
    INSTANT_MESSAGE       9 \n\
    ----------------------------- \n";

static const char control_cmds[] = "\
    -------------------------------------------\n\
    Enable New Alerts Category                0 \n\
    Enable Unread Alert Status Category       1 \n\
    Disable New Alerts Category               2 \n\
    Disable Unread Alert Status Category      3 \n\
    Notify New Alerts immediately             4 \n\
    Notify Unread Alerts immediately          5 \n\
    -------------------------------------------\n";

/******************************************************************************
 *                           GLOBAL VARIABLES
 ******************************************************************************/
wiced_bt_heap_t *p_default_heap = NULL;
uint8_t anc_bd_address[LOCAL_BDA_LEN] = {0x11, 0x12, 0x13, 0x51, 0x52, 0x53};

/******************************************************************************
 *                       FUNCTION DECLARATIONS
 ******************************************************************************/
uint32_t hci_control_proc_rx_cmd(uint8_t *p_buffer, uint32_t length);
void APPLICATION_START(void);

/******************************************************************************
 *                       FUNCTION DEFINITIONS
 ******************************************************************************/

/******************************************************************************
 * Function Name: hci_control_proc_rx_cmd()
 *******************************************************************************
 * Summary:
 *   Function to handle HCI receive called from platform code
 *
 * Parameters:
 *   uint8_t* p_buffer   : rx buffer
 *   uint32_t length     : rx buffer length
 *
 * Return:
 *  status code
 *
 ******************************************************************************/
uint32_t hci_control_proc_rx_cmd(uint8_t *p_buffer, uint32_t length)
{
    return 0;
}

/*******************************************************************************
 * Function Name: APPLICATION_START()
 ********************************************************************************
 * Summary:
 *   BT stack initialization function wrapper called from platform initialization
 *
 * Parameters:
 *   None
 *
 * Return:
 *   None
 *
 *******************************************************************************/
void APPLICATION_START(void)
{
    application_start();
}

/*******************************************************************************
 * Function Name: main()
 ********************************************************************************
 * Summary:
 *   Application entry function
 *
 * Parameters:
 *   int argc            : argument count
 *   char *argv[]        : list of arguments
 *
 * Return:
 *   None
 *
 *******************************************************************************/
int main(int argc, char *argv[])
{
    int ip = 0; /* user input for option */
    wiced_result_t status = WICED_BT_SUCCESS;
    unsigned int alert_category = 0;
    unsigned int cmd_id = 0;
    int filename_len = 0;
    char fw_patch_file[MAX_PATH];
    char hci_port[MAX_PATH];
    char peer_ip_addr[IP_ADDR_LEN] = "000.000.000.000";
    uint32_t hci_baudrate = 0;
    uint32_t patch_baudrate = 0;
    int btspy_inst = 0;
    uint8_t btspy_is_tcp_socket = 0;         /* Throughput calculation thread handler */
    pthread_t throughput_calc_thread_handle; /* Audobaud configuration GPIO bank and pin */
    cybt_controller_autobaud_config_t autobaud;
    int ret = 0;
    memset(fw_patch_file, 0, MAX_PATH);
    memset(hci_port, 0, MAX_PATH);
    if (PARSE_ERROR ==
        arg_parser_get_args(argc, argv, hci_port, anc_bd_address, &hci_baudrate,
                            &btspy_inst, peer_ip_addr, &btspy_is_tcp_socket,
                            fw_patch_file, &patch_baudrate, &autobaud))
    {
        return EXIT_FAILURE;
    }
    filename_len = strlen(argv[0]);
    if (filename_len >= MAX_PATH)
    {
        filename_len = MAX_PATH - 1;
    }

    cy_platform_bluetooth_init(fw_patch_file, hci_port, hci_baudrate,
                               patch_baudrate, &autobaud);

    do
    {
        fprintf(stdout, "%s", bt_app_anc_app_menu);
        fflush(stdin);
        ret = fscanf(stdin, "%d", &ip);
        if (EXP_IP_RET_VAL != ret)
        {
            ip = INVALID_IP_CMD;
            while (getchar() != '\n');
        }

        switch (ip)
        {
        case 0: /* Exiting application */
            status = WICED_BT_SUCCESS;
            break;

        case 1: /*Start Advertising */
            fprintf(stdout, "Starting ANC Advertisement \n");
            bt_app_anc_start_advertisement();
            break;

            /* Fall through as it is the same function called to execute command */
        case USR_ANC_COMMAND_READ_SERVER_SUPPORTED_NEW_ALERTS:
        case USR_ANC_COMMAND_READ_SERVER_SUPPORTED_UNREAD_ALERTS:
        case USR_ANC_COMMAND_ENABLE_NTF_NEW_ALERTS:
        case USR_ANC_COMMAND_ENABLE_NTF_UNREAD_ALERT_STATUS:
        case USR_ANC_COMMAND_DISABLE_NTF_NEW_ALERTS:
        case USR_ANC_COMMAND_DISABLE_NTF_UNREAD_ALERT_STATUS:
            alert_category = 0;
            cmd_id = 0;
            status = bt_app_handle_usr_cmd(ip, cmd_id, alert_category);
            if (status == WICED_BT_GATT_SUCCESS)
            {
                fprintf(stdout, "Command Sent to ANS \n");
            }
            break;

        case USR_ANC_COMMAND_CONTROL_ALERTS:
            fprintf(stdout, "\n    Control Point Commands \n");
            fprintf(stdout, "%s", control_cmds);
            fprintf(stdout, "Enter Control Point Command: ");
            if ( INVALID_SCAN == fscanf(stdin, "%u", &cmd_id) )
            {
                fprintf(stdout, "Unknown input for control point command\n");
                continue;
            }
            fprintf(stdout, "\n    Alert Categories \n");
            fprintf(stdout, "%s", alert_ids);
            fprintf(stdout,
                    "Set Control for a particular Category ID (example- 2 for News): ");
            if ( INVALID_SCAN == fscanf(stdin, "%u", &alert_category) )
            {
                fprintf(stdout, "Unknown input for alert categories\n");
                continue;
            }
            status =
                bt_app_handle_usr_cmd(ip, (uint8_t)cmd_id,
                                      (uint8_t)alert_category);
            if (status == WICED_BT_GATT_SUCCESS)
            {
                fprintf(stdout, "Command Sent to ANS \n");
            }
            break;

        default:
            fprintf(stdout,
                    "Unknown ANC Command. Choose option from the Menu \n");
            break;
        }
        if (status != WICED_BT_SUCCESS)
        {
            fprintf(stderr, "\n Command Failed. Status: 0x%x \n", status);
            status = WICED_BT_SUCCESS;
        }
        fflush(stdin);
    } while (ip != 0);

    fprintf(stdout, "Exiting...\n");
    wiced_bt_delete_heap(p_default_heap);
    wiced_bt_stack_deinit();

    return EXIT_SUCCESS;
}
