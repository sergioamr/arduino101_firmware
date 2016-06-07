/* mail.c - test mailbox APIs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module tests the following mailbox APIs:

   task_mbox_put, task_mbox_put_wait, task_mbox_put_wait_timeout
   task_mbox_get, task_mbox_get_wait, task_mbox_get_wait_timeout

   task_mbox_data_get
   task_mbox_data_get_async_block,  task_mbox_data_get_async_block_wait,
   task_mbox_data_get_async_block_wait_timeout

The module does NOT test the following mailbox APIs:

   task_mbox_put_async

Also, not all capabilities of all of the tested APIs are exercised. Things that
are not (yet) tested include:

- Having multiple tasks sending simultaneously to a mailbox,
  to ensure a mailbox can contain more than one message.
- Having multiple tasks waiting simultaneously on a mailbox,
  to ensure a mailbox can have more than one waiting task.
- Having messages of differing priorities residing in a mailbox,
  to ensure higher priority messages get preference.
- Having receiving tasks of differing priorities waiting on a mailbox.
  to ensure higher priority tasks get preference.
 */

#include <microkernel.h>

#include <tc_util.h>

#define MSGSIZE		16    /* Standard message data size */
#define XFER_PRIO	5     /* standard message transfer priority */
#define MSG_INFO1	1234  /* Message info test value */
#define MSG_INFO2	666   /* Message info test value */

static char myData1[MSGSIZE] = "This is myData1";
static char myData2[MSGSIZE] = "This is myData2";
static char myData3[MSGSIZE] = "This is myData3";
static char myData4[MSGSIZE] = "This is myData4";

extern ktask_t msgSenderTask;
extern ktask_t msgRcvrTask;

extern ksem_t semSync1;
extern ksem_t semSync2;

extern kmbox_t myMbox;
extern kmbox_t noRcvrMbox;

extern kmemory_pool_t testPool;
extern kmemory_pool_t smallBlkszPool;

/**
 *
 * @brief Sets various fields in the message for the sender
 *
 * Sets the following fields in the message:
 *   rx_task to receiverTask - destination for the message
 *   mailbox to inMbox
 *
 * @param inMsg          The message being received.
 * @param inMbox         Mail box to receive the message.
 * @param receiverTask   Destination for the message.
 * @param dataArea       Pointer to (optional) buffer to send.
 * @param dataSize       Size of (optional) buffer to send.
 * @param info           Additional (optional) info to send.
 *
 * @return  N/A
 */

static void setMsg_Sender(struct k_msg *inMsg, kmbox_t inMbox, ktask_t receiverTask,
						  void *dataArea, uint32_t dataSize, uint32_t info)
{
	inMsg->rx_task = receiverTask;
	inMsg->mailbox = inMbox;
	inMsg->tx_data = dataArea;
	inMsg->size = SIZEOFUNIT_TO_OCTET(dataSize);
	inMsg->info = info;
}

/**
 *
 * @brief Sets various fields in the message for the receiver
 *
 * Sets the following fields in the message:
 *   rx_data to NULL       - to allow message transfer to occur
 *   size    to MSGSIZE
 *   tx_task to senderTask - receiver tries to get message from this source
 *   mailbox to inMbox
 *
 * @param inMsg          Message descriptor.
 * @param inMbox         Mail box to receive from.
 * @param senderTask     Sending task to receive from.
 * @param inBuffer       Incoming data area
 * @param inBufferSize   Size of incoming data area.
 *
 * @return  N/A
 */

static void setMsg_Receiver(struct k_msg *inMsg, kmbox_t inMbox, ktask_t senderTask,
							void *inBuffer, uint32_t inBufferSize)
{
	inMsg->mailbox = inMbox;
	inMsg->tx_task = senderTask;
	inMsg->rx_data = inBuffer;
	inMsg->size = inBufferSize;
	if (inBufferSize != 0 && inBuffer != NULL) {
		memset(inBuffer, 0, inBufferSize);
	}
}

/**
 *
 * @brief Sets rx_data field in msg and clears buffer
 *
 * @param inMsg          The message being received.
 * @param inBuffer       Incoming data area.
 * @param inBufferSize   Size of incoming data area.
 *
 * @return  N/A
 */

