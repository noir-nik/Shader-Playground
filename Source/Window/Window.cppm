module;

#include <GLFW/glfw3.h>

export module Window;
export import IWindow;
export import WindowCreateInfo;

export struct WindowRect {
	int x, y, width, height;
};

export class Window;

export class Window : public IWindow {
public:
	Window();

	void Init(WindowCreateInfo const& info);

public:
	~Window() override;
	virtual void SetPos(int x, int y) override;
	virtual void SetSize(int width, int height) override;
	virtual void SetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) override;
	virtual void Destroy() override;
	virtual void Minimize() override;
	virtual void Maximize() override;
	virtual void Restore() override;
	virtual void Show() override;
	virtual void Hide() override;
	virtual void SetWindowMode(WindowMode mode) override;
	virtual auto GetWindowMode() const -> WindowMode override;
	virtual bool IsMaximized() const override;
	virtual bool IsMinimized() const override;
	virtual bool GetFullScreenRect(int& x, int& y, int& width, int& height) const override;
	virtual bool GetRestoredRect(int& x, int& y, int& width, int& height) const override;
	virtual void GetRect(int& x, int& y, int& width, int& height) const override;
	virtual void SetFocus() override;
	virtual void SetOpacity(float const value) override;
	virtual void Enable(bool bEnable) override;
	virtual auto GetHandle() const -> void* override;
	virtual void SetText(const char* const text) override;

private:
	GLFWwindow* window = nullptr;
	WindowMode  mode   = WindowMode::eWindowed;

	WindowRect windowedDimensions;
};
