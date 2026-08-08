#pragma once

/* SDL3-backed replacement for DirectXTK12's DirectX::Mouse.
   Field names match DirectXTK so MouseController ports unchanged. */
class Mouse
{
public:
	struct State
	{
		int x = 0;
		int y = 0;

		bool leftButton = false;
		bool middleButton = false;
		bool rightButton = false;
		bool xButton1 = false;
		bool xButton2 = false;

		/* DirectXTK accumulated wheel movement rather than reporting a delta,
		   and MouseController diffs it, so the running total is kept here. */
		int scrollWheelValue = 0;
	};

	Mouse();
	~Mouse();

	Mouse(const Mouse&) = delete;
	Mouse& operator=(const Mouse&) = delete;

	static Mouse& Get();

	State GetState() const;

	/* SDL has no notion of a mouse being absent. */
	bool IsConnected() const { return true; }

	/* DirectXTK needed the HWND to track the cursor; SDL resolves the focused
	   window itself, so this only records the handle. */
	void SetWindow(void* pWindow) { m_pWindow = pWindow; }

	/* Called by the window layer when SDL reports a wheel event. */
	void AddScrollDelta(float fDelta);

private:
	void* m_pWindow = nullptr;
	int m_iScrollWheelValue = 0;

	static Mouse* s_pInstance;
};
