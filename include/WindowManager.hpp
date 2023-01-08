#pragma once

extern "C"
{
#include "X11/Xlib.h"
}

#include "Utils.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>

class WindowManager
{
public:
    static std::unique_ptr<WindowManager> Create(const std::string& displayStr = std::string());
    
    void Run();
    ~WindowManager();

private:
    static int OnXError(Display* display, XErrorEvent* e);
    static int OnWMDetected(Display* display, XErrorEvent* e);
    static bool wmDetected_;
    static std::mutex wmDetectedMutex_;

    void OnCreateNotify(const XCreateWindowEvent& e);
    void OnDestroyNotify(const XDestroyWindowEvent& e);
    void OnReparentNotify(const XReparentEvent& e);
    void OnMapNotify(const XMapEvent& e);
    void OnUnmapNotify(const XUnmapEvent& e);
    void OnConfigureNotify(const XConfigureEvent& e);
    void OnMapRequest(const XMapRequestEvent& e);
    void OnConfigureRequest(const XConfigureRequestEvent& e);
    void OnButtonPress(const XButtonEvent& e);
    void OnButtonRelease(const XButtonEvent& e);
    void OnMotionNotify(const XMotionEvent& e);
    void OnKeyPress(const XKeyEvent& e);
    void OnKeyRelease(const XKeyEvent& e);

    WindowManager(Display* display);
    void Frame(Window w, bool wasCreatedBeforeWindowManager);
    void Unframe(Window w);

    Display* display_;
    const Window root_;
    std::unordered_map<Window, Window> clients_;

    Position<int> dragStartPosition_;
    Position<int> dragStartFramePosition_;
    Size<int> dragStartFrameSize_;

    const Atom WM_PROTOCOLS;
    const Atom WM_DELETE_WINDOW;
};