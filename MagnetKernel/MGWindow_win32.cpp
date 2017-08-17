#include "Platform.h"
#include "MGWindow.h"
#include "MGRenderer.h"

#if VK_USE_PLATFORM_WIN32_KHR

LRESULT CALLBACK WindowsEventHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MGWindow * window = reinterpret_cast<MGWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (uMsg)
	{
	case WM_CLOSE:
		//window->Close();
		window->window_should_run = false;
		return 0;
	case WM_SIZE:
		window->OnResize(LOWORD(lParam), HIWORD(lParam));
		//此处为窗口改变函数//先改变OsWindow大小，再改变VulkanSurface，Image，ViewPort，ortho/perspective
		return 0;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

uint64_t MGWindow::_win32_class_id_counter = 0;

MGWindow::MGWindow(std::string title, int w, int h)
{
	Width = w;
	Hight = h;
	surfaceValid = false;
	RelatingRenderer = nullptr;

	WNDCLASSEX win_class{};
	//assert(_surface_size_x > 0);
	//assert(_surface_size_y > 0);

	_win32_instance = GetModuleHandle(nullptr);
	_win32_class_name = title + "_" + std::to_string(_win32_class_id_counter);

	//初始化Windows窗口类结构体
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WindowsEventHandler;
	win_class.cbClsExtra = 0;
	win_class.cbWndExtra = 0;
	win_class.hInstance = _win32_instance;
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = _win32_class_name.c_str();
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	//注册窗口类
	if (!RegisterClassEx(&win_class))
	{
		//assert(0 && "Cannot create a Windows window for drawing!\n");
		//fflush(stdout);
		std::exit(-1);
	}

	DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD style = WS_OVERLAPPEDWINDOW;

	//用注册的窗口类创建窗口
	RECT wr = { 0,0,LONG(Width),LONG(Hight) };
	AdjustWindowRectEx(&wr, style, FALSE, ex_style);
	_win32_window = CreateWindowEx(
		0,
		_win32_class_name.c_str(),//class name
		title.c_str(),//app name
		style,//window style
		CW_USEDEFAULT, CW_USEDEFAULT,//x/y coords
		wr.right - wr.left,//width
		wr.bottom - wr.top,//height
		NULL,//handle to parent
		NULL,//handle to menu
		_win32_instance,//hInstance
		NULL//no extra parameters
	);
	if (!_win32_window)
	{
		//无效窗口，给出error
		//assert(1 && "Cannot create a window in which to draw!\n");
		//fflush(stdout);
		std::exit(-1);
	}
	SetWindowLongPtr(_win32_window, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(_win32_window, SW_SHOW);
	SetForegroundWindow(_win32_window);
	SetFocus(_win32_window);
}

MGWindow::~MGWindow(){}

void MGWindow::releaseWindow()
{
	DestroyWindow(_win32_window);
	UnregisterClass(_win32_class_name.c_str(), _win32_instance);
}

VkResult MGWindow::createWindowSurface(VkInstance instance) {

	VkWin32SurfaceCreateInfoKHR surface_create_info{};
	surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create_info.hinstance = _win32_instance;
	surface_create_info.hwnd = _win32_window;
	VkResult result = vkCreateWin32SurfaceKHR(instance, &surface_create_info, nullptr, &windowSurface);
	if (result == VK_SUCCESS) {
		surfaceValid = true;
	}
	return result;
}

bool MGWindow::getWindowSurface(VkSurfaceKHR* surface) {
	if (surfaceValid) {
		*surface = windowSurface;
	}
	return surfaceValid;
}

std::vector<const char*> MGWindow::getSurfaceExtensions()
{
	std::vector<const char*> result;
	result.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	result.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	return result;
}

void MGWindow::UpdateWindow() {
	MSG msg;
	if (PeekMessage(&msg, _win32_window, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

bool MGWindow::ShouldRun() {
	return window_should_run;
}

void MGWindow::OnResize(int w, int h) {
	Width = w;
	Hight = h;
	if (RelatingRenderer)
	{
		RelatingRenderer->OnWindowResized();
	}
	//std::cout << w << "\\" << h << std::endl;
	//RECT rect;
	//if (GetWindowRect(_win32_window, &rect))
	//{
	//	int width = rect.right - rect.left;
	//	int height = rect.bottom - rect.top;
	//	std::cout << width << "/" << height << std::endl;
	//}
}

#endif