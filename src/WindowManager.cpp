extern "C"
{
#include "X11/Xutil.h"
}

#include "WindowManager.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

bool WindowManager::wmDetected_;
std::mutex WindowManager::wmDetectedMutex_;


std::unique_ptr<WindowManager> WindowManager::Create(const std::string& displayStr)
{
    Logger::EnableTraceback();

    const char* displayCStr = displayStr.empty() ? nullptr : displayStr.c_str();

    Display* display = XOpenDisplay(displayCStr);

    if(display == nullptr)
    {
        LOG_CRITICAL("Failed to open X display %s !\n", XDisplayName(displayCStr));
        return nullptr;
    }

    return std::unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display* display)
    :root_(DefaultRootWindow(display_)),
    WM_PROTOCOLS(XInternAtom(display_, "WM_PROTOCOLS", false)),
    WM_DELETE_WINDOW(XInternAtom(display_, "WM_DELETE_WINDOW", false))
    {}

WindowManager::~WindowManager()
{
    XCloseDisplay(display_);
}

void WindowManager::Run()
{
    std::lock_guard<std::mutex> lock(wmDetectedMutex_);

    wmDetected_ = false;

    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display_, false);

    if(wmDetected_)
    {
        LOG_ERROR("Detected another window maager on display ", XDisplayString(display_), "\n");
        return;
    }

    XSetErrorHandler(&WindowManager::OnXError);
    XGrabServer(display_);

    Window returnedRoot, returnedParent;
    Window* topLevelWindows;
    u_int numTopLevelWindows;

    for(u_int i = 0; i < numTopLevelWindows; ++i)
        Frame(topLevelWindows[i], true);

    XFree(topLevelWindows);
    XUngrabServer(display_);

    // Event loop
    for(;;)
    {
        XEvent e;
        XNextEvent(display_, &e);
        LOG_INFO("Received event: ", ToString(e).c_str());

        switch (e.type)
        {
        case CreateNotify:
            OnCreateNotify(e.xcreatewindow);
            break;
        case DestroyNotify:
            OnDestroyNotify(e.xdestroywindow);
            break;
        case ReparentNotify:
            OnReparentNotify(e.xreparent);
            break;
        case MapNotify:
            OnMapNotify(e.xmap);
            break;
        case UnmapNotify:
            OnUnmapNotify(e.xunmap);
            break;
        case ConfigureNotify:
            OnConfigureNotify(e.xconfigure);
            break;
        case MapRequest:
            OnMapRequest(e.xmaprequest);
            break;
        case ConfigureRequest:
            OnConfigureRequest(e.xconfigurerequest);
            break;
        case ButtonPress:
            OnButtonPress(e.xbutton);
            break;
        case ButtonRelease:
            OnButtonRelease(e.xbutton);
            break;
        case MotionNotify:
            while(XCheckTypedWindowEvent(display_, e.xmotion.window, MotionNotify, &e))
            {}
            OnMotionNotify(e.xmotion);
            break;
        case KeyPress:
            OnKeyPress(e.xkey);
            break;
        case KeyRelease:
            OnKeyRelease(e.xkey);
            break;
        default:
            LOG_WARNING("Ignored event\n", NULL);
            break;
        }
    }
}

void WindowManager::Frame(Window w, bool wasCreatedBeforeWindowManager)
{
    const u_int BORDER_WIDTH = 3;
    const u_long BORDER_COLOR = 0xff0000;
    const u_long BG_COLOR = 0x0000ff;

    XWindowAttributes xWindowAttrs;

    if(wasCreatedBeforeWindowManager)
    {
        if(xWindowAttrs.override_redirect || xWindowAttrs.map_state != IsViewable)
            return;
    }

    const Window frame = XCreateSimpleWindow(display_, root_, xWindowAttrs.x, xWindowAttrs.y, xWindowAttrs.width, xWindowAttrs.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);

    XSelectInput(display_, frame, SubstructureRedirectMask | SubstructureNotifyMask);
    XAddToSaveSet(display_, w);
    XReparentWindow(display_, w, frame, 0, 0);
    XMapWindow(display_, frame);

    clients_[w] = frame;

    // move window with alt + left button
    XGrabButton(display_, 
                Button1, 
                Mod1Mask, 
                w, 
                false,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync,
                GrabModeAsync,
                None,
                None
    );

    // resize window with alt + right button
    XGrabButton(display_,
                Button3,
                Mod1Mask,
                w,
                false,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync,
                GrabModeAsync,
                None,
                None
    );

    // destroy window with alt+f4
    XGrabKey(display_,
             XKeysymToKeycode(display_, XK_F4),
             Mod1Mask,
             w,
             false,
             GrabModeAsync,
             GrabModeAsync
    );

    // switch window with alt + tab
    XGrabKey(display_,
             XKeysymToKeycode(display_, XK_Tab),
             Mod1Mask,
             w,
             false,
             GrabModeAsync,
             GrabModeAsync
    );

    std::cout << "Framed window" << w << "to" << "[" << frame << "]" << std::endl;
}

