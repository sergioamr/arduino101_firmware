/* k_pipe_buffer.c */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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


/* Implementation remarks:
- when using a floating end pointer: do not use pipe_desc->iBuffsize for
  (pipe_desc->pEnd - pipe_desc->pBegin)
 */

#include <microkernel/base_api.h>
#include <k_pipe_buffer.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>
#include <misc/__assert.h>

#define STORE_NBR_MARKERS
/* NOTE: the number of pending write and read Xfers is always stored,
   as it is required for the pipes to function properly. It is stored in the
   pipe descriptor fields iNbrPendingWrites and iNbrPendingReads.

   In the Writer and Reader MarkersList, the number of markers (==nbr. of
   unreleased Xfers)
   is monitored as well. They actually equal iNbrPendingWrites and
   iNbrPendingReads.
   Their existence depends on STORE_NBR_MARKERS. A reason to have them
   additionally is that
   some extra consistency checking is performed in the markers manipulation
   functionality
   itself.
   Drawback: double storage of nbr. of pending write Xfers (but for test
   purposes this is
   acceptable I think)
 */

#define CHECK_BUFFER_POINTER(pData) \
	__ASSERT_NO_MSG(desc->pBegin <= pData && pData < desc->pEnd)

static void pipe_intrusion_check(struct pipe_desc *desc, unsigned char *pBegin, int iSize);

/**
 * Markers
 */

static int MarkerFindFree(struct marker aMarkers[])
{
	struct marker *pM = aMarkers;
	int i;

	for (i = 0; i < MAXNBR_MARKERS; i++, pM++) {
		if (NULL == pM->pointer) {
			break;
		}
	}
	if (MAXNBR_MARKERS == i) {
		i = -1;
	}

	return i;
}

static void MarkerLinkToListAfter(struct marker aMarkers[],
								  int iMarker, int iNewMarker)
{
	int iNextMarker; /* index of next marker in original list */

	/* let the original list be aware of the new marker */
	if (-1 != iMarker) {
		iNextMarker = aMarkers[iMarker].Next;
		aMarkers[iMarker].Next = iNewMarker;
		if (-1 != iNextMarker) {
			aMarkers[iNextMarker].Prev = iNewMarker;
		} else {
			/* there was no next marker */
		}
	} else {
		iNextMarker = -1; /* there wasn't even a marker */
	}

	/* link the new marker with the marker and next marker */
	aMarkers[iNewMarker].Prev = iMarker;
	aMarkers[iNewMarker].Next = iNextMarker;
}

static int MarkerAddLast(struct marker_list *pMarkerList,
						 unsigned char *pointer, int iSize, bool bXferBusy)
{
	int i = MarkerFindFree(pMarkerList->aMarkers);

	if (i == -1) {
		return i;
	}

	pMarkerList->aMarkers[i].pointer = pointer;
	pMarkerList->aMarkers[i].size = iSize;
	pMarkerList->aMarkers[i].bXferBusy = bXferBusy;

	if (-1 == pMarkerList->iFirstMarker) {
		__ASSERT_NO_MSG(-1 == pMarkerList->iLastMarker);
		pMarkerList->iFirstMarker = i; /* we still need to set Prev & Next */
	} else {
		__ASSERT_NO_MSG(-1 != pMarkerList->iLastMarker);
		__ASSERT_NO_MSG(-1 ==
		       pMarkerList->aMarkers[pMarkerList->iLastMarker].Next);
	}

	MarkerLinkToListAfter(pMarkerList->aMarkers, pMarkerList->iLastMarker, i);

	__ASSERT_NO_MSG(-1 == pMarkerList->aMarkers[i].Next);
	pMarkerList->iLastMarker = i;

#ifdef STORE_NBR_MARKERS
	pMarkerList->iNbrMarkers++;
	__ASSERT_NO_MSG(0 < pMarkerList->iNbrMarkers);
#endif

	return i;
}

static void MarkerUnlinkFromList(struct marker aMarkers[], int iMarker,
								 int *piPredecessor, int *piSuccessor)
{
	int iNextMarker = aMarkers[iMarker].Next;
	int iPrevMarker = aMarkers[iMarker].Prev;

	/* remove the marker from the list */
	aMarkers[iMarker].Next = -1;
	aMarkers[iMarker].Prev = -1;

	/* repair the chain */
	if (-1 != iPrevMarker) {
		aMarkers[iPrevMarker].Next = iNextMarker;
	}
	if (-1 != iNextMarker) {
		aMarkers[iNextMarker].Prev = iPrevMarker;
	}
	*piPredecessor = iPrevMarker;
	*piSuccessor = iNextMarker;
}

