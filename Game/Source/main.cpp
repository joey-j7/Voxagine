#include "VoxApp.h"

/* SDL owns the entry point on every platform now, so the wWinMain variant and
   its <eventtoken.h> include are gone. */
int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	VoxApp app;
	app.Run();

	return 0;
}
