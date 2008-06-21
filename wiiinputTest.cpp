// Name:        wiiinputTest.cpp
// Purpose:     Wiiinput Test
// Author:      Shun Cox
// $Id: wiiinputTest.cpp,v 1.2 2007/01/26 13:19:15 zhangshun Exp $

#include <wx/wx.h>
#include "wiimoteInput.h"
#include "wiiinputTest.h"

IMPLEMENT_APP(wiiinputTest)


bool wiiinputTest::OnInit()
{
    // Create the main application window
    wtFrame *frame = new wtFrame(_T("Wiiinput Test"));

    // Show it
    frame->Show(true);

	// Start the event loop
    return true;
}

int wiiinputTest::OnExit()
{
	return 0;
}


// Event table for wtFrame
BEGIN_EVENT_TABLE(wtFrame, wxFrame)
    EVT_MENU(wxID_ABOUT, wtFrame::OnAbout)
    EVT_MENU(wxID_EXIT,  wtFrame::OnQuit)
END_EVENT_TABLE()

wtFrame::wtFrame(const wxString& title)
: wxFrame(NULL, wxID_ANY, title)
{
	createControls();
	initWiiinput();
}

void wtFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox(_T("Wiiinput Test 1.0"), _T("About Wiiinput Test"),
		wxOK | wxICON_INFORMATION, this);
}

void wtFrame::OnQuit(wxCommandEvent& event)
{
    // Destroy the frame
    Close();
}

// icon file
#include "wiiinputTest.xpm"

void wtFrame::createControls(void)
{
    // Set the frame icon
    SetIcon(wxIcon(wiiinputTest_xpm));

    // Create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // The "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(wxID_ABOUT, wxT("&About...\tF1"),
		wxT("About Wiiinput Test"));

	fileMenu->Append(wxID_EXIT, wxT("E&xit\tAlt-X"),
		wxT("Quit Wiiinput Test"));

    // Now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, wxT("&File"));
    menuBar->Append(helpMenu, wxT("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    // Create a status bar just for fun
    CreateStatusBar();
    SetStatusText(wxT("Welcome to Wiiinput Test!"));

	// Create a text control
	txtInfo = new wxTextCtrl(this, -1, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
}

void wtFrame::initWiiinput()
{
	if (!wiiInput.init(static_cast<HWND>(GetHandle()))) {
		txtInfo->AppendText(wxT("wiimoteInput initialization faild."));
		return;
	}

	wiiInput[0]->setThreshold(1);
	wiiInput[0]->setMode(WIIMOTE_MODE_ACC);
}

WXLRESULT wtFrame::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
	wxString str;
	UINT index = wParam;
	WIIMOTE_ACCELEROMETER acc;

	switch (nMsg) {
	case WM_WIIMOTE_BUTTON:
		*txtInfo << _T("Stick[") << (long)wParam << _T("] = ") << (long)lParam << _T("\n");
		break;
	case WM_WIIMOTE_ACCELER:
		wiiInput[index]->getAcceleration(acc);
		str.Printf(_T("Xacc = %+1.4f, Yacc = %+1.4f, Zacc = %+1.4f"), acc.x, acc.y, acc.z);
		SetStatusText(str);
		break;
	case WM_INPUT:
		wiiInput.processInputMessage(wParam, lParam);
		break;
	}
    return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}