static void MarkerDelete(struct marker_list *pMarkerList, int index)
{
	int i;
	int iPredecessor;
	int iSuccessor;

	i = index;

	__ASSERT_NO_MSG(-1 != i);

	pMarkerList->aMarkers[i].pointer = NULL;
	MarkerUnlinkFromList(pMarkerList->aMarkers, i, &iPredecessor, &iSuccessor);

	/* update first/last info */
	if (i == pMarkerList->iLastMarker) {
		pMarkerList->iLastMarker = iPredecessor;
	}
	if (i == pMarkerList->iFirstMarker) {
		pMarkerList->iFirstMarker = iSuccessor;
	}

#ifdef STORE_NBR_MARKERS
	pMarkerList->iNbrMarkers--;
	__ASSERT_NO_MSG(0 <= pMarkerList->iNbrMarkers);

	if (0 == pMarkerList->iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == pMarkerList->iFirstMarker);
		__ASSERT_NO_MSG(-1 == pMarkerList->iLastMarker);
	}
#endif
}

static void MarkersClear(struct marker_list *pMarkerList)
{
	struct marker *pM = pMarkerList->aMarkers;
	int i;

	for (i = 0; i < MAXNBR_MARKERS; i++, pM++) {
		memset(pM, 0, sizeof(struct marker));
		pM->Next = -1;
		pM->Prev = -1;
	}
#ifdef STORE_NBR_MARKERS
	pMarkerList->iNbrMarkers = 0;
#endif
	pMarkerList->iFirstMarker = -1;
	pMarkerList->iLastMarker = -1;
	pMarkerList->iAWAMarker = -1;
}

/**/

/* note on setting/clearing markers/guards:

  If there is at least one marker, there is a guard and equals one of the
  markers; if there are no markers (*), there is no guard.
  Consequently, if a marker is add when there were none, the guard will equal
  it. If additional markers are add, the guard will not change.
  However, if a marker is deleted:
    if it equals the guard a new guard must be selected (**)
    if not, guard doesn't change

  (*) we need to housekeep how much markers there are or we can inspect the
  guard
  (**) for this, the complete markers table needs to be investigated
 */

/**/

/* This function will see if one or more 'areas' in the buffer
   can be made available (either for writing xor reading).
   Note: such a series of areas starts from the beginning.
 */
static int ScanMarkers(struct marker_list *pMarkerList,
					   int *piSizeBWA, int *piSizeAWA, int *piNbrPendingXfers)
{
	struct marker *pM;
	bool bMarkersAreNowAWA;
	int index;

	index = pMarkerList->iFirstMarker;

	__ASSERT_NO_MSG(-1 != index);

	bMarkersAreNowAWA = false;
	do {
		int index_next;

		__ASSERT_NO_MSG(index == pMarkerList->iFirstMarker);

		if (index == pMarkerList->iAWAMarker) {
			bMarkersAreNowAWA = true; /* from now on, everything is AWA */
		}

		pM = &(pMarkerList->aMarkers[index]);

		if (pM->bXferBusy == true) {
			break;
		}

		if (!bMarkersAreNowAWA) {
			*piSizeBWA += pM->size;
		} else {
			*piSizeAWA += pM->size;
		}

		index_next = pM->Next;
		/* pMarkerList->iFirstMarker will be updated */
		MarkerDelete(pMarkerList, index);
		/* adjust *piNbrPendingXfers */
		if (piNbrPendingXfers) {
			__ASSERT_NO_MSG(0 <= *piNbrPendingXfers);
			(*piNbrPendingXfers)--;
		}
		index = index_next;
	} while (-1 != index);

	__ASSERT_NO_MSG(index == pMarkerList->iFirstMarker);

	if (bMarkersAreNowAWA) {
		pMarkerList->iAWAMarker = pMarkerList->iFirstMarker;
	}

#ifdef STORE_NBR_MARKERS
	if (0 == pMarkerList->iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == pMarkerList->iFirstMarker);
		__ASSERT_NO_MSG(-1 == pMarkerList->iLastMarker);
		__ASSERT_NO_MSG(-1 == pMarkerList->iAWAMarker);
	}
#endif

	return pMarkerList->iFirstMarker;
}

/**
 * General
 */

