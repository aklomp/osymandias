SECTIONS
{
	.layers : ALIGN(32) SUBALIGN(4)
	{
		layers_list_start = .;
		KEEP (*(SORT(.layer.*)))
		layers_list_end = .;
	}
}

INSERT AFTER .rodata;
