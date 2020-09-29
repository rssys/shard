#define N 10000

#define LBR_ADDR(upper, lower) (((unsigned long) upper << 32) | (unsigned long) lower)

void enable_lbr(void) {
	asm volatile("xor %%edx, %%edx;"
		     "xor %%eax, %%eax;"
		     "inc %%eax;"
		     "mov $0x1d9, %%ecx;"
		     "wrmsr;"
		     : : );
}

void disable_lbr(void) {
	asm volatile("xor %%edx, %%edx;"
		     "xor %%eax, %%eax;"
		     "mov $0x1d9, %%ecx;"
		     "wrmsr;"
		     : : );
}


// void filter_lbr(void) {
// 	asm volatile("xor %%edx, %%edx;"
// 		     "xor %%eax, %%eax;"
// 		     "mov $0x100, %%eax;" // set bits 7, 6
// 		     // "mov $0x139, %%eax;" // capture ring > 0
// 		     //		     "mov $0x1fa, %%eax;"
// 		     "mov $0x1c8, %%ecx;"
// 		     "wrmsr;"
// 		     : : );
// }


// static unsigned long get_lbr_from(unsigned long to) {
// 	int ax1f, dx1f, ax1t, dx1t, msr_from_counter, msr_to_counter;
// 	int ax1i, dx1i, msr_lbr_info;

// 	for (msr_from_counter = 1664,msr_to_counter = 1728,msr_lbr_info=3520;
// 	     /* msr_from_counter < 1680; msr_from_counter++, msr_to_counter++) { */
// 	     msr_from_counter < 1696; msr_from_counter++, msr_to_counter++, msr_lbr_info++) {
// 		asm volatile("mov %6, %%ecx;"
// 			     "rdmsr;"
// 			     "mov %%eax, %0;"
// 			     "mov %%edx, %1;"
// 			     "mov %7, %%ecx;"
// 			     "rdmsr;"
// 			     "mov %%eax, %2;"
// 			     "mov %%edx, %3;"
// 		// 	     "mov %8, %%ecx;"
// 		// 	     "rdmsr;"
// 		// 	     "mov %%eax, %4;"
// 		// 	     "mov %%edx, %5;"
// 			     : "=g" (ax1f), "=g" (dx1f), "=g" (ax1t), "=g" (dx1t), "=g" (ax1i), "=g" (dx1i)
// 			     : "g" (msr_from_counter), "g" (msr_to_counter), "g" (msr_lbr_info)
// 			     : "%eax", "%ecx", "%edx"
// 			     );

// 		if(LBR_ADDR(dx1t, ax1t) == to) {
// 			// printk(KERN_INFO "On cpu %d, branch from: %8x%8x (MSR: %X), to %8x%8x (MSR %X)\n",
// 			//        smp_processor_id(), dx1f, ax1f, msr_from_counter, dx1t, ax1t, msr_to_counter);
// 			// printk(KERN_INFO "%8x %8x\n", dx1i, ax1i);
// 			return LBR_ADDR(dx1f, ax1f);
// 		}
// 	}
// 	return 0;
// }



static unsigned long get_lbr_from(unsigned long to) {
	int ax1f, dx1f, ax1t, dx1t, msr_from_counter, msr_to_counter;
	int ax1i, dx1i, msr_lbr_info;

	asm volatile("mov $0x1c9, %%ecx;"
		     "rdmsr;"
		     "mov %%eax, %0;"
		     : "=g" (ax1f), "=g" (dx1f), "=g" (ax1t), "=g" (dx1t), "=g" (ax1i), "=g" (dx1i)
		     : "g" (msr_from_counter), "g" (msr_to_counter), "g" (msr_lbr_info)
		     : "%eax", "%ecx", "%edx"
		     );

	msr_from_counter = 1664 + ax1f;
	msr_to_counter = 1728 + ax1f;
	asm volatile("mov %6, %%ecx;"
		     "rdmsr;"
		     "mov %%eax, %0;"
		     "mov %%edx, %1;"
		     "mov %7, %%ecx;"
		     "rdmsr;"
		     "mov %%eax, %2;"
		     "mov %%edx, %3;"
		     : "=g" (ax1f), "=g" (dx1f), "=g" (ax1t), "=g" (dx1t), "=g" (ax1i), "=g" (dx1i)
		     : "g" (msr_from_counter), "g" (msr_to_counter), "g" (msr_lbr_info)
		     : "%eax", "%ecx", "%edx"
		     );

	if(LBR_ADDR(dx1t, ax1t) == to) {
		// printk(KERN_INFO "On cpu %d, branch from: %8x%8x (MSR: %X), to %8x%8x (MSR %X)\n",
		//        smp_processor_id(), dx1f, ax1f, msr_from_counter, dx1t, ax1t, msr_to_counter);
		// printk(KERN_INFO "matched\n");
		return LBR_ADDR(dx1f, ax1f);
	} 
	return 0;
}


static void print_lbr(void) {
	int ax1f, dx1f, ax1t, dx1t, msr_from_counter, msr_to_counter;
	int ax1i, dx1i, msr_lbr_info;

	for (msr_from_counter = 1664,msr_to_counter = 1728,msr_lbr_info=3520;
	     /* msr_from_counter < 1680; msr_from_counter++, msr_to_counter++) { */
	     msr_from_counter < 1696; msr_from_counter++, msr_to_counter++, msr_lbr_info++) {
		asm volatile("mov %6, %%ecx;"
			     "rdmsr;"
			     "mov %%eax, %0;"
			     "mov %%edx, %1;"
			     "mov %7, %%ecx;"
			     "rdmsr;"
			     "mov %%eax, %2;"
			     "mov %%edx, %3;"
		// 	     "mov %8, %%ecx;"
		// 	     "rdmsr;"
		// 	     "mov %%eax, %4;"
		// 	     "mov %%edx, %5;"
			     : "=g" (ax1f), "=g" (dx1f), "=g" (ax1t), "=g" (dx1t), "=g" (ax1i), "=g" (dx1i)
			     : "g" (msr_from_counter), "g" (msr_to_counter), "g" (msr_lbr_info)
			     : "%eax", "%ecx", "%edx"
			     );

		printk(KERN_INFO "On cpu %d, branch from: %8x%8x (MSR: %X), to %8x%8x (MSR %X)\n",
		       smp_processor_id(), dx1f, ax1f, msr_from_counter, dx1t, ax1t, msr_to_counter);
		printk(KERN_INFO "%8x %8x\n", dx1i, ax1i);
	}
}