void BuffInit(unsigned char *pBuffer, int *piBuffSize, struct pipe_desc *desc)
{
	desc->pBegin = pBuffer;

	desc->iBuffSize = *piBuffSize;

	/* reset all pointers */

	desc->pEnd = desc->pBegin + OCTET_TO_SIZEOFUNIT(desc->iBuffSize);
	desc->pEndOrig = desc->pEnd;

	/* assumed it is allowed */
	desc->BuffState = BUFF_EMPTY;
	desc->pEnd = desc->pEndOrig;
	desc->pWrite = desc->pBegin;
	desc->pWriteGuard = NULL;
	desc->bWriteWA = false;
	desc->pRead = desc->pBegin;
	desc->pReadGuard = NULL;
	desc->bReadWA = true; /* YES!! */
	desc->iFreeSpaceCont = desc->iBuffSize;
	desc->iFreeSpaceAWA = 0;
	desc->iNbrPendingReads = 0;
	desc->iAvailDataCont = 0;
	desc->iAvailDataAWA = 0;
	desc->iNbrPendingWrites = 0;
	MarkersClear(&desc->WriteMarkers);
	MarkersClear(&desc->ReadMarkers);

}

int CalcFreeSpace(struct pipe_desc *desc, int *piFreeSpaceCont,
				  int *piFreeSpaceAWA)
{
	unsigned char *pStart = desc->pWrite;
	unsigned char *pStop = desc->pRead;

	if (NULL != desc->pWriteGuard) {
		pStop = desc->pWriteGuard;
	} else {
		/*
		 * if BuffState==BUFF_EMPTY but we have a WriteGuard,
		 * we still need to calculate it as a normal [Start,Stop] interval
		 */

		if (BUFF_EMPTY == desc->BuffState) {
			*piFreeSpaceCont = SIZEOFUNIT_TO_OCTET(desc->pEnd - pStart);
			*piFreeSpaceAWA = SIZEOFUNIT_TO_OCTET(pStop - desc->pBegin);
			return (*piFreeSpaceCont + *piFreeSpaceAWA);
			/* this sum equals pEnd-pBegin */
		}
	}

	/*
	 * on the other hand, if BuffState is full, we do not need a special flow;
	 * it will be correct as (pStop - pStart) equals 0
	 */

	if (pStop >= pStart) {
		*piFreeSpaceCont = SIZEOFUNIT_TO_OCTET(pStop - pStart);
		*piFreeSpaceAWA = 0;
	} else {
		*piFreeSpaceCont = SIZEOFUNIT_TO_OCTET(desc->pEnd - pStart);
		*piFreeSpaceAWA = SIZEOFUNIT_TO_OCTET(pStop - desc->pBegin);
	}
	return (*piFreeSpaceCont + *piFreeSpaceAWA);
}

void BuffGetFreeSpace(struct pipe_desc *desc, int *piFreeSpaceTotal,
					  int *piFreeSpaceCont, int *piFreeSpaceAWA)
{
	int iFreeSpaceCont;
	int iFreeSpaceAWA;
	int iFreeSpaceTotal;

	iFreeSpaceTotal =
		CalcFreeSpace(desc, &iFreeSpaceCont, &iFreeSpaceAWA);
	__ASSERT_NO_MSG(iFreeSpaceCont == desc->iFreeSpaceCont);
	__ASSERT_NO_MSG(iFreeSpaceAWA == desc->iFreeSpaceAWA);
	*piFreeSpaceTotal = iFreeSpaceTotal;
	*piFreeSpaceCont = desc->iFreeSpaceCont;
	*piFreeSpaceAWA = desc->iFreeSpaceAWA;
}

void BuffGetFreeSpaceTotal(struct pipe_desc *desc, int *piFreeSpaceTotal)
{
	int dummy1, dummy2;
	*piFreeSpaceTotal = CalcFreeSpace(desc, &dummy1, &dummy2);
	__ASSERT_NO_MSG(dummy1 == desc->iFreeSpaceCont);
	__ASSERT_NO_MSG(dummy2 == desc->iFreeSpaceAWA);
}

int BuffEmpty(struct pipe_desc *desc)
{
	/* 0==iAvailDataTotal is an INcorrect condition b/c of async behavior */

	int iTotalFreeSpace;

	BuffGetFreeSpaceTotal(desc, &iTotalFreeSpace);
	return (desc->iBuffSize == iTotalFreeSpace);
}

