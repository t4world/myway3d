#include "MWDIMouse.h"
#include "Engine.h"

namespace Myway
{

DIMouse::DIMouse(LPDIRECTINPUT8 input, HWND hWnd)
: m_positionLocal(0, 0),
  m_positionGlobal(0, 0),
  m_positionUnit(0, 0),
  m_oldPositionLocal(0, 0),
  m_oldPositionGlobal(0, 0),
  m_oldPositionUnit(0, 0)
{
	m_bActive = false;
	m_device = NULL;
	m_hWnd = NULL;
	ZeroMemory(&m_mouseState, sizeof(DIMOUSESTATE));
	ZeroMemory(&m_oldMouseState, sizeof(DIMOUSESTATE));

	DWORD flags;

	if (input->CreateDevice(GUID_SysMouse, & m_device, NULL) == DI_OK)
	{
		if (m_device->SetDataFormat(&c_dfDIMouse) == DI_OK)
		{
			/*
			if (exclusive)
				flags = DISCL_FOREGROUND | DISCL_EXCLUSIVE | DISCL_NOWINKEY;
			else
				flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;
			*/

			flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

			if (FAILED(m_device->SetCooperativeLevel(hWnd, flags)))
				m_device->Acquire();

			m_bActive = true;
		}
	}

	m_hWnd = hWnd;
}

DIMouse::~DIMouse()
{
	if (m_device)
	{
		m_device->Unacquire();
		m_device->Release();
		m_device = NULL;
	}
}

bool DIMouse::KeyUp(MouseKeyCode key)
{
	if (!IsActive())
		return false;

	const BYTE * cur = m_mouseState.rgbButtons;
	const BYTE * old = m_oldMouseState.rgbButtons;

	return ((cur[key] & 0x80) != 0x80) && (cur[key] != old[key]);
}

bool DIMouse::KeyDown(MouseKeyCode key)
{
	if (!IsActive())
		return false;

	const BYTE * cur = m_mouseState.rgbButtons;
	const BYTE * old = m_oldMouseState.rgbButtons;

	return ((cur[key] & 0x80) == 0x80) && (cur[key] != old[key]);
}

bool DIMouse::KeyPressed(MouseKeyCode key)
{
	if (!IsActive())
		return false;

	const BYTE * cur = m_mouseState.rgbButtons;
	const BYTE * old = m_oldMouseState.rgbButtons;

	return ((cur[key] & 0x80) == 0x80) && (cur[key] == old[key]);
}

bool DIMouse::MouseMoved()
{
	if (!IsActive())
		return false;

    RECT rt;
    ::GetWindowRect(m_hWnd, &rt);

    if (m_positionGlobal.x >= rt.left && m_positionGlobal.x <= rt.right &&
        m_positionGlobal.y >= rt.top && m_positionGlobal.y <= rt.bottom)
    {
        return m_positionGlobal.x != m_oldPositionGlobal.x ||
               m_positionGlobal.y != m_oldPositionGlobal.y;
    }

    return false;
}

int DIMouse::MouseWheel()
{
	if (!IsActive())
		return 0;

	return m_mouseState.lZ;
}

Point2i DIMouse::GetPosition()
{
	return m_positionLocal;
}

Point2i DIMouse::GetPositionGlobal()
{
	return m_positionGlobal;
}

Point2f DIMouse::GetPositionUnit()
{
	return m_positionUnit;
}

Point2i DIMouse::GetOldPosition()
{
    return m_oldPositionLocal;
}

Point2i DIMouse::GetOldPositionGlobal()
{
    return m_oldPositionGlobal;
}

Point2f DIMouse::GetOldPositionUnit()
{
    return m_oldPositionUnit;
}

Point2i DIMouse::GetPositionDiff()
{
    Point2i diff;

    diff.x = m_positionLocal.x - m_oldPositionLocal.x;
    diff.y = m_positionLocal.y - m_oldPositionLocal.y;

    return diff;
}

Point2f DIMouse::GetPositionDiffUnit()
{
	Point2f diff;

	diff.x = m_positionUnit.x - m_oldPositionUnit.x;
	diff.y = m_positionUnit.y - m_oldPositionUnit.y;

	return diff;
}

bool DIMouse::IsActive()
{
	return m_bActive;
}

void DIMouse::Update()
{
	if (m_device)
	{
		//save old state for input comparing
		Memcpy(&m_oldMouseState, &m_mouseState, sizeof(m_mouseState));

		HRESULT hr = S_OK;

		//if error getting device state, re-aquire
		hr = m_device->GetDeviceState(sizeof(DIMOUSESTATE), &m_mouseState);

		if (FAILED(hr))
		{
			hr = m_device->Acquire();

			if (SUCCEEDED(hr))
				hr = m_device->GetDeviceState(sizeof(DIMOUSESTATE), &m_mouseState);
		}

		m_bActive = SUCCEEDED(hr);

		POINT pt;
		POINT ptLocal;
		::GetCursorPos(&pt);

		ptLocal = pt;

		::ScreenToClient(m_hWnd, &ptLocal);

		int w = Engine::Instance()->GetDeviceProperty()->Width;
		int h = Engine::Instance()->GetDeviceProperty()->Height;

        m_oldPositionLocal = m_positionLocal;
        m_oldPositionGlobal = m_positionGlobal;
        m_oldPositionUnit = m_positionUnit;

		m_positionGlobal.x = pt.x;
		m_positionGlobal.y = pt.y;
		m_positionLocal.x = ptLocal.x;
		m_positionLocal.y = ptLocal.y;
		m_positionUnit.x = (float)ptLocal.x / w;
		m_positionUnit.y = (float)ptLocal.y / h;
	}
}

}