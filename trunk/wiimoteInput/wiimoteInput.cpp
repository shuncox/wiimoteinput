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
// $Id: wiimoteInput.cpp,v 1.8 2008/06/18 14:49:14 zhangshun Exp $
//
////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <string.h>

#include "idebug.hpp"
#include "isafe.hpp"
#include "wiimoteInput.h"

////////////////////////////////////////////////////////////////////////////
//! Contructor
////////////////////////////////////////////////////////////////////////////
wiimoteInput::wiimoteInput(void)
: count(0), winHandle(NULL)
{
	memset(stick, 0, sizeof(stick));
	memset(rawHandle, 0, sizeof(rawHandle));
}

////////////////////////////////////////////////////////////////////////////
//! Destructor
////////////////////////////////////////////////////////////////////////////
wiimoteInput::~wiimoteInput(void)
{
	clear();
}

////////////////////////////////////////////////////////////////////////////
//! Initialization
///
/// Looking for wiimote devices and registering them via Raw Input API.
/// It should be done before doing any other operation.
///
/// @param hwnd handle to the target window.
/// @return true for success, false for failure.
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::init(HWND hwnd)
{
	if (!findWiimote()) {
		ILOG("Cannot found wiimote!");
		return false;
	}

	ILOG("Found " << count << " wiimote(s).");

	if (!rawinputRegister(hwnd)) {
		return false;
	}

	winHandle = hwnd;

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Find wiimote device
////////////////////////////////////////////////////////////////////////////
int wiimoteInput::findWiimote(void)
{
	UINT nDevices;
	PRAWINPUTDEVICELIST pRawInputDeviceList;

	if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
		ILOG("GetRawInputDeviceList failed.");
		return (-1);
	}
	if ((pRawInputDeviceList = new RAWINPUTDEVICELIST[nDevices]) == NULL) {
		ILOG("Allocate memory failed.");
		return (-1);
	}
	if (GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == -1) {
		ILOG("GetRawInputDeviceList(2) failed.");
		return (-1);
	}
	
	WCHAR pathName[WIIMOTE_STICK_PATHNAME_MAX];
	PRAWINPUTDEVICELIST pDev;
	RID_DEVICE_INFO info;
	UINT bufSize;

	wiiStick* pws = NULL;

	for (unsigned int i = 0; i < nDevices; i ++) {

		pDev = &pRawInputDeviceList[i];
		bufSize = sizeof(info);
		info.cbSize = sizeof(info);

		if (GetRawInputDeviceInfo(pDev->hDevice, RIDI_DEVICEINFO, &info, &bufSize) > 0) {

			// is general hid device ?
			if (RIM_TYPEHID == info.dwType) {

				// is wiimote ?
				if (
					(WIIMOTE_VENDOR_ID == info.hid.dwVendorId) &&
					(WIIMOTE_PRODUCT_ID == info.hid.dwProductId) &&
					(WIIMOTE_USAGE_PAGE == info.hid.usUsagePage) &&
					(WIIMOTE_USAGE == info.hid.usUsage)
					) {

					bufSize = WIIMOTE_STICK_PATHNAME_MAX;
					if (GetRawInputDeviceInfo(
						pDev->hDevice, RIDI_DEVICENAME, pathName, &bufSize) > 0) {


						///////////////////////////////////////////////////
						// HOTFIX !!!
						// The 2nd char of device name is mistaked by '?'
						// in returns of GetRawInputDeviceInfo()
						///////////////////////////////////////////////////
						pathName[1] = '\\';
						///////////////////////////////////////////////////
						wchar_t *wiimote_id = L"\\\\?\\HID#BTHIDJOYSTK#2";
						if (wcsncmp(pathName, wiimote_id, wcslen(wiimote_id)) != 0) {
							// NOT WIIMOTE
							ILOG("Not wiimote.");
						} else {

							INEW_SOLO(pws, wiiStick);

							if ((count < WIIMOTE_STICK_NUMBER_MAX) && pws->init(pathName)) {
								stick[count]		= pws;				// wiiStick object
								rawHandle[count]	= pDev->hDevice;	// raw handle
								count ++;
							} else {
								IDELETE_SOLO(pws);
							}

						}

					} else {
						ILOG("GetRawInputDeviceInfo failed.");
						delete []pRawInputDeviceList;
						return (-1);
					}

				}
				// wiimote
			}
			// hid
		}

	}

	// after the job, free the RAWINPUTDEVICELIST
	delete []pRawInputDeviceList;

	return (count);
}