int CalcAvailData(struct pipe_desc *desc, int *piAvailDataCont,
				  int *piAvailDataAWA)
{
	unsigned char *pStart = desc->pRead;
	unsigned char *pStop = desc->pWrite;

	if (NULL != desc->pReadGuard) {
		pStop = desc->pReadGuard;
	} else {
		/*
		 * if BuffState==BUFF_FULL but we have a ReadGuard,
		 * we still need to calculate it as a normal [Start,Stop] interval
		 */

		if (BUFF_FULL == desc->BuffState) {
			*piAvailDataCont = SIZEOFUNIT_TO_OCTET(desc->pEnd - pStart);
			*piAvailDataAWA = SIZEOFUNIT_TO_OCTET(pStop - desc->pBegin);
			return (*piAvailDataCont + *piAvailDataAWA);
			/* this sum equals pEnd-pBegin */
		}
	}

	/*
	 * on the other hand, if BuffState is empty, we do not need a special flow;
	 * it will be correct as (pStop - pStart) equals 0
	 */

	if (pStop >= pStart) {
		*piAvailDataCont = SIZEOFUNIT_TO_OCTET(pStop - pStart);
		*piAvailDataAWA = 0;
	} else {
		*piAvailDataCont = SIZEOFUNIT_TO_OCTET(desc->pEnd - pStart);
		*piAvailDataAWA = SIZEOFUNIT_TO_OCTET(pStop - desc->pBegin);
	}
	return (*piAvailDataCont + *piAvailDataAWA);
}

void BuffGetAvailData(struct pipe_desc *desc, int *piAvailDataTotal,
					  int *piAvailDataCont, int *piAvailDataAWA)
{
	int iAvailDataCont;
	int iAvailDataAWA;
	int iAvailDataTotal;

	iAvailDataTotal =
		CalcAvailData(desc, &iAvailDataCont, &iAvailDataAWA);
	__ASSERT_NO_MSG(iAvailDataCont == desc->iAvailDataCont);
	__ASSERT_NO_MSG(iAvailDataAWA == desc->iAvailDataAWA);
	*piAvailDataTotal = iAvailDataTotal;
	*piAvailDataCont = desc->iAvailDataCont;
	*piAvailDataAWA = desc->iAvailDataAWA;
}

void BuffGetAvailDataTotal(struct pipe_desc *desc, int *piAvailDataTotal)
{
	int dummy1, dummy2;

	*piAvailDataTotal = CalcAvailData(desc, &dummy1, &dummy2);
	__ASSERT_NO_MSG(dummy1 == desc->iAvailDataCont);
	__ASSERT_NO_MSG(dummy2 == desc->iAvailDataAWA);
}

int BuffFull(struct pipe_desc *desc)
{
	/* 0==iTotalFreeSpace is an INcorrect condition b/c of async behavior */

	int iAvailDataTotal;

	BuffGetAvailDataTotal(desc, &iAvailDataTotal);
	return (desc->iBuffSize == iAvailDataTotal);
}

/**
 * Buffer en-queuing:
 */

static int AsyncEnQRegstr(struct pipe_desc *desc, int iSize)
{
	int i;

	pipe_intrusion_check(desc, desc->pWrite, iSize);

	i = MarkerAddLast(&desc->WriteMarkers, desc->pWrite, iSize, true);
	if (i != -1) {
		/* adjust iNbrPendingWrites */
		__ASSERT_NO_MSG(0 <= desc->iNbrPendingWrites);
		desc->iNbrPendingWrites++;
		/* pReadGuard changes? */
		if (NULL == desc->pReadGuard) {
			desc->pReadGuard = desc->pWrite;
		}
		__ASSERT_NO_MSG(desc->WriteMarkers.aMarkers
						[desc->WriteMarkers.iFirstMarker].pointer ==
						desc->pReadGuard);
		/* iAWAMarker changes? */
		if (-1 == desc->WriteMarkers.iAWAMarker && desc->bWriteWA) {
			desc->WriteMarkers.iAWAMarker = i;
		}
	}
	return i;
}

static void AsyncEnQFinished(struct pipe_desc *desc, int iTransferID)
{
	desc->WriteMarkers.aMarkers[iTransferID].bXferBusy = false;

	if (desc->WriteMarkers.iFirstMarker == iTransferID) {
		int iNewFirstMarker = ScanMarkers(&desc->WriteMarkers,
										  &desc->iAvailDataCont,
										  &desc->iAvailDataAWA,
										  &desc->iNbrPendingWrites);
		if (-1 != iNewFirstMarker) {
			desc->pReadGuard =
				desc->WriteMarkers.aMarkers[iNewFirstMarker].pointer;
		} else {
			desc->pReadGuard = NULL;
		}
	}
}

