/* sema.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include "syskernel.h"

struct nano_sem nanoSem1;
struct nano_sem nanoSem2;

/**
 *
 * @brief Initialize semaphores for the test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void sema_test_init(void)
{
	nano_sem_init(&nanoSem1);
	nano_sem_init(&nanoSem2);
}


/**
 *
 * @brief Semaphore test context
 *
 * @param par1   Ignored parameter.
 * @param par2   Number of test loops.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void sema_fiber1(int par1, int par2)
{
	int i;

	ARG_UNUSED(par1);

	for (i = 0; i < par2; i++) {
		nano_fiber_sem_take_wait(&nanoSem1);
		nano_fiber_sem_give(&nanoSem2);
	}
}


/**
 *
 * @brief Semaphore test context
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void sema_fiber2(int par1, int par2)
{
	int i;
	int * pcounter = (int *) par1;

	for (i = 0; i < par2; i++) {
		nano_fiber_sem_give(&nanoSem1);
		nano_fiber_sem_take_wait(&nanoSem2);
		(*pcounter)++;
	}
}

/**
 *
 * @brief Semaphore test context
 *
 * @param par1   Address of the counter.
 * @param par2   Number of test cycles.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void sema_fiber3(int par1, int par2)
{
	int i;
	int * pcounter = (int *) par1;

	for (i = 0; i < par2; i++) {
		nano_fiber_sem_give(&nanoSem1);
		while (!nano_fiber_sem_take(&nanoSem2)) {
			fiber_yield();
		}
		(*pcounter)++;
	}
}


/**
 *
 * @brief The main test entry
 *
 * @return 1 if success and 0 on failure
 *
 * \NOMANUAL
 */

int sema_test(void)
{
	uint32_t t;
	int i = 0;
	int return_value = 0;

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #1");
	fprintf(output_file, sz_description,
			"\n\tnano_sem_init"
			"\n\tnano_fiber_sem_take_wait"
			"\n\tnano_fiber_sem_give");
	printf(sz_test_start_fmt);

	sema_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, sema_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, sema_fiber2, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #2");
	fprintf(output_file, sz_description,
			"\n\tnano_sem_init"
			"\n\tnano_fiber_sem_take"
			"\n\tfiber_yield"
			"\n\tnano_fiber_sem_give");
	printf(sz_test_start_fmt);

	sema_test_init();
	i = 0;

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, sema_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	task_fiber_start(fiber_stack2, STACK_SIZE, sema_fiber3, (int) &i,
					 NUMBER_OF_LOOPS, 3, 0);

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	fprintf(output_file, sz_test_case_fmt,
			"Semaphore #3");
	fprintf(output_file, sz_description,
			"\n\tnano_sem_init"
			"\n\tnano_fiber_sem_take_wait"
			"\n\tnano_fiber_sem_give"
			"\n\tnano_task_sem_give"
			"\n\tnano_task_sem_take_wait");
	printf(sz_test_start_fmt);

	sema_test_init();

	t = BENCH_START();

	task_fiber_start(fiber_stack1, STACK_SIZE, sema_fiber1, 0,
					 NUMBER_OF_LOOPS, 3, 0);
	for (i = 0; i < NUMBER_OF_LOOPS; i++) {
		nano_task_sem_give(&nanoSem1);
		nano_task_sem_take_wait(&nanoSem2);
	}

	t = TIME_STAMP_DELTA_GET(t);

	return_value += check_result(i, t);

	return return_value;
}
