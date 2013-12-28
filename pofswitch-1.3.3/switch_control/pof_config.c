/**
 * Copyright (c) 2012, 2013, Huawei Technologies Co., Ltd.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../include/pof_common.h"
#include "../include/pof_type.h"
#include "../include/pof_global.h"
#include "../include/pof_conn.h"
#include "../include/pof_log_print.h"
#include "../include/pof_local_resource.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define POF_STRING_MAX_LEN (100)

char pofc_cmd_input_file_name[POF_STRING_MAX_LEN] = "\0";
uint32_t pofc_cmd_set_ip_ret = FALSE;
uint32_t pofc_cmd_set_port_ret = FALSE;
uint32_t pofc_cmd_set_config_file = FALSE;
uint32_t pofc_cmd_auto_clear = TRUE;

#ifdef POF_CONN_CONFIG_COMMAND
static void pof_start_command_help();

#define COMMAND_STR_LEN (30)
#define HELP_STR_LEN    (50)
typedef struct pof_start_commands {
	char cmd_str[COMMAND_STR_LEN];
	char help_str[HELP_STR_LEN];
} pof_start_commands;

#define COMMAND_NUM (8)
const pof_start_commands cmds[COMMAND_NUM] = {
	{"-i", "IP address of the Controller."},
	{"-p", "Connection port number. Default is 6633."},
	{"-f", "Set config file."},
	{"-m, --man-clear", "Don't auto clear the resource when disconnect."},
	{"-l, --log-file", "Create log file:"},
	{"              ", "/usr/local/var/log/pofswitch.log."},
	{"-h, --help", "Print help message."},
	{"-v, --version", "Print the version number of POFSwitch."},
};

/***********************************************************************
 * Initialize the config by command.
 * Form:     static uint32_t pof_set_init_config_by_command(int argc, char *argv[])
 * Input:    command arg numbers, command args.
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by user's 
 *           command arguments. 
 *           -i: set the IP address of the Controller.
 *           -p: set the port to connect to the Controller.
 ***********************************************************************/
static uint32_t pof_set_init_config_by_command(int argc, char *argv[]){
	uint32_t ret = POF_OK;
	char optstring[] = "i:p:f:mlhv";
 	struct option long_options[] = {
		{"man-clear", 0, NULL, 'm'},
		{"log-file", 0, NULL, 'l'},
		{"help", 0, NULL, 'h'},
		{"version", 0, NULL, 'v'},
		{NULL,0,NULL,0}
	};
	int ch;

	while((ch=getopt_long(argc, argv, optstring, long_options, NULL)) != -1){
		switch(ch){
			case '?':
				exit(0);
				break;
			case 'i':
                pofsc_set_controller_ip(optarg);
				pofc_cmd_set_ip_ret = TRUE;
				break;
			case 'p':
                pofsc_set_controller_port(atoi(optarg));
				pofc_cmd_set_port_ret = TRUE;
				break;
			case 'f':
				strncpy(pofc_cmd_input_file_name, optarg, POF_STRING_MAX_LEN-1);
				pofc_cmd_set_config_file = TRUE;
				break;
			case 'm':
				pofc_cmd_auto_clear = FALSE;
				break;
			case 'l':
				if(POF_OK != pofsc_check_root()){
					exit(0);
				}
				pof_open_log_file(NULL);
				
				break;
			case 'h':
				pof_start_command_help();
				exit(0);
				break;
			case 'v':
				printf("Version: %s\n", POFSWITCH_VERSION);
				exit(0);
				break;
			default:
				break;
		}
	}

	POF_DEBUG_CPRINT_FL(1,GREEN,"Command config");
}

uint32_t pof_auto_clear(){
	return pofc_cmd_auto_clear;
}

static void pof_start_command_help(){
	uint32_t i;

	printf("Usage: pofswitch [options] [target] ...\n");
	printf("Options:\n");

	for(i=0; i<COMMAND_NUM; i++){
		printf("  %-30s%-50s\n", cmds[i].cmd_str, cmds[i].help_str);
	}

	printf("\nReport bugs to <yujingzhou@huawei.com>\n");

	return;
}

#endif // POF_CONN_CONFIG_COMMAND

#ifdef POF_CONN_CONFIG_FILE

enum pof_init_config_type{
	POFICT_CONTROLLER_IP	= 0,
	POFICT_CONNECTION_PORT  = 1,
	POFICT_MM_TABLE_NUMBER = 2,
	POFICT_LPM_TABLE_NUMBER = 3,
	POFICT_EM_TABLE_NUMBER  = 4,
	POFICT_DT_TABLE_NUMBER  = 5,
	POFICT_FLOW_TABLE_SIZE  = 6,
	POFICT_FLOW_TABLE_KEY_LENGTH = 7,
	POFICT_METER_NUMBER     = 8,
	POFICT_COUNTER_NUMBER   = 9,
	POFICT_GROUP_NUMBER     = 10,
	POFICT_DEVICE_PORT_NUMBER_MAX = 11,

	POFICT_CONFIG_TYPE_MAX,
};

char *config_type_str[POFICT_CONFIG_TYPE_MAX] = {
	"Controller_IP", "Connection_port", 
	"MM_table_number", "LPM_table_number", "EM_table_number", "DT_table_number",
	"Flow_table_size", "Flow_table_key_length", 
	"Meter_number", "Counter_number", "Group_number", 
	"Device_port_number_max"
};