int BuffEnQ(struct pipe_desc *desc, int iSize, unsigned char **ppWrite)
{
	int iTransferID;

	if (0 == BuffEnQA(desc, iSize, ppWrite, &iTransferID)) {
		return 0;
	}

	/* check ret value */

	BuffEnQA_End(desc, iTransferID, iSize /* optional */);
	return iSize;
}

int BuffEnQA(struct pipe_desc *desc, int iSize, unsigned char **ppWrite,
			 int *piTransferID)
{
	if (iSize > desc->iFreeSpaceCont) {
		return 0;
	}
	*piTransferID = AsyncEnQRegstr(desc, iSize);
	if (-1 == *piTransferID) {
		return 0;
	}

	*ppWrite = desc->pWrite;

	/* adjust write pointer and free space*/

	desc->pWrite += OCTET_TO_SIZEOFUNIT(iSize);
	if (desc->pEnd == desc->pWrite) {
		desc->pWrite = desc->pBegin;
		desc->iFreeSpaceCont = desc->iFreeSpaceAWA;
		desc->iFreeSpaceAWA = 0;
		desc->bWriteWA = true;
		desc->bReadWA = false;
		desc->ReadMarkers.iAWAMarker = -1;
	} else {
		desc->iFreeSpaceCont -= iSize;
	}

	if (desc->pWrite == desc->pRead) {
		desc->BuffState = BUFF_FULL;
	} else {
		desc->BuffState = BUFF_OTHER;
	}

	CHECK_BUFFER_POINTER(desc->pWrite);

	return iSize;
}

void BuffEnQA_End(struct pipe_desc *desc, int iTransferID,
				  int iSize /* optional */)
{
	ARG_UNUSED(iSize);

	/* An asynchronous data transfer to the buffer has finished */

	AsyncEnQFinished(desc, iTransferID);
}

/**
 * Buffer de-queuing:
 */

static int AsyncDeQRegstr(struct pipe_desc *desc, int iSize)
{
	int i;

	pipe_intrusion_check(desc, desc->pRead, iSize);

	i = MarkerAddLast(&desc->ReadMarkers, desc->pRead, iSize, true);
	if (i != -1) {
		/* adjust iNbrPendingReads */
		__ASSERT_NO_MSG(0 <= desc->iNbrPendingReads);
		desc->iNbrPendingReads++;
		/* pWriteGuard changes? */
		if (NULL == desc->pWriteGuard) {
			desc->pWriteGuard = desc->pRead;
		}
		__ASSERT_NO_MSG(desc->ReadMarkers.aMarkers
						[desc->ReadMarkers.iFirstMarker].pointer ==
						desc->pWriteGuard);
		/* iAWAMarker changes? */
		if (-1 == desc->ReadMarkers.iAWAMarker && desc->bReadWA) {
			desc->ReadMarkers.iAWAMarker = i;
		}
	}
	return i;
}

static void AsyncDeQFinished(struct pipe_desc *desc, int iTransferID)
{
	desc->ReadMarkers.aMarkers[iTransferID].bXferBusy = false;

	if (desc->ReadMarkers.iFirstMarker == iTransferID) {
		int iNewFirstMarker = ScanMarkers(&desc->ReadMarkers,
										  &desc->iFreeSpaceCont,
										  &desc->iFreeSpaceAWA,
										  &desc->iNbrPendingReads);
		if (-1 != iNewFirstMarker) {
			desc->pWriteGuard =
				desc->ReadMarkers.aMarkers[iNewFirstMarker].pointer;
		} else {
			desc->pWriteGuard = NULL;
		}
	}
}

int BuffDeQ(struct pipe_desc *desc, int iSize, unsigned char **ppRead)
{
	int iTransferID;

	if (0 == BuffDeQA(desc, iSize, ppRead, &iTransferID)) {
		return 0;
	}
	BuffDeQA_End(desc, iTransferID, iSize /* optional */);
	return iSize;
}