static void setMsg_RecvBuf(struct k_msg *inMsg, char *inBuffer, uint32_t inBufferSize)
{
	inMsg->rx_data = inBuffer;
	inMsg->size = inBufferSize;
	if (inBufferSize != 0 && inBuffer != NULL) {
		memset(inBuffer, 0, inBufferSize);
	}
}

/**
 *
 * @brief Task that tests sending of mailbox messages
 *
 * This routine exercises the task_mbox_put[_wait[_timeout]] APIs.
 *
 * @return TC_PASS or TC_FAIL
 */

int MsgSenderTask(void)
{
	int retValue;           /* task_mbox_xxx interface return value */
	struct k_msg  MSTmsg;   /* Message sender task msg */

	/* Send message (no wait) to a mailbox with no receiver */

	setMsg_Sender(&MSTmsg, noRcvrMbox, msgRcvrTask, myData1, MSGSIZE, 0);
	retValue = task_mbox_put(noRcvrMbox, XFER_PRIO, &MSTmsg);
	if (RC_FAIL != retValue) {
		TC_ERROR("task_mbox_put to non-waiting task returned %d\n", retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put to non-waiting task is OK\n", __func__);

	/* Send message (with timeout) to a mailbox with no receiver */

	setMsg_Sender(&MSTmsg, noRcvrMbox, msgRcvrTask, myData1, MSGSIZE, 0);
	retValue = task_mbox_put_wait_timeout(noRcvrMbox, XFER_PRIO, &MSTmsg, 2);
	if (RC_TIME != retValue) {
		TC_ERROR("task_mbox_put_wait_timeout to non-waiting task returned %d\n", retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait_timeout to non-waiting task is OK\n", __func__);

	/* Wait for Receiver Task to finish using myMbox */

	(void) task_sem_take_wait(semSync1);

	/* Send message (no wait) to specified task that is waiting for it */

	setMsg_Sender(&MSTmsg, myMbox, msgRcvrTask, myData1, MSGSIZE, 0);
	/*
	 * Transmit more data then receiver can actually handle
	 * to ensure that "size" field gets updated properly during send
	 */
	MSTmsg.size += 10;
	retValue = task_mbox_put(myMbox, XFER_PRIO, &MSTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put to specified waiting task returned %d\n",
			retValue);
		return TC_FAIL;
	}
	if (MSTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_put to specified waiting task got wrong size (%d)\n",
			MSTmsg.size);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put to specified waiting task is OK\n", __func__);

	/* Wait for Receiver Task to start sleeping */

	(void) task_sem_take_wait(semSync2);

	/* Send message to any task that is not yet waiting for it */

	setMsg_Sender(&MSTmsg, myMbox, ANYTASK, myData2, MSGSIZE, MSG_INFO1);
	retValue = task_mbox_put_wait_timeout(myMbox, XFER_PRIO, &MSTmsg, 5);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait_timeout to anonymous non-waiting task returned %d\n",
			retValue);
		return TC_FAIL;
	}
	if (MSTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_put_wait_timeout to anonymous non-waiting task "
			"got wrong size (%d)\n", MSTmsg.size);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait_timeout to anonymous non-waiting task is OK\n",
		__func__);

	/* Send empty message to specified task */

	setMsg_Sender(&MSTmsg, myMbox, msgRcvrTask, NULL, 0, MSG_INFO2);
	retValue = task_mbox_put_wait(myMbox, XFER_PRIO, &MSTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait of empty message returned %d\n", retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait of empty message is OK\n", __func__);


	/* Sync with Receiver Task, since we're about to use a timeout */

	task_sem_take_wait(semSync1);

	/* Send message used in 2 part receive test */

	setMsg_Sender(&MSTmsg, myMbox, ANYTASK, myData3, MSGSIZE, MSG_INFO1);
	/*
	 * Transmit more data then receiver can actually handle
	 * to ensure that "size" field gets updated properly during send
	 */
	MSTmsg.size += 10;
	retValue = task_mbox_put_wait_timeout(myMbox, XFER_PRIO, &MSTmsg, 5);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait_timeout for 2 part receive test returned %d\n",
			retValue);
		return TC_FAIL;
	}
	if (MSTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_put_wait_timeout for 2 part receive test got wrong size (%d)\n",
			MSTmsg.size);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait_timeout for 2 part receive test is OK\n", __func__);

	/* Sync with Receiver Task, since he's about to use a timeout */

	task_sem_give(semSync2);

	/* Send message used in cancelled receive test */

	setMsg_Sender(&MSTmsg, myMbox, msgRcvrTask, myData4, MSGSIZE, MSG_INFO2);
	retValue = task_mbox_put_wait(myMbox, XFER_PRIO, &MSTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait for cancelled receive test returned %d\n",
			retValue);
		return TC_FAIL;
	}
	if (MSTmsg.size != MSGSIZE) {
		/* kernel bug: should really set size to 0! */
		TC_ERROR("task_mbox_put_wait for cancelled receive test got wrong size (%d)\n",
			MSTmsg.size);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait for cancelled receive test is OK\n", __func__);

	/* Send message used in block-based receive test */

	setMsg_Sender(&MSTmsg, myMbox, msgRcvrTask, myData1, MSGSIZE, MSG_INFO2);
	retValue = task_mbox_put_wait(myMbox, XFER_PRIO, &MSTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait for block-based receive test returned %d\n",
			retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait for block-based receive test is OK\n",
		__func__);

	/* Send message used in block-exhaustion receive test */

	setMsg_Sender(&MSTmsg, myMbox, msgRcvrTask, myData2, MSGSIZE, MSG_INFO2);
	retValue = task_mbox_put_wait(myMbox, XFER_PRIO, &MSTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait for block-exhaustion receive test returned %d\n",
			retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait for block-exhaustion receive test is OK\n",
		__func__);

	/* Sync with Receiver Task, since we're about to use a timeout */

	task_sem_take_wait(semSync1);

	/* Send message used in long-duration receive test */

	setMsg_Sender(&MSTmsg, myMbox, ANYTASK, myData3, MSGSIZE, MSG_INFO1);
	retValue = task_mbox_put_wait_timeout(myMbox, XFER_PRIO, &MSTmsg, 2);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_put_wait_timeout for long-duration receive test returned %d\n",
			retValue);
		return TC_FAIL;
	}
	if (MSTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_put_wait_timeout for long-duration receive test got wrong size "
			"(%d)\n", MSTmsg.size);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_put_wait_timeout for long-duration receive test is OK\n",
		__func__);

	return TC_PASS;
}

