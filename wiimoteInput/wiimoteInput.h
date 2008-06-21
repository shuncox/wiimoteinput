////////////////////////////////////////////////////////////////////////////
//
//  wiimoteInput
//  Copyright(c) Shun Cox (shuncox@gmail.com)
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
////////////////////////////////////////////////////////////////////////////
//
// $Id: wiimoteInput.h,v 1.6 2007/01/26 13:18:39 zhangshun Exp $
//
////////////////////////////////////////////////////////////////////////////

/// @file wiimoteInput.h

#ifndef __WIIMOTEINPUT__
#define __WIIMOTEINPUT__

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0501
#include <windows.h>

extern "C" {
	#include <hidsdi.h>
}

#include "imatrix.hpp"

#define WIIMOTE_STICK_PATHNAME_MAX	512
#define WIIMOTE_STICK_NUMBER_MAX	64

#define WIIMOTE_VENDOR_ID	0x057e
#define WIIMOTE_PRODUCT_ID	0x0306
#define WIIMOTE_USAGE_PAGE	0x01
#define WIIMOTE_USAGE		0x05

/// The WM_WIIMOTE_BUTTON will be sent to the target window as soon as buttons
/// of wiimote were pressed or released
///
/// @param wParam index of the wiimote
/// @param lParam value of buttons
#define WM_WIIMOTE_BUTTON	WM_USER + 1200

/// The WM_WIIMOTE_ACCELER will be sent to the target window as soon as
/// aceleration of wiimote were changed.
///
/// @param wParam index of the wiimote
/// @param lParam none
#define WM_WIIMOTE_ACCELER	WM_USER + 1201

/// Bits of the wiimote button
enum WIIMOTE_BUTTON_VALUES {
	WIIMOTE_BUTTON_TWO		= 0X0001,	///< bit of button '2'
	WIIMOTE_BUTTON_ONE		= 0X0002,	///< bit of button '1'
	WIIMOTE_BUTTON_B		= 0X0004,	///< bit of button 'B'
	WIIMOTE_BUTTON_A		= 0X0008,	///< bit of button 'A'
	WIIMOTE_BUTTON_MINUS	= 0X0010,	///< bit of button '-'
	//Z ACCELERATION, BIT 6 (REPORT 3E) OR BIT 2 (REPORT 3F) 	6 	0X0020
	//Z ACCELERATION, BIT 7 (REPORT 3E) OR BIT 3 (REPORT 3F) 	7 	0X0040
	WIIMOTE_BUTTON_HOME		= 0X0080,	///< bit of button 'Home'
	WIIMOTE_BUTTON_LEFT		= 0X0100,	///< bit of button 'Left'
	WIIMOTE_BUTTON_RIGHT	= 0X0200,	///< bit of button 'Right'
	WIIMOTE_BUTTON_DOWN		= 0X0400,	///< bit of button 'Down'
	WIIMOTE_BUTTON_UP		= 0X0800,	///< bit of button 'Up'
	WIIMOTE_BUTTON_PLUS		= 0X1000	///< bit of button '+'
	//Z ACCELERATION, BIT 4 (REPORT 3E) OR BIT 0 (REPORT 3F) 	14 	0X2000
	//Z ACCELERATION, BIT 5 (REPORT 3E) OR BIT 1 (REPORT 3F) 	15 	0X4000
	// ? UNKNOWN ? 	16 	0X8000
};

/// Values of the wiimote data mode
enum WIIMOTE_MODE_VALUES {
	WIIMOTE_MODE_EXPANSION	= 0x20,		///< expansion mode
	WIIMOTE_MODE_BASIC		= 0x30,		///< basic mode (buttons only)
	WIIMOTE_MODE_ACC		= 0x31,		///< acceleration & buttons
	WIIMOTE_MODE_ACC_IR		= 0x33,		///< acceleration, buttons & IR
	WIIMOTE_MODE_FULL		= 0x3E		///< full mode
};

/// Bitmask of wiimote button
const DWORD WIIMOTE_BUTTONS_MASK =
	WIIMOTE_BUTTON_LEFT | WIIMOTE_BUTTON_RIGHT | WIIMOTE_BUTTON_DOWN | WIIMOTE_BUTTON_UP |
	WIIMOTE_BUTTON_TWO | WIIMOTE_BUTTON_ONE | WIIMOTE_BUTTON_B | WIIMOTE_BUTTON_A |
	WIIMOTE_BUTTON_MINUS | WIIMOTE_BUTTON_PLUS | WIIMOTE_BUTTON_HOME;