int BuffDeQA(struct pipe_desc *desc, int iSize, unsigned char **ppRead,
			 int *piTransferID)
{
	/* asynchronous data transfer; read guard pointers must be set */

	if (iSize > desc->iAvailDataCont) {
		/* free space is from read to guard pointer/end pointer */
		return 0;
	}
	*piTransferID = AsyncDeQRegstr(desc, iSize);
	if (-1 == *piTransferID) {
		return 0;
	}

	*ppRead = desc->pRead;

	/* adjust read pointer and avail data */

	desc->pRead += OCTET_TO_SIZEOFUNIT(iSize);
	if (desc->pEnd == desc->pRead) {
		desc->pRead = desc->pBegin;
		desc->iAvailDataCont = desc->iAvailDataAWA;
		desc->iAvailDataAWA = 0;
		desc->bWriteWA = false;
		desc->bReadWA = true;
		desc->WriteMarkers.iAWAMarker = -1;
	} else {
		desc->iAvailDataCont -= iSize;
	}

	if (desc->pWrite == desc->pRead) {
		desc->BuffState = BUFF_EMPTY;
	} else {
		desc->BuffState = BUFF_OTHER;
	}

	CHECK_BUFFER_POINTER(desc->pRead);

	return iSize;
}

void BuffDeQA_End(struct pipe_desc *desc, int iTransferID,
				  int iSize /* optional */)
{
	ARG_UNUSED(iSize);

	/* An asynchronous data transfer from the buffer has finished */

	AsyncDeQFinished(desc, iTransferID);
}

/**
 * Buffer instrusion
 */

static bool AreasCheck4Intrusion(unsigned char *pBegin1, int iSize1,
								 unsigned char *pBegin2, int iSize2)
{
	unsigned char *pEnd1;
	unsigned char *pEnd2;

	pEnd1 = pBegin1 + OCTET_TO_SIZEOFUNIT(iSize1);
	pEnd2 = pBegin2 + OCTET_TO_SIZEOFUNIT(iSize2);

	/*
	 * 2 tests are required to determine the status of the 2 areas,
	 * in terms of their position wrt each other
	 */

	if (pBegin2 >= pBegin1) {
		/* check intrusion of pBegin2 in [pBegin1, pEnd1( */
		if (pBegin2 < pEnd1) {
			/* intrusion!! */
			return true;
		} else {
			/* pBegin2 lies outside and to the right of the first area,
			  intrusion is impossible */
			return false;
		}
	} else {
		/* pBegin2 lies to the left of (pBegin1, pEnd1) */
		/* check end pointer: is pEnd2 in (pBegin1, pEnd1( ?? */
		if (pEnd2 > pBegin1) {
			/* intrusion!! */
			return true;
		} else {
			/* pEnd2 lies outside and to the left of the first area,
			   intrusion is impossible */
			return false;
		}
	}
}

static void pipe_intrusion_check(struct pipe_desc *desc, unsigned char *pBegin, int iSize)
{
	/*
	 * check possible collision with all existing data areas,
	 * both for read and write areas
	 */

	int index;
	struct marker_list *pMarkerList;

	/* write markers */

#ifdef STORE_NBR_MARKERS
	/* first a small consistency check */

	if (0 == desc->WriteMarkers.iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == desc->WriteMarkers.iFirstMarker);
		__ASSERT_NO_MSG(-1 == desc->WriteMarkers.iLastMarker);
		__ASSERT_NO_MSG(-1 == desc->WriteMarkers.iAWAMarker);
	}
#endif

	pMarkerList = &desc->WriteMarkers;
	index = pMarkerList->iFirstMarker;

	while (-1 != index) {
		struct marker *pM;

		pM = &(pMarkerList->aMarkers[index]);

		if (0 != AreasCheck4Intrusion(pBegin, iSize, pM->pointer, pM->size)) {
			__ASSERT_NO_MSG(1 == 0);
		}
		index = pM->Next;
	}

	/* read markers */

#ifdef STORE_NBR_MARKERS
	/* first a small consistency check */

	if (0 == desc->ReadMarkers.iNbrMarkers) {
		__ASSERT_NO_MSG(-1 == desc->ReadMarkers.iFirstMarker);
		__ASSERT_NO_MSG(-1 == desc->ReadMarkers.iLastMarker);
		__ASSERT_NO_MSG(-1 == desc->ReadMarkers.iAWAMarker);
	}
#endif

	pMarkerList = &desc->ReadMarkers;
	index = pMarkerList->iFirstMarker;

	while (-1 != index) {
		struct marker *pM;

		pM = &(pMarkerList->aMarkers[index]);

		if (0 != AreasCheck4Intrusion(pBegin, iSize, pM->pointer, pM->size)) {
			__ASSERT_NO_MSG(1 == 0);
		}
		index = pM->Next;
	}
}
