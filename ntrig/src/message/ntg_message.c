/**
 * @file ntg_message.c
 * @brief
 * @details
 * @author tzh
 * @date Jan 27, 2016
 * @version V0.1
 * @copyright tzh
 */

#include "ntg_message.h"


int msg_size[NTG_MESSAGE_TYPES] = {
		0,
		sizeof(ntg_channel_message_t),
		sizeof(ntg_simulation_message_t),
		sizeof(ntg_control_message_t),
		sizeof(ntg_result_message_t),
		sizeof(ntg_request_message_t),
		sizeof(ntg_feedback_message_t),
		sizeof(ntg_record_message_t)
};
