// Name:        wiiinputTest.h
// Purpose:     Wiiinput Test
// Author:      Shun Cox
// $Id: wiiinputTest.h,v 1.1 2007/01/24 16:30:10 zhangshun Exp $

#ifndef __RAWINPUTTEST__
#define __RAWINPUTTEST__

#include "wiimoteinput.h"

// Declare the application class
class wiiinputTest : public wxApp
{
public:
    // Called on application startup
    virtual bool OnInit();
    virtual int OnExit();
};

DECLARE_APP(wiiinputTest)

// Declare our main frame class
class wtFrame : public wxFrame
{
public:
    // Constructor
    wtFrame(const wxString& title);

    // Event handlers
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
	virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);

private:
	// Controls
	wxTextCtrl* txtInfo;
	wiimoteInput wiiInput;

	void createControls(void);
	void initWiiinput(void);

	// This class handles events
    DECLARE_EVENT_TABLE()
};

#endif // __RAWINPUTTEST__
