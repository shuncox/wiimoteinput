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
// $Id: wiiStick.cpp,v 1.7 2007/01/26 13:18:39 zhangshun Exp $
//
////////////////////////////////////////////////////////////////////////////

#include "idebug.hpp"
#include "isafe.hpp"
#include "wiimoteInput.h"

////////////////////////////////////////////////////////////////////////////
//! Contructor
////////////////////////////////////////////////////////////////////////////
wiimoteInput::wiiStick::wiiStick(void)
: handle(INVALID_HANDLE_VALUE), ppd(NULL)
, threshold(0)
, compatibleMode(false)
, rumble(false)
{
	memset(path, 0, sizeof(path));
	memset(&caps, 0, sizeof(caps));
}

////////////////////////////////////////////////////////////////////////////
//! Destructor
////////////////////////////////////////////////////////////////////////////
wiimoteInput::wiiStick::~wiiStick(void)
{
	if (NULL != ppd) {
		HidD_FreePreparsedData(ppd);
		ppd = NULL;
	}
	ICLOSE_HANDLE(handle);
	// rawHandle needn't to be released.
}

////////////////////////////////////////////////////////////////////////////
//! Initialization
///
/// @param deviceName path to the wiimote device
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::init(PWCHAR deviceName)
{
	BYTE buffer[64];
	DWORD size;

	if (NULL != wcscpy_s(path, WIIMOTE_STICK_PATHNAME_MAX, deviceName)) {
		ILOG("String operation error.");
		false;
	}

	if (!open()) {
		ILOG("Open device failed.");
		memset(path, 0, sizeof(path));
		return false;
	}

	if (!HidD_GetPreparsedData(handle, &ppd)) {
		ILOG("HidD_GetPreparsedData failed.");
		close();
		return false;
	}

	if (!HidP_GetCaps(ppd, &caps)) {
		ILOG("HidP_GetCaps failed.");
		HidD_FreePreparsedData(ppd);
		ppd = NULL;
		close();
		return false;
	}

	ILOG("HIDP_CAPS.Usage                   " << caps.Usage);
	ILOG("HIDP_CAPS.UsagePage               " << caps.UsagePage);
	ILOG("HIDP_CAPS.InputReportByteLength   " << caps.InputReportByteLength);
	ILOG("HIDP_CAPS.OutputReportByteLength  " << caps.OutputReportByteLength);
	ILOG("HIDP_CAPS.FeatureReportByteLength " << caps.FeatureReportByteLength);

	// get factory calibration data for on-board flash memory
	// EXPERIMENT !!!
	static BYTE report[] = {0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	size = sizeof(report);
	report[1] = rumble ? 1 : 0;		// rumble bit
	report[3] = 0x16;				// address starts from 0x16
	report[5] = 0x08;				// size of calibration data
	sendReport(report, size);
	size = sizeof(buffer);
	memset(buffer, 0, sizeof(buffer));
	if (receiveReport(buffer, size) && ((buffer[0] == 21) && ((buffer[2] & 0x04) == 0))) {
		// success
		calibration.x = buffer[5];
		calibration.y = buffer[6];
		calibration.z = buffer[7];
		gravity.x = buffer[9];
		gravity.y = buffer[10];
		gravity.z = buffer[11];
		ILOG("I gotta the factory calibration data.");
	} else {
		// failure
		calibration.x = calibration.y = calibration.z = 128;
		gravity.x = gravity.y = gravity.z = 30;
		ILOG("I cannot get the factory calibration data.");
	}

	// get initial value of acceleration & buttons
	setMode(WIIMOTE_MODE_ACC);
	size = sizeof(buffer);
	memset(buffer, 0, sizeof(buffer));
	if (receiveReport(buffer, size) && (buffer[0] == WIIMOTE_MODE_ACC)) {
		// success
		buttons.value = ((static_cast<DWORD>(buffer[1]) << 8) | buffer[2]) & WIIMOTE_BUTTONS_MASK;
		accelerometer.x = buffer[3];
		accelerometer.y = buffer[4];
		accelerometer.z = buffer[5];
		ILOG("I gotta the initial state of accelerometer & button.");
	} else {
		ILOG("I cannot get the initial state of accelerometer & button.");
	}

	close();

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Send a report to wiimote
///
/// @param data pointer to the data
/// @param size size of the data in byte
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::sendReport(PBYTE data, const DWORD& size)
{
	DWORD length = caps.OutputReportByteLength;

	if (size > length) {
		ILOG("Too many data to be sent.");
		return false;
	}

	PBYTE buf = new BYTE[length];
	if (!open(false, true)) {
		ILOG("Open device failed.");
		delete buf;
		return false;
	}
	memset(buf, 0, length);
	memcpy(buf, data, size);
	if (!write(buf, length)) {
		ILOG("Write device failed.");
		close();
		delete buf;
		return false;
	}
	close();
	delete buf;

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Receive a report from wiimote
///
/// @param data pointer to the data
/// @param size size of the data received in byte
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::receiveReport(PBYTE data, DWORD& size)
{
	DWORD length = caps.InputReportByteLength;

	if (size < length) {
		ILOG("Buffer is too small.");
		return false;
	}

	PBYTE buf = new BYTE[length];
	if (!open(true, false)) {
		ILOG("Open device failed.");
		delete buf;
		return false;
	}
	if (read(buf, length)) {
		memset(data, 0, size);
		memcpy(data, buf, length);
		size = length;
	} else {
		ILOG("Read device failed.");
		close();
		delete buf;
		return false;
	}
	close();
	delete buf;

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Open the wiimote device
///
/// @param hasReadAccess privilege to read
/// @param hasWriteAccess privilege to write
/// @param isExclusive exclusive access
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::open(bool hasReadAccess, bool hasWriteAccess, bool isExclusive)
{
    DWORD   accessFlags = 0;
    DWORD   sharingFlags = 0;

	//if (INVALID_HANDLE_VALUE != handle) {
	//	return true;
	//}

    if (hasReadAccess) {
        accessFlags |= GENERIC_READ;
    }

    if (hasWriteAccess) {
        accessFlags |= GENERIC_WRITE;
    }

    if (!isExclusive) {
        sharingFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

	handle = CreateFile(path,
	//handle = CreateFile(
	//	_T("\\\\?\\hid#bthidjoystk#2&24779bbb&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}"),
		accessFlags,
		sharingFlags,
		NULL,
		OPEN_EXISTING,	// No special create flags
		0,				// Open device as non-overlapped so we can get data
		NULL);			// No template file

    if (INVALID_HANDLE_VALUE == handle) {
        return false;
    }

	//ILOG("Open device successful.");

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Close the wiimote device
///
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::close()
{
	ICLOSE_HANDLE(handle);
	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Read from wiimote
///
/// @param data pointer to the buffer
/// @param size bytes of the data
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::read(PBYTE buffer, DWORD& bytes)
{
	if (INVALID_HANDLE_VALUE == handle) {
		return false;
	}

	if (compatibleMode) {
		// use HidD_GetInputReport()
		if (!HidD_GetInputReport(handle, buffer, bytes)) {
			ILOG("HidD_GetInputReport failed.");
			return false;
		}
	} else {
		// use ReadFile()
		if (!ReadFile(handle, buffer, bytes, &bytes, NULL)) {
			ILOG("Read file failed. error = " << GetLastError());
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Write to wiimote
///
/// @param data pointer to the buffer
/// @param size bytes of the data
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::write(PBYTE buffer, DWORD& bytes)
{
	if (INVALID_HANDLE_VALUE == handle) {
		return false;
	}

	if (compatibleMode) {
		//use HidD_SetOutputReport
		if (!HidD_SetOutputReport(handle, buffer, bytes)) {
			ILOG("HidD_SetOutputReport failed.");
			return false;
		}
	} else {
		// use WriteFile()
		if (!WriteFile(handle, buffer, bytes, &bytes, NULL)) {
			ILOG("Write file failed. error = " << GetLastError());
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Get the path to wiimote device
///
/// @param deviceName pointer to the buffer
/// @param count max count of characters
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::getPath(PWCHAR deviceName, const DWORD count)
{
	if (count < wcslen(path) + 1) {
		ILOG("String buffer is too small.");
		return false;
	}
	return (NULL == wcscpy_s(deviceName, count, path));
}

////////////////////////////////////////////////////////////////////////////
//! Set the state of buttons
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::setButtonState(const WIIMOTE_BUTTON_STATE buttonState)
{
	buttons = buttonState;
}

////////////////////////////////////////////////////////////////////////////
//! Get the state of buttons
///
/// @param buttonState state of the buttons
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::getButtonState(WIIMOTE_BUTTON_STATE& buttonState)
{
	buttonState = buttons;
}

////////////////////////////////////////////////////////////////////////////
//! Set acceleration
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::setAcceleration(const WIIMOTE_ACCEL_RAW acc)
{
	accelerometer = acc;
}

////////////////////////////////////////////////////////////////////////////
//! Get acceleration
///
/// @param acc acceleration of three-axis in raw
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::getAcceleration(WIIMOTE_ACCEL_RAW& acc)
{
	acc = accelerometer;
}

////////////////////////////////////////////////////////////////////////////
//! Get acceleration
///
/// @param acc acceleration of three-axis in normalize.
/// Value 1.0F will be returned if acceleration equals to +1g.
/// The range is approxmiately from +3.0 to -3.0 according to hardware
/// specifications of the accelerometer used in wiimote.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::getAcceleration(WIIMOTE_ACCELEROMETER& acc)
{
	acc.x = (static_cast<FLOAT>(accelerometer.x) - calibration.x) / gravity.x;
	acc.y = (static_cast<FLOAT>(accelerometer.y) - calibration.y) / gravity.y;
	acc.z = (static_cast<FLOAT>(accelerometer.z) - calibration.z) / gravity.z;
}

////////////////////////////////////////////////////////////////////////////
//! Set report data mode
///
/// @param mode report data mode
/// @return true for success, false for failure
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::setMode(const WIIMOTE_MODE_VALUES mode)
{
	static BYTE report[] = {0x12, 0x00, 0x00};
	DWORD size = sizeof(report);
	report[2] = mode;
	return sendReport(report, size);
}

////////////////////////////////////////////////////////////////////////////
//! Set threshold
///
/// Tweak the threshold of acceleration if more stable values were desired.
///
/// @param value threshold. The bigger the stable.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::setThreshold(const DWORD value)
{
	threshold = value;
}

////////////////////////////////////////////////////////////////////////////
//! Get threshold
///
/// @param value threshold.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::getThreshold(DWORD& value)
{
	value = threshold;
}

////////////////////////////////////////////////////////////////////////////
//! Set calibration data
///
/// @param cal center of three-axis in raw.
/// @param g +1g offset of three-axis in raw.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::setCalibration(const WIIMOTE_ACCEL_RAW& cal, const WIIMOTE_ACCEL_RAW& g)
{
	calibration = cal;
	gravity = g;
}

////////////////////////////////////////////////////////////////////////////
//! Get calibration data
///
/// @param cal center of three-axis in raw.
/// @param g +1g offset of three-axis in raw.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::getCalibration(WIIMOTE_ACCEL_RAW& cal, WIIMOTE_ACCEL_RAW& g)
{
	cal = calibration;
	g = gravity;
}

////////////////////////////////////////////////////////////////////////////
//! Set compatible mode
///
/// You can turn on compatible mode if microsoft bluetooth stack was working
/// unproperly.
///
/// @param mode true for on, false for off.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::setCompatibleMode(const bool mode)
{
	compatibleMode = mode; 
}

////////////////////////////////////////////////////////////////////////////
//! Get compatible mode
///
/// @param mode true for on, false for off.
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::wiiStick::getCompatibleMode(bool& mode)
{
	mode = compatibleMode; 
}

////////////////////////////////////////////////////////////////////////////
//! Set rumble state
///
/// @param state true for on, false for off.
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::setRumble(const bool state)
{
	static BYTE report[] = {0x13, 0x00};
	DWORD size = sizeof(report);
	rumble = state;
	report[1] = rumble ? 1 : 0;
	return sendReport(report, size);
}

////////////////////////////////////////////////////////////////////////////
//! Set LED state
///
/// @param state 4 low bits control the LEDs of wiimote.
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::wiiStick::setLED(const DWORD state)
{
	static BYTE report[] = {0x11, 0x00};
	DWORD size = sizeof(report);
	report[1] = static_cast<BYTE>(state << 4) | (rumble ? 1 : 0);
	return sendReport(report, size);
}
