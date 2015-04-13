### Intro. ###
wiimoteInput is based on Raw Input API of Windows XP. It provided a easy to use and event-driven interface for Wiimotes or other expansions in the future.
The latest code is in the SVN respository.

### Quick Start ###
```
#include "wiimoteInput.h"

// Initialization
if (!wiiInput.init(<Handle of Main Window>))) {
	std::cout << "wiimoteInput initialization faild." << std::endl;
	return;
}
wiiInput[0]->setThreshold(1);
wiiInput[0]->setMode(WIIMOTE_MODE_ACC);
...

// Message Processing Function
LRESULT CALLBACK Sample::WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg) {
	case WM_WIIMOTE_ACCELER:
		WIIMOTE_ACCELEROMETER acc;
		wiiInput[wParam]->getAcceleration(acc);
		std::cout << "Xacc = " << acc.x << ", Yacc = " << acc.y << ", Zacc = %f" << acc.z << std::endl;
		break;
		...
	case WM_INPUT:
		wiiInput.processInputMessage(wParam, lParam);
		break;
	}
	...
}
```

### Requirement ###
  * Visual C++ 2005
  * Windows 2003 DDK
  * wxWidgets 2.8.0 (for wiimoteTest)

### Support ###
Please mail to shuncox AT gmail DOT com