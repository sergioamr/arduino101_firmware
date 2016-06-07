/* test_device.c - APIs for testing task level interrupts */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
This module exercises the task level interrupt handling feature.

Each function allocates 2 IRQ objects and then tests for an event
associated with the IRQ. The taskA() function also attempts to allocate an IRQ
that has already been allocated by another task. The taskB() function also
exercises the task_irq_free() API.
 */

#include <microkernel.h>
#include <misc/printk.h>

#include <tc_util.h>

#define DEV1_ID	0
#define DEV2_ID	1
#define DEV3_ID	2
#define DEV4_ID	3
#define DEV5_ID	4

#if defined(CONFIG_X86_32)
  #define DEV1_IRQ  8
  #define DEV2_IRQ 14
  #define DEV3_IRQ 32
  #define DEV4_IRQ 34
#elif defined(CONFIG_CPU_CORTEX_M3_M4)
  #define DEV1_IRQ  0
  #define DEV2_IRQ  1
  #define DEV3_IRQ  2
  #define DEV4_IRQ  3
#else
  #error "Unknown target"
#endif

#define NUM_OBJECTS 4
uint32_t irq_vectors[NUM_OBJECTS] = {[0 ... (NUM_OBJECTS - 1)] = INVALID_VECTOR};

/**
 *
 * @brief First of 2 tasks to allocate IRQ objects and check for events
 *
 * This task allocates 2 IRQ objects with unique IRQs and then tests for an
 * interrupt associated with those IRQs. The function then attempts to allocate
 * a device that has already been allocate from taskB.
 *
 * @return TC_PASS, TC_FAIL
 */

int taskA(ksem_t semRdy)
{
	irq_vectors[DEV1_ID] = task_irq_alloc(DEV1_ID, DEV1_IRQ, 1);
	if (irq_vectors[DEV1_ID] == INVALID_VECTOR) {
		TC_ERROR("Not able to allocate IRQ object\n");
		return TC_FAIL;
	}
	TC_PRINT("IRQ object %d using IRQ%d allocated\n", DEV1_ID, DEV1_IRQ);

	irq_vectors[DEV2_ID] = task_irq_alloc(DEV2_ID, DEV2_IRQ, 2);
	if (irq_vectors[DEV2_ID] == INVALID_VECTOR) {
		TC_ERROR("Not able to allocate IRQ object\n");
		return TC_FAIL;
	}
	TC_PRINT("IRQ object %d using IRQ%d allocated\n\n", DEV2_ID, DEV2_IRQ);

	/* Send semaphore to let loader know IRQ objects have been allocated */

	task_sem_give(semRdy);

	if (task_irq_test_wait(DEV1_ID) != RC_OK) {
		TC_ERROR("Not able to test IRQ object event\n");
		return TC_FAIL;
	}
	TC_PRINT("Received event for IRQ object %d\n", DEV1_ID);
	task_irq_ack(DEV1_ID);

	if (task_irq_test_wait(DEV2_ID) != RC_OK) {
		TC_ERROR("Not able to test IRQ object event\n");
		return TC_FAIL;
	}
	TC_PRINT("Received event for IRQ object %d\n", DEV2_ID);
	task_irq_ack(DEV2_ID);

	/* Wait for other task to receive its events */

	(void)task_sem_take_wait(semRdy);

	TC_PRINT("\nAttempt to allocate an IRQ object that\n");
	TC_PRINT("is already allocated by another task...\n");
	if (task_irq_alloc(DEV4_ID, DEV4_IRQ, 1) != INVALID_VECTOR) {
		TC_ERROR("Error: Was able to allocate\n\n");
		return TC_FAIL;
	}
	TC_PRINT("Re-allocation of IRQ object %d prevented\n", DEV4_ID);

	TC_PRINT("\nAttempt to allocate an IRQ that\n"
		      "is already allocated by another task...\n");
	if (task_irq_alloc(DEV5_ID, DEV4_IRQ, 1) != INVALID_VECTOR) {
		TC_ERROR("Error: Was able to allocate\n\n");
		return TC_FAIL;
	}
	TC_PRINT("Re-allocation of IRQ%d prevented\n\n", DEV4_IRQ);

	/* Signal other task that we are done processing */

	task_sem_give(semRdy);

	return TC_PASS;
}

/**
 *
 * @brief Second of 2 tasks to allocate IRQ objects and check for events
 *
 * This task allocates 2 IRQ objects with unique IRQs and then tests for an
 * interrupt associated with those IRQs. The function then frees an IRQ object
 * using task_irq_free().
 *
 * @return TC_PASS, TC_FAIL
 */

int taskB(ksem_t semRdy)
{
	irq_vectors[DEV3_ID] = task_irq_alloc(DEV3_ID, DEV3_IRQ, 1);
	if (irq_vectors[DEV3_ID] == INVALID_VECTOR) {
		TC_ERROR("Not able to allocate IRQ object\n");
		return TC_FAIL;
	}
	TC_PRINT("IRQ object %d using IRQ%d allocated\n", DEV3_ID, DEV3_IRQ);

	irq_vectors[DEV4_ID] = task_irq_alloc(DEV4_ID, DEV4_IRQ, 1);
	if (irq_vectors[DEV4_ID] == INVALID_VECTOR) {
		TC_ERROR("Not able to allocate IRQ object\n");
		return TC_FAIL;
	}
	TC_PRINT("IRQ object %d using IRQ%d allocated\n\n", DEV4_ID, DEV4_IRQ);

	/* Send semaphore to let loader/main know objects have been allocated */

	task_sem_give(semRdy);

	if (task_irq_test_wait(DEV3_ID) != RC_OK) {
		TC_ERROR("Not able to test IRQ object event\n");
		return TC_FAIL;
	}
	TC_PRINT("Received event for IRQ object %d\n", DEV3_ID);
	task_irq_ack(DEV3_ID);

	if (task_irq_test_wait(DEV4_ID) != RC_OK) {
		TC_ERROR("Not able to test IRQ object event\n");
		return TC_FAIL;
	}
	TC_PRINT("Received event for IRQ object %d\n", DEV4_ID);
	task_irq_ack(DEV4_ID);

	/* Signal other task that we are done receiving events */

	task_sem_give(semRdy);

	/*
	 *  Wait for other task to finish processing. The signal just previously
	 *  sent will not be seen here as the other task is a higher priority and
	 *  will thus consume the signal first.
	 */

	(void)task_sem_take_wait(semRdy);

	TC_PRINT("Attempt to free an IRQ object...\n");
	task_irq_free(DEV3_ID);
	TC_PRINT("IRQ object %d freed\n", DEV3_ID);

	return TC_PASS;
}
