#include "CoCo3EmuPGE.h"

int main()
{
	CoCoEmuPGE emuMainWindow;

	//if (emuMainWindow.Construct(640, 242, 1, 2))
	if (emuMainWindow.Construct(640, 256, 1, 2))
		emuMainWindow.Start();

	return 0;
}