void WindowManager::Unframe(Window w)
{
    const Window frame = clients_[w];

    XUnmapWindow(display_, frame);
    XReparentWindow(display_, w, root_, 0, 0);
    XRemoveFromSaveSet(display_, w);
    XDestroyWindow(display_, frame);

    clients_.erase(w);

    std::cout << "Unframed window" << w << "to" << "[" << frame << "]" << std::endl;
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e)
{}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e)
{}

void WindowManager::OnReparentNotify(const XReparentEvent& e)
{}

void WindowManager::OnMapNotify(const XMapEvent& e)
{}

void WindowManager::OnUnmapNotify(const XUnmapEvent& e)
{
    if(!clients_.count(e.window))
    {
        LOG_WARNING("Ignore UnmapNotify for non-client window.", NULL);
        return;
    }

    if(e.event == root_)
    {
        LOG_WARNING("Ignore UnmapNotify for reparented pre-existing window\n", NULL);
        return;
    }

    Unframe(e.window);
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e)
{}

void WindowManager::OnMapRequest(const XMapRequestEvent& e)
{
    Frame(e.window, false);
    XMapWindow(display_, e.window);   
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e)
{
    XWindowChanges changes;
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    if(clients_.count(e.window))
    {
        const Window frame = clients_[e.window];
        XConfigureWindow(display_, frame, e.value_mask, &changes);
        std::cout << "Resize window " << frame << "to" << Size<int>(e.width, e.height) << std::endl;
    }

    XConfigureWindow(display_, e.window, e.value_mask, &changes);
    std::cout << "Resize window" << e.window << "to" << Size<int>(e.width, e.height) << std::endl;
}

void WindowManager::OnButtonPress(const XButtonEvent& e)
{
    const Window frame = clients_[e.window];

    dragStartPosition_ = Position<int>(e.x_root, e.y_root);

    Window returnedRoot;
    int x, y;
    unsigned width, height, borderWidth, depth;

    dragStartFramePosition_ = Position<int>(x, y);
    dragStartFrameSize_ = Size<int>(width, height);

    XRaiseWindow(display_, frame);
}

void WindowManager::OnButtonRelease(const XButtonEvent &e)
{}

void WindowManager::OnMotionNotify(const XMotionEvent& e)
{
    const Window frame = clients_[e.window];

    const Position<int> dragPosition(e.x_root, e.y_root);
    const Vector2D<int> delta = dragPosition - dragStartPosition_;

    if(e.state & Button1Mask)
    {
        // alt + left button to move window
        const Position<int> destFramePosition = dragStartFramePosition_ + delta;
        XMoveWindow(display_, frame, destFramePosition.x, destFramePosition.y);
    }
    else if(e.state & Button3Mask)
    {
        // alt + right button to resize window
        const Vector2D<int> sizeDelta(std::max(delta.x, -dragStartFrameSize_.width),
                                      std::max(delta.y, -dragStartFrameSize_.height)
        );
        const Size<int> destFrameSize = dragStartFrameSize_+ sizeDelta;

        XResizeWindow(display_, frame, destFrameSize.width, destFrameSize.height);
        XResizeWindow(display_, e.window, destFrameSize.width, destFrameSize.height);
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& e)
{
    if((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display_, XK_F4)))
    {
        // close window with alt f4
        Atom* supportedProtocols;
        int numSupportedProtocols;

        if(XGetWMProtocols(display_, e.window, &supportedProtocols, &numSupportedProtocols) && 
                          (std::find(supportedProtocols, supportedProtocols + numSupportedProtocols, WM_DELETE_WINDOW)))
        {
            LOG_INFO("Gracefully deleting window.", NULL);

            XEvent msg;
            memset(&msg, 0, sizeof msg);
            msg.xclient.type = ClientMessage;
            msg.xclient.message_type = WM_PROTOCOLS;
            msg.xclient.window = e.window;
            msg.xclient.format = 32;
            msg.xclient.data.l[0] = WM_DELETE_WINDOW;
        }
        else
        {
            LOG_INFO("Killing window\n", NULL);
            XKillClient(display_, e.window);
        }
    }
    else if ((e.state & Mod1Mask) && e.keycode == XKeysymToKeycode(display_,  XK_Tab))
    {
        // switch window with alt tab
        auto i = clients_.find(e.window);
        ++i;

        if(i == clients_.end())
            i = clients_.begin();

        XRaiseWindow(display_, i->second);
        XSetInputFocus(display_, i->first, RevertToPointerRoot, CurrentTime);
    }
}

void WindowManager::OnKeyRelease(const XKeyEvent& e)
{}

int WindowManager::OnXError(Display* display, XErrorEvent* e)
{
    const int MAX_ERROR_TEST_LENGTH = 1024;
    char errorText[MAX_ERROR_TEST_LENGTH];

    XGetErrorText(display, e->error_code, errorText, sizeof errorText);
    std::cout << "Received error: \n"
               << "     Request: " << int(e->request_code)
               << " - " << XRequestCodeToString(e->request_code) << "\n"
               << "     Error code: " << int(e->error_code)
               << " - " << errorText << "\n"
               << "     Resource ID: " << e->resourceid << std::endl;

    return 0;
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* e)
{
    wmDetected_ = true;

    return 0;
}