union wiimoteButtonState {
	DWORD value;
	struct {
		unsigned two	: 1;
		unsigned one	: 1;
		unsigned b		: 1;
		unsigned a		: 1;
		unsigned minus	: 1;
		unsigned ZACC6	: 1;
		unsigned ZACC7	: 1;
		unsigned home	: 1;
		unsigned left	: 1;
		unsigned right	: 1;
		unsigned down	: 1;
		unsigned up		: 1;
		unsigned plus	: 1;
		unsigned ZACC4	: 1;
		unsigned ZACC5	: 1;
		unsigned unk	: 1;
	} button;
};

/// Button state
typedef wiimoteButtonState WIIMOTE_BUTTON_STATE, *PWIIMOTE_BUTTON_STATE;
/// Acceleration in raw
typedef iVector<BYTE> WIIMOTE_ACCEL_RAW, *PWIIMOTE_ACCEL_RAW;
/// Acceleration in normalize
typedef iVector<FLOAT> WIIMOTE_ACCELEROMETER, *PWIIMOTE_ACCELEROMETER;


/// Class for wiimote input
class wiimoteInput {
public:
	virtual bool processInputMessage(const WPARAM wParam, const LPARAM lParam);

public:
	/// Wiimote stick class
	class wiiStick {
	public:
		wiiStick(void);
		virtual ~wiiStick(void);
		bool init(PWCHAR deviceName);
		bool sendReport(PBYTE data, const DWORD& size);
		bool receiveReport(PBYTE data, DWORD& size);
		bool setMode(const WIIMOTE_MODE_VALUES mode);
		bool setRumble(const bool state);
		bool setLED(const DWORD state);
		void getButtonState(WIIMOTE_BUTTON_STATE& buttonState);
		void getAcceleration(WIIMOTE_ACCEL_RAW& acc);
		void getAcceleration(WIIMOTE_ACCELEROMETER& acc);
		void setThreshold(const DWORD value);
		void getThreshold(DWORD& value);
		void setCalibration(const WIIMOTE_ACCEL_RAW& cal, const WIIMOTE_ACCEL_RAW& g);
		void getCalibration(WIIMOTE_ACCEL_RAW& cal, WIIMOTE_ACCEL_RAW& g);
		void setCompatibleMode(const bool mode);
		void getCompatibleMode(bool& mode);
	private:
		WCHAR					path[WIIMOTE_STICK_PATHNAME_MAX];
		HANDLE					handle;
		PHIDP_PREPARSED_DATA	ppd;
		HIDP_CAPS				caps;

		DWORD					threshold;

		bool					compatibleMode;

		WIIMOTE_BUTTON_STATE	buttons;
		WIIMOTE_ACCEL_RAW		accelerometer;
		WIIMOTE_ACCEL_RAW		calibration;
		WIIMOTE_ACCEL_RAW		gravity;
		bool					rumble;

		bool open(bool hasReadAccess = false, bool hasWriteAccess = false, bool isExclusive = false);
		bool close();
		bool read(PBYTE buffer, DWORD& bytes);
		bool write(PBYTE buffer, DWORD& bytes);
		bool getPath(PWCHAR deviceName, const DWORD size);
		void setButtonState(const WIIMOTE_BUTTON_STATE buttonState);
		void setAcceleration(const WIIMOTE_ACCEL_RAW acc);

		// friend functions
		friend bool wiimoteInput::processInputMessage(const WPARAM wParam, const LPARAM lParam);
	};

public:
	wiimoteInput(void);
	virtual ~wiimoteInput(void);
	bool init(HWND hwnd);
	int getCount(void);
	wiiStick* operator[](UINT index);

private:
	HWND winHandle;
	wiiStick* stick[WIIMOTE_STICK_NUMBER_MAX];
	HANDLE rawHandle[WIIMOTE_STICK_NUMBER_MAX];
	UINT count;

	int findWiimote(void);
	bool rawinputRegister(HWND hwnd);
	int whichOne(HANDLE handle);
	void clear(void);
};

#endif __WIIMOTEINPUT__
