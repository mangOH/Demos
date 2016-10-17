/**
 * @file
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */

#include <stdio.h>
#include "canopen_iox1.h"

int main(void)
{
	int rt;
	unsigned char value;

	rt = iox1_init();
	if (rt) {
		printf("iox1_init failed, %d\n", rt);
		return -1;
	}

	value = iox1_DigitalInput_DI0_DI7();
	printf("DI0_DI7 value: 0x%x\n", value);

	value = iox1_DigitalInput_DI8_DI15();
	printf("DI8_DI15 value: 0x%x\n", value);

	value = 0xff;
	iox1_DigitalOutput_DO0_DO7(value);
	printf("DO0_DO7 value: 0x%x\n", value);

	iox1_free();

	return 0;
}
