#include "CoCo3EmuPGE.h"

int main()
{
	CoCoEmuPGE emuMainWindow;

	if (emuMainWindow.Construct(320, 242, 2, 2))
		emuMainWindow.Start();

	return 0;
}