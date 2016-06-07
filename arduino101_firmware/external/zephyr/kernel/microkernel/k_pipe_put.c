/* command processing for pipe put operation */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#include <micro_private.h>
#include <k_pipe_util.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>


/**
 *
 * @brief Process request command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_request(struct k_args *RequestOrig)
{
	struct k_args *Request;
	struct k_args *RequestProc;

	kpipe_t pipeId = RequestOrig->Args.pipe_req.ReqInfo.pipe.id;

	bool bAsync;

	if (_ASYNCREQ == _k_pipe_request_type_get(&RequestOrig->Args)) {
		bAsync = true;
	} else {
		bAsync = false;
	}

	if (!bAsync) {
		/* First save the pointer to the task's TCB for rescheduling later */
		RequestOrig->Ctxt.proc = _k_current_task;
		_k_state_bit_set(_k_current_task, TF_SEND);
	} else {
		/* No need to put in data about sender, since it's a poster */
		RequestOrig->Ctxt.proc = NULL;
	}

	mycopypacket(&Request, RequestOrig);

	/* if we end up here, we arrived at destination node and the packet
	   Request is not local */

	/* Now, we need a new packet for processing of the request; the
	   Request package is too small b/c of space lost due to possible
	   embedded local data
	 */

	mycopypacket(&RequestProc, Request);
	RequestProc->Args.pipe_xfer_req.ReqInfo.pipe.ptr =
		&(_k_pipe_list[OBJ_INDEX(pipeId)]);

	switch (_k_pipe_request_type_get(&RequestProc->Args)) {
	case _SYNCREQ:
		RequestProc->Args.pipe_xfer_req.pData =
			Request->Args.pipe_req.ReqType.Sync.pData;
		RequestProc->Args.pipe_xfer_req.iSizeTotal =
			Request->Args.pipe_req.ReqType.Sync.iSizeTotal;
		break;
	case _ASYNCREQ:
		RequestProc->Args.pipe_xfer_req.pData =
			Request->Args.pipe_req.ReqType.Async.block.pointer_to_data;
		RequestProc->Args.pipe_xfer_req.iSizeTotal =
			Request->Args.pipe_req.ReqType.Async.iSizeTotal;
		break;
	default:
		break;
	}
	RequestProc->Args.pipe_xfer_req.status = XFER_IDLE;
	RequestProc->Args.pipe_xfer_req.iNbrPendXfers = 0;
	RequestProc->Args.pipe_xfer_req.iSizeXferred = 0;

	RequestProc->Forw = NULL;
	RequestProc->Head = NULL;

	switch (RequestProc->Time.ticks) {
	case TICKS_NONE:
		_k_pipe_time_type_set(&RequestProc->Args, _TIME_NB);
		break;
	case TICKS_UNLIMITED:
		_k_pipe_time_type_set(&RequestProc->Args, _TIME_B);
		break;
	default:
		_k_pipe_time_type_set(&RequestProc->Args, _TIME_BT);
		break;
	}

	/* start processing */

	struct pipe_struct *pPipe;

	pPipe = RequestProc->Args.pipe_xfer_req.ReqInfo.pipe.ptr;

	do {
		int iSpace2WriteinReaders;
		int iFreeBufferSpace;
		int iTotalSpace2Write;
		int32_t ticks;

		iSpace2WriteinReaders = CalcFreeReaderSpace(pPipe->Readers);
		iFreeBufferSpace =
			pPipe->desc.iFreeSpaceCont + pPipe->desc.iFreeSpaceAWA;
		iTotalSpace2Write =
			iFreeBufferSpace + iSpace2WriteinReaders;

		if (0 == iTotalSpace2Write)
			break; /* special case b/c even not good enough for 1_TO_N */

		/* (possibly) do some processing */

		ticks = RequestProc->Time.ticks;
		RequestProc->Time.timer = NULL;
		_k_pipe_process(pPipe, RequestProc /* writer */, NULL /* reader */);
		RequestProc->Time.ticks = ticks;

		/* check if request was processed */

		if (TERM_XXX & RequestProc->Args.pipe_xfer_req.status) {
			RequestProc->Time.timer = NULL; /* not really required */
			return; /* not listed anymore --> completely processed */
		}

	} while (0);

	/* if we got up to here, we did none or SOME (partial)
	 * processing on the request
	 */

	if (_TIME_NB !=
		_k_pipe_time_type_get(&RequestProc->Args)) {
		/* call is blocking */
		INSERT_ELM(pPipe->Writers, RequestProc);
		/*
		 * NOTE: It is both faster and simpler to blindly assign the
		 * PIPE_PUT_TIMEOUT microkernel command to the packet even though it
		 * is only useful to the finite timeout case.
		 */
		RequestProc->Comm = PIPE_PUT_TIMEOUT;
		if (_TIME_B == _k_pipe_time_type_get(&RequestProc->Args)) {
			/*
			 * The writer specified TICKS_UNLIMITED; NULL the timer.
			 */
			RequestProc->Time.timer = NULL;
			return;
		} else {
			/* { TIME_BT } */
#ifdef CANCEL_TIMERS
			if (RequestProc->Args.pipe_xfer_req.iSizeXferred != 0) {
				RequestProc->Time.timer = NULL;
			} else
#endif
				/* enlist a new timer into the timeout chain */
				_k_timeout_alloc(RequestProc);

			return;
		}
	} else {
		/* call is non-blocking;
		   Check if we don't have to queue it b/c it could not
		   be processed at once
		 */
		RequestProc->Time.timer = NULL;

		if (XFER_BUSY == RequestProc->Args.pipe_xfer_req.status) {
			INSERT_ELM(pPipe->Writers, RequestProc);
		} else {
			__ASSERT_NO_MSG(XFER_IDLE ==
				RequestProc->Args.pipe_xfer_req.status);
			__ASSERT_NO_MSG(0 == RequestProc->Args.pipe_xfer_req.iSizeXferred);
			RequestProc->Comm = PIPE_PUT_REPLY;
			_k_pipe_put_reply(RequestProc);
		}
		return;
	}
}

