/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* C file for generic analog sensor
 * contains function similar to what a regular UPM driver would have
 */


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "sol-flow/genericanalog.h"

#include "sol-aio.h"
#include "sol-flow-internal.h"
#include "sol-flow.h"
#include "sol-mainloop.h"
#include "sol-util-internal.h"
#include "sol-types.h"

/* Struct for the generic analog sensor
 *
 */
struct _upm_generic_analog {
	struct sol_flow_node *node;
	struct sol_timeout *timer;
	struct sol_aio *aio;
	struct sol_aio_pending *pending;
	int	pin;
	int	mask;
	int	ret_data;
};

static int 
genericanalog_open(struct sol_flow_node *node, void *data, const struct sol_flow_node_options *options){
	// the device number
	int device, pin;
	struct _upm_generic_analog *mdata = data;

	// some soletta stuff
	// need to change the sensor name involved in the next couple of lines
	const struct sol_flow_node_type_genericanalog_options *opts = (const struct sol_flow_node_type_genericanalog_options *)options;
    	SOL_FLOW_NODE_OPTIONS_SUB_API_CHECK(opts, SOL_FLOW_NODE_TYPE_GENERICANALOG_OPTIONS_API_VERSION, -EINVAL);

	// again copied over from a soletta example
	// it is a check but doesn't really need to be done actually
	if (!opts->pin) {
		SOL_WRN("aio: Option 'pin' cannot be neither 'null' nor empty.");
		return -EINVAL;
    	}
	// since only integer options have been defined, no need to go for a raw option check
	if (sscanf(opts->pin, "%d %d", &device, &pin) == 2) {
	//pin = opts->pin;
		mdata->aio = sol_aio_open(device, pin, 12);
	}
	// some sort of a print statement
	// but basically it prints the error msg in case if the returned value is null
	SOL_NULL_CHECK_MSG(mdata->aio, -EINVAL, "aio (%d): Couldn't be open. Maybe you used an invalid 'pin'=%d?", pin, pin);
	mdata->pin = pin;
	mdata->mask = 12;
	//mdata->timer = sol_timeout_add(opts->poll_timeout, _on_reader_timeout, mdata);
	return 0;
}

/* generic close function
 *
 */
static void 
genericanalog_close(struct sol_flow_node *node, void *data){
	// copy pasted from soletta aio driver
	// dont forget to change the struct name
	struct _upm_generic_analog *mdata = data;

    	SOL_NULL_CHECK(mdata);
	// free up the pin memo
    	//free(mdata->pin);

	// check and close aio
    	if (mdata->aio)
        	sol_aio_close(mdata->aio);
	//check and close the timer
    	if (mdata->timer)
        	sol_timeout_del(mdata->timer);
}

// Callback function which holds the AIO data
static void 
read_cb(void *cb_data, struct sol_aio *aio, int32_t ret){
	struct _upm_generic_analog *mdata = cb_data;
	struct sol_irange val = {
		.min = 0,
		.max = 4094,
		.step = ret
    	};
	mdata->ret_data = ret;
	sol_flow_send_irange_packet(mdata->node, SOL_FLOW_NODE_TYPE_GENERICANALOG__OUT__OUT, &val);
} 


/* Generic analog read value function
 * this function will call the get value function from the analog IO
 * Upon completion it calls a callback which contains the returned AIO value
 */
static int 
genericanalog_tick(struct sol_flow_node *node, void *data, uint16_t port, uint16_t conn_id, const struct sol_flow_packet *packet){
	// 
	struct _upm_generic_analog *mdata = data;
	SOL_NULL_CHECK(data, true);
	if (sol_aio_busy(mdata->aio))
        	return true;

	mdata->pending = sol_aio_get_value(mdata->aio, read_cb, mdata);

	if (!mdata->pending) {
        	sol_flow_send_error_packet(mdata->node, EINVAL, "AIO (%d): Failed to issue read operation.", mdata->pin);
        	return false;
   	}
	return 0;
}
#include "genericanalog-gen.c"