/**
 *
 * @brief Task that tests receiving of mailbox messages
 *
 * This routine exercises the task_mbox_get[_wait[_timeout]] and task_mbox_data_get[xxx] APIs.
 *
 * @return TC_PASS or TC_FAIL
 */

int MsgRcvrTask(void)
{
	int retValue;           /* task_mbox_xxx interface return value */
	struct k_msg MRTmsg = {0, };   /* Message receiver task msg */
	char rxBuffer[MSGSIZE*2];  /* Buffer to place message data in (has extra
				  space at end for overrun testing) */
	struct k_block  MRTblock;      /* Message receiver task memory block */
	struct k_block  MRTblockAlt;   /* Message receiver task memory block (alternate) */

	/* Receive message (no wait) from an empty mailbox */

	setMsg_Receiver(&MRTmsg, myMbox, ANYTASK, rxBuffer, MSGSIZE);
	retValue = task_mbox_get(myMbox, &MRTmsg);
	if (RC_FAIL != retValue) {
		TC_ERROR("task_mbox_get when no message returned %d\n", retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get when no message is OK\n", __func__);

	/* Receive message (with timeout) from an empty mailbox */

	setMsg_Receiver(&MRTmsg, myMbox, ANYTASK, rxBuffer, MSGSIZE);
	retValue = task_mbox_get_wait_timeout(myMbox, &MRTmsg, 2);
	if (RC_TIME != retValue) {
		TC_ERROR("task_mbox_get_wait_timeout when no message returned %d\n", retValue);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait_timeout when no message is OK\n", __func__);

	/* Allow Sender Task to proceed once we start our receive */

	task_sem_give(semSync1);

	/* Receive message (no timeout) from specified task */

	setMsg_Receiver(&MRTmsg, myMbox, msgSenderTask, rxBuffer, MSGSIZE);
	retValue = task_mbox_get_wait(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait from specified task got wrong return code (%d)\n",
			retValue);
		return TC_FAIL;
	}
	if (strcmp(MRTmsg.rx_data, myData1) != 0) {
		TC_ERROR("task_mbox_get_wait from specified task got wrong data (%s)\n",
			MRTmsg.rx_data);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait from specified task is OK\n", __func__);

	/* Allow Sender Task to proceed once we go to sleep for a while */

	task_sem_give(semSync2);
	task_sleep(2);

	/* Receive message (no wait) from anonymous task */

	setMsg_Receiver(&MRTmsg, myMbox, ANYTASK, rxBuffer, MSGSIZE);
	/*
	 * Ask for more data then is actually being sent
	 * to ensure that "size" field gets updated properly during receive
	 */
	MRTmsg.size += MSGSIZE;

	retValue = task_mbox_get(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get from anonymous task got wrong return code (%d)\n",
			retValue);
		return TC_FAIL;
	}
	if (MRTmsg.info != MSG_INFO1) {
		TC_ERROR("task_mbox_get from anonymous task got wrong info (%d)\n",
			MRTmsg.info);
		return TC_FAIL;
	}
	if (MRTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_get from anonymous task got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}
	if (strcmp(MRTmsg.rx_data, myData2) != 0) {
		TC_ERROR("task_mbox_get from anonymous task got wrong data (%s)\n",
			MRTmsg.rx_data);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get from anonymous task is OK\n", __func__);

	/* Receive empty message from anonymous task */

	setMsg_Receiver(&MRTmsg, myMbox, ANYTASK, rxBuffer, MSGSIZE);
	retValue = task_mbox_get_wait(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait of empty message got wrong return code (%d)\n",
			retValue);
		return TC_FAIL;
	}
	if (MRTmsg.info != MSG_INFO2) {
		TC_ERROR("task_mbox_get_wait of empty message got wrong info (%d)\n",
			MRTmsg.info);
		return TC_FAIL;
	}
	if (MRTmsg.size != 0) {
		TC_ERROR("task_mbox_get_wait of empty message got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait of empty message is OK\n", __func__);


	/* Sync with Sender Task, since he's about to use a timeout */

	(void) task_sem_give(semSync1);

	/* Receive message header for 2 part receive test */

	setMsg_Receiver(&MRTmsg, myMbox, msgSenderTask, NULL, MSGSIZE);
	retValue = task_mbox_get_wait(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait of message header #3 returned %d\n", retValue);
		return TC_FAIL;
	}
	if (MRTmsg.info != MSG_INFO1) {
		TC_ERROR("task_mbox_get_wait of message header #3 got wrong info (%d)\n",
			MRTmsg.info);
		return TC_FAIL;
	}
	if (MRTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_get_wait of message header #3 got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}

	/* Now grab the message data */

	setMsg_RecvBuf(&MRTmsg, rxBuffer, MSGSIZE);
	task_mbox_data_get(&MRTmsg);
	if (strcmp(MRTmsg.rx_data, myData3) != 0) {
		TC_ERROR("task_mbox_data_get got wrong data #3 (%s)\n", MRTmsg.rx_data);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait of message header #3 is OK\n", __func__);
	TC_PRINT("%s: task_mbox_data_get of message data #3 is OK\n", __func__);

	/* Sync with Sender Task, since we're about to use a timeout */

	(void) task_sem_take_wait(semSync2);

	/* Receive message header for cancelled receive test */

	setMsg_Receiver(&MRTmsg, myMbox, msgSenderTask, NULL, MSGSIZE);
	retValue = task_mbox_get_wait_timeout(myMbox, &MRTmsg, 5);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait_timeout of message header #4 returned %d\n", retValue);
		return TC_FAIL;
	}
	if (MRTmsg.info != MSG_INFO2) {
		TC_ERROR("task_mbox_get_wait_timeout of message header #4 got wrong info (%d)\n",
			MRTmsg.info);
		return TC_FAIL;
	}
	if (MRTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_get_wait_timeout of message header #4 got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}

	/* Cancel receiving of message data */

	setMsg_RecvBuf(&MRTmsg, rxBuffer, 0);
	task_mbox_data_get(&MRTmsg);

	TC_PRINT("%s: task_mbox_get_wait_timeout of message header #4 is OK\n", __func__);
	TC_PRINT("%s: task_mbox_data_get cancellation of message #4 is OK\n", __func__);

	/* Receive message header for block-based receive test */

	setMsg_Receiver(&MRTmsg, myMbox, ANYTASK, NULL, MSGSIZE);
	retValue = task_mbox_get_wait(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait of message header #1 returned %d\n", retValue);
		return TC_FAIL;
	}
	if (MRTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_get_wait of message header #1 got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}

	/* Try to grab message data using a block that's too small */

	retValue = task_mbox_data_get_async_block(&MRTmsg, &MRTblock, smallBlkszPool);
	if (RC_FAIL != retValue) {
		TC_ERROR("task_mbox_data_get_async_block that should have failed returned %d\n",
			retValue);
		return TC_FAIL;
	}

	/* Now grab message data using a block that's big enough */

	retValue = task_mbox_data_get_async_block(&MRTmsg, &MRTblock, testPool);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_data_get_async_block returned %d\n", retValue);
		return TC_FAIL;
	}
	if (strcmp((char *)(MRTblock.pointer_to_data), myData1) != 0) {
		TC_ERROR("task_mbox_data_get_async_block got wrong data #1 (%s)\n",
			MRTblock.pointer_to_data);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait of message header #1 is OK\n", __func__);
	TC_PRINT("%s: task_mbox_data_get_async_block of message data #1 is OK\n",  __func__);

	/* Don't free block yet ... */

	/* Receive message header for block-exhaustion receive test */

	setMsg_Receiver(&MRTmsg, myMbox, ANYTASK, NULL, MSGSIZE);
	retValue = task_mbox_get_wait(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait of message header #2 returned %d\n", retValue);
		return TC_FAIL;
	}
	if (MRTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_get_wait of message header #2 got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}

	/* Try to grab message data using block from an empty pool */

	retValue = task_mbox_data_get_async_block_wait_timeout(&MRTmsg, &MRTblockAlt, testPool, 2);
	if (RC_TIME != retValue) {
		TC_ERROR("task_mbox_data_get_async_block_wait_timeout that should have timed out "
			"returned %d\n", retValue);
		return TC_FAIL;
	}

	/* Free block used with previous message */

	task_mem_pool_free(&MRTblock);

	/* Now grab message data using the newly released block */

	retValue = task_mbox_data_get_async_block(&MRTmsg, &MRTblockAlt, testPool);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_data_get_async_block returned %d\n", retValue);
		return TC_FAIL;
	}
	if (strcmp((char *)(MRTblockAlt.pointer_to_data), myData2) != 0) {
		TC_ERROR("task_mbox_data_get_async_block got wrong data #2 (%s)\n",
			MRTblockAlt.pointer_to_data);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait of message header #2 is OK\n", __func__);
	TC_PRINT("%s: task_mbox_data_get_async_block of message data #2 is OK\n",  __func__);

	/* Free block used with most recent message */

	task_mem_pool_free(&MRTblockAlt);

	/* Sync with Sender Task, since he's about to use a timeout */

	(void) task_sem_give(semSync1);

	/* Receive message header for long-duration receive test */

	setMsg_Receiver(&MRTmsg, myMbox, msgSenderTask, NULL, MSGSIZE);
	retValue = task_mbox_get_wait(myMbox, &MRTmsg);
	if (RC_OK != retValue) {
		TC_ERROR("task_mbox_get_wait of message header #3 returned %d\n", retValue);
		return TC_FAIL;
	}
	if (MRTmsg.info != MSG_INFO1) {
		TC_ERROR("task_mbox_get_wait of message header #3 got wrong info (%d)\n",
			MRTmsg.info);
		return TC_FAIL;
	}
	if (MRTmsg.size != MSGSIZE) {
		TC_ERROR("task_mbox_get_wait of message header #3 got wrong size (%d)\n",
			MRTmsg.size);
		return TC_FAIL;
	}

	/* Now sleep long enough for sender's timeout to expire */

	task_sleep(10);

	/* Sender should still be blocked, so grab the message data */

	setMsg_RecvBuf(&MRTmsg, rxBuffer, MSGSIZE);
	task_mbox_data_get(&MRTmsg);
	if (strcmp(MRTmsg.rx_data, myData3) != 0) {
		TC_ERROR("task_mbox_data_get got wrong data #3 (%s)\n", MRTmsg.rx_data);
		return TC_FAIL;
	}

	TC_PRINT("%s: task_mbox_get_wait of message header #3 is OK\n", __func__);
	TC_PRINT("%s: task_mbox_data_get of message data #3 is OK\n", __func__);

	return TC_PASS;
}
