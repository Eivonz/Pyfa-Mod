#pragma once

#include <Windows.h>

class MouseTrackEvents {
	bool m_bMouseTracking;

public:
	MouseTrackEvents() : m_bMouseTracking(false)
	{
	}

	void OnMouseMove(HWND hwnd)
	{
		if (!m_bMouseTracking)
		{
			// Enable mouse tracking.
			TRACKMOUSEEVENT tme = {};
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = hwnd;
			//tme.dwFlags = TME_HOVER | TME_LEAVE | TME_CANCEL;
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
			m_bMouseTracking = true;
		}
	}
	void Reset(HWND hwnd)
	{
		m_bMouseTracking = false;
	}
	UINT GetMouseHoverTime()
	{
		UINT msec;
		if (SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &msec, 0))
		{
			return msec;
		}
		else
		{
			return 0;
		}
	}
	BOOL IsMouseWheelPresent()
	{
		return (GetSystemMetrics(SM_MOUSEWHEELPRESENT) != 0);
	}
};