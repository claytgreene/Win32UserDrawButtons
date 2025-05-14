#pragma once

class MouseTrackEvents
{
public:
	MouseTrackEvents() = default;
	~MouseTrackEvents()
	{
	}
	void OnMouseMove(HWND hwnd);
	void Reset(HWND hwnd);
};