static uint8_t pofsic_get_config_type(char *str){
	uint8_t i;

	for(i=0; i<POFICT_CONFIG_TYPE_MAX; i++){
		if(strcmp(str, config_type_str[i]) == 0){
			return i;
		}
	}

	return 0xff;
}

static uint32_t pofsic_get_config_data(FILE *fp, uint32_t *ret_p){
	char str[POF_STRING_MAX_LEN];

	if(fscanf(fp, "%s", str) != 1){
		*ret_p = POF_ERROR;
		return POF_ERROR;
	}else{
		return atoi(str);
	}
}

/***********************************************************************
 * Initialize the config by file.
 * Form:     static uint32_t pof_set_init_config_by_file()
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by "config"
 *           file. The config's content:
 *			 "Controller_IP", "Connection_port", 
 *			 "MM_table_number", "LPM_table_number", "EM_table_number", "DT_table_number",
 *			 "Flow_table_size", "Flow_table_key_length", 
 *			 "Meter_number", "Counter_number", "Group_number", 
 *			 "Device_port_number_max"
 ***********************************************************************/
static uint32_t pof_set_init_config_by_file(){
	uint32_t ret = POF_OK, data = 0;
	char     filename_relative[] = "./pofswitch_config.conf";
	char     filename_absolute[] = "/etc/pofswitch/pofswitch_config.conf";
	char     filename_final[POF_STRING_MAX_LEN] = "\0";
	char     str[POF_STRING_MAX_LEN] = "\0";
	char     ip_str[POF_STRING_MAX_LEN] = "\0";
	FILE     *fp = NULL;
	uint8_t  config_type = 0;

	if((fp = fopen(pofc_cmd_input_file_name, "r")) == NULL){
		if(pofc_cmd_set_config_file != FALSE){
			POF_DEBUG_CPRINT_FL(1,RED,"The file %s is not exist.", pofc_cmd_input_file_name);
		}
		if((fp = fopen(filename_relative, "r")) == NULL){
			POF_DEBUG_CPRINT_FL(1,RED,"The file %s is not exist.", filename_relative);
			if((fp = fopen(filename_absolute, "r")) == NULL){
				POF_DEBUG_CPRINT_FL(1,RED,"The file %s is not exist.", filename_absolute);
				return POF_ERROR;
			}else{
				POF_DEBUG_CPRINT_FL(1,GREEN,"The config file %s have been loaded.", filename_absolute);
			}
		}else{
			POF_DEBUG_CPRINT_FL(1,GREEN,"The config file %s have been loaded.", filename_relative);
		}
	}else{
		POF_DEBUG_CPRINT_FL(1,GREEN,"The config file %s have been loaded.", pofc_cmd_input_file_name);
	}
	
	while(fscanf(fp, "%s", str) == 1){
		config_type = pofsic_get_config_type(str);
		if(config_type == POFICT_CONTROLLER_IP){
			if(fscanf(fp, "%s", ip_str) != 1){
				ret = POF_ERROR;
			}else{
				if(pofc_cmd_set_ip_ret == FALSE){
					pofsc_set_controller_ip(ip_str);
				}
			}
		}else{
			data = pofsic_get_config_data(fp, &ret);
			switch(config_type){
				case POFICT_CONNECTION_PORT:
					if(pofc_cmd_set_port_ret == FALSE){
	                    pofsc_set_controller_port(data);
					}
					break;
				case POFICT_MM_TABLE_NUMBER:
					poflr_set_MM_table_number(data);
					break;
				case POFICT_LPM_TABLE_NUMBER:
					poflr_set_LPM_table_number(data);
					break;
				case POFICT_EM_TABLE_NUMBER:
					poflr_set_EM_table_number(data);
					break;
				case POFICT_DT_TABLE_NUMBER:
					poflr_set_DT_table_number(data);
					break;
				case POFICT_FLOW_TABLE_SIZE:
					poflr_set_flow_table_size(data);
					break;
				case POFICT_FLOW_TABLE_KEY_LENGTH:
					poflr_set_key_len(data);
					break;
				case POFICT_METER_NUMBER:
					poflr_set_meter_number(data);
					break;
				case POFICT_COUNTER_NUMBER:
					poflr_set_counter_number(data);
					break;
				case POFICT_GROUP_NUMBER:
					poflr_set_group_number(data);
					break;
				case POFICT_DEVICE_PORT_NUMBER_MAX:
					poflr_set_port_number_max(data);
					break;
				default:
					ret = POF_ERROR;
					break;
			}
		}

		if(ret != POF_OK){
			POF_ERROR_CPRINT_FL(1,RED,"Can't load config file: Wrong config format.");
			exit(0);
		}
	}

	fclose(fp);
	POF_DEBUG_CPRINT_FL(1,GREEN,"File config");
	return ret;
}
#endif // POF_CONN_CONFIG_FILE

/***********************************************************************
 * Initialize the config by file or command.
 * Form:     uint32_t pof_set_init_config(int argc, char *argv[])
 * Input:    number of arg, args
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by "config"
 *           file or command arguments.
 ***********************************************************************/
uint32_t pof_set_init_config(int argc, char *argv[]){
	uint32_t ret;
#ifdef POF_CONN_CONFIG_COMMAND
	ret = pof_set_init_config_by_command(argc, argv);
#endif
#ifdef POF_CONN_CONFIG_FILE
	ret = pof_set_init_config_by_file();
#endif // POF_CONN_CONFIG_FILE
	return POF_OK;
}
