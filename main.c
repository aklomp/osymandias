#include "gui.h"

// Application entry point
int
main (int argc, char **argv)
{
	// Initialize GUI:
	if (!gui_init(&argc, &argv))
		return 1;

	// Run GUI:
	if (!gui_run())
		return 1;

	return 0;
}