////////////////////////////////////////////////////////////////////////////
//! Register the device
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::rawinputRegister(HWND hwnd)
{
	RAWINPUTDEVICE Rid;
	Rid.usUsagePage = WIIMOTE_USAGE_PAGE;	// Top level collection Usage page for wiimote.
	Rid.usUsage = WIIMOTE_USAGE;				// Top level collection Usage for the wiimote.
	Rid.dwFlags = RIDEV_INPUTSINK;				// Mode flag that specifies how to interpret the information provided by usUsagePage and usUsage.
	Rid.hwndTarget = hwnd;

	if (!RegisterRawInputDevices(&Rid, 1, sizeof(Rid))) {
		ILOG("Rawinput registration failed. error = " << GetLastError());
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Clear the device list
////////////////////////////////////////////////////////////////////////////
void wiimoteInput::clear(void)
{
	if (0 == count) return;
	for (unsigned int i = 0; i < count; i ++) {
		IDELETE_SOLO(stick[i]);
	}
	count = 0;
	winHandle = NULL;
	memset(stick, 0, sizeof(stick));
	memset(rawHandle, 0, sizeof(rawHandle));
}

////////////////////////////////////////////////////////////////////////////
//! Which one is the current device
////////////////////////////////////////////////////////////////////////////
int wiimoteInput::whichOne(HANDLE handle)
{
	for (unsigned int i = 0; i < count; i ++) {
		if (rawHandle[i] == handle) {
			return (i);
		}
	}
	return (-1);
}

////////////////////////////////////////////////////////////////////////////
//! Get the wiiStick object
///
/// A wiiStick stands for a wiimote.
///
/// @param index index of the wiimote. it must be less than the number
/// of wiimote.
/// @return pointer to the wiiStick object
////////////////////////////////////////////////////////////////////////////
wiimoteInput::wiiStick* wiimoteInput::operator[](UINT index)
{
	if (index >= count)	{
		ILOG("Bad index of Wiimote.");
		return NULL;
	}
	return stick[index];
}

////////////////////////////////////////////////////////////////////////////
//! Process input messages
///
/// This function must be invoked in the message processing loop. It will
/// process the Raw Input messages and post messages of specific events.
///
/// @param wParam specifies additional message-specific information.
/// @param lParam specifies additional message-specific information.
/// @return true for success, false for failure.
////////////////////////////////////////////////////////////////////////////
bool wiimoteInput::processInputMessage(const WPARAM wParam, const LPARAM lParam)
{
	const UINT inputBufferSize = 512;
	static BYTE inBuffer[inputBufferSize];	// use static buffer to improve the performance
	UINT size = inputBufferSize;
	INT index;

	// get input code
	// return value: RIM_INPUT or RIM_INPUTSINK

	WPARAM inputCode = GET_RAWINPUT_CODE_WPARAM(wParam);
	if ((inputCode != RIM_INPUT) && (inputCode != RIM_INPUTSINK)) {
		ILOG("This message is noting to do with me.");
		return false;
	}

	// get input data (unbuffered)
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, inBuffer, &size, 
		sizeof(RAWINPUTHEADER));

	RAWINPUT* raw = (RAWINPUT*)inBuffer;

	if (RIM_TYPEHID != raw->header.dwType) {
		ILOG("This HID device is not my type.");
		return false;
	}

	if ((index = whichOne(raw->header.hDevice)) == -1) {
		ILOG("I don't known what is it at all.");
		return false;
	}

	BYTE *data = raw->data.hid.bRawData;
	// which button is pressed?
	if (
		WIIMOTE_MODE_BASIC	== data[0] ||
		WIIMOTE_MODE_ACC	== data[0] ||
		WIIMOTE_MODE_ACC_IR	== data[0]
	) {
		static WIIMOTE_BUTTON_STATE buttonValue, oldValue;
		buttonValue.value = ((static_cast<DWORD>(data[1]) << 8) | data[2]) & WIIMOTE_BUTTONS_MASK;
		oldValue.value = stick[index]->buttons.value;
		if (buttonValue.value ^ oldValue.value) {
			stick[index]->setButtonState(buttonValue);
			PostMessage(winHandle, WM_WIIMOTE_BUTTON, index, buttonValue.value);
		}
	}

	// accelerometer
	if (
		WIIMOTE_MODE_ACC	== data[0] ||
		WIIMOTE_MODE_ACC_IR	== data[0]
	) {
		static WIIMOTE_ACCEL_RAW acc, old;
		static DWORD threshold;
		acc.x = data[3];
		acc.y = data[4];
		acc.z = data[5];
		stick[index]->getThreshold(threshold);
		if (
			(0 == threshold) ||
			((abs(static_cast<int>(old.x) - static_cast<int>(acc.x)) > static_cast<int>(threshold)) ||
			(abs(static_cast<int>(old.y) - static_cast<int>(acc.y)) > static_cast<int>(threshold)) ||
			(abs(static_cast<int>(old.z) - static_cast<int>(acc.z)) > static_cast<int>(threshold)))
			){

			stick[index]->setAcceleration(acc);
			PostMessage(winHandle, WM_WIIMOTE_ACCELER, index, NULL);
			old = acc;
		}
	}

	// IR
	if (WIIMOTE_MODE_ACC_IR	== data[0]) {

		//// right side
		// IR1 = data[6];
		// IR2 = data[7];
		// IR3 = data[8];

		//// left side
		// IR4 = data[9];
		// IR5 = data[10];
		// IR6 = data[11];
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////
//! Get the number of wiimote
///
/// The total number of wiimote device will be returned.
///
/// @return the number of wiimote
////////////////////////////////////////////////////////////////////////////
int wiimoteInput::getCount(void)
{
	return count;
}