/**
 *
 * @brief Perform timeout command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_timeout(struct k_args *ReqProc)
{
	__ASSERT_NO_MSG(NULL != ReqProc->Time.timer);

	myfreetimer(&(ReqProc->Time.timer));
	_k_pipe_request_status_set(&ReqProc->Args.pipe_xfer_req, TERM_TMO);

	DeListWaiter(ReqProc);
	if (0 == ReqProc->Args.pipe_xfer_req.iNbrPendXfers) {
		_k_pipe_put_reply(ReqProc);
	}
}

/**
 *
 * @brief Process reply command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_reply(struct k_args *ReqProc)
{
	__ASSERT_NO_MSG(
		0 == ReqProc->Args.pipe_xfer_req.iNbrPendXfers /*  no pending Xfers */
	    && NULL == ReqProc->Time.timer /*  no pending timer */
	    && NULL == ReqProc->Head); /*  not in list */

	/* orig packet must be sent back, not ReqProc */

	struct k_args *ReqOrig = ReqProc->Ctxt.args;
	PIPE_REQUEST_STATUS status;

	ReqOrig->Comm = PIPE_PUT_ACK;

	/* determine return value:
	 */
	status = ReqProc->Args.pipe_xfer_req.status;
	if (unlikely(TERM_TMO == status)) {
		ReqOrig->Time.rcode = RC_TIME;
	} else if ((TERM_XXX | XFER_IDLE) & status) {
		K_PIPE_OPTION Option = _k_pipe_option_get(&ReqProc->Args);

		if (likely(ReqProc->Args.pipe_xfer_req.iSizeXferred ==
				   ReqProc->Args.pipe_xfer_req.iSizeTotal)) {
			/* All data has been transferred */
			ReqOrig->Time.rcode = RC_OK;
		} else if (ReqProc->Args.pipe_xfer_req.iSizeXferred != 0) {
			/* Some but not all data has been transferred */
			ReqOrig->Time.rcode = (Option == _ALL_N) ? RC_INCOMPLETE : RC_OK;
		} else {
			/* No data has been transferred */
			ReqOrig->Time.rcode = (Option == _0_TO_N) ? RC_OK : RC_FAIL;
		}
	} else {
		/* unknown (invalid) status */
		__ASSERT_NO_MSG(1 == 0); /* should not come here */
	}
	if (_ASYNCREQ != _k_pipe_request_type_get(&ReqOrig->Args)) {
		ReqOrig->Args.pipe_ack.iSizeXferred =
			ReqProc->Args.pipe_xfer_req.iSizeXferred;
	}

	SENDARGS(ReqOrig);

	FREEARGS(ReqProc);
}

/**
 *
 * @brief Process acknowledgment command for a pipe put operation
 *
 * @return N/A
 */

void _k_pipe_put_ack(struct k_args *Request)
{
	if (_ASYNCREQ == _k_pipe_request_type_get(&Request->Args)) {
		struct _pipe_ack_arg *pipe_ack = &Request->Args.pipe_ack;
		struct k_args A;
		struct k_block *blockptr;

		/* invoke command to release block */
		blockptr = &pipe_ack->ReqType.Async.block;
		A.Comm = REL_BLOCK;
		A.Args.p1.poolid = blockptr->poolid;
		A.Args.p1.req_size = blockptr->req_size;
		A.Args.p1.rep_poolptr = blockptr->address_in_pool;
		A.Args.p1.rep_dataptr = blockptr->pointer_to_data;
		_k_mem_pool_block_release(&A); /* will return immediately */

		if ((ksem_t)NULL != pipe_ack->ReqType.Async.sema) {
			/* invoke command to signal sema */
			struct k_args A;

			A.Comm = SIGNALS;
			A.Args.s1.sema = pipe_ack->ReqType.Async.sema;
			_k_sem_signal(&A); /* will return immediately */
		}
	} else {
		/* Reschedule the sender task */
		struct k_args *LocalReq;

		LocalReq = Request->Ctxt.args;
		LocalReq->Time.rcode = Request->Time.rcode;
		LocalReq->Args.pipe_ack = Request->Args.pipe_ack;

		_k_state_bit_reset(LocalReq->Ctxt.proc, TF_SEND | TF_SENDDATA);
	}

	FREEARGS(Request);
}
