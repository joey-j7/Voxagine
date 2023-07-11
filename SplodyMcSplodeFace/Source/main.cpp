#include "VoxApp.h"

#if defined(_ORBIS)
#define ENTRYPOINT int main(int argc, const char *argv[])
#else
#include <eventtoken.h>
#define ENTRYPOINT int WINAPI wWinMain(_In_ HINSTANCE HInstance, _In_opt_ HINSTANCE HPrevInstance, _In_ LPWSTR LpCmdLine, _In_ int NCmdShow)
#endif

#include <vector>

ENTRYPOINT
{

	VoxApp app;
	app.Run();

	return 0;
}