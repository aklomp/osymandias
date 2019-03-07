SECTIONS {

	.programs : ALIGN(32) SUBALIGN(4)
	{
		programs_list_start = .;
		KEEP (*(.programs))
		programs_list_end = .;
	}
}

INSERT AFTER .rodata;
