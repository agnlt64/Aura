#include <cstdlib>
#include "WindowManager.hpp"
#include "Logger.hpp"

int main(int argc, char** argv)
{
    Logger::SetPriority(DevLevel);
    Logger::EnableTraceback();
    Logger::EnableFileOutput("logs/logs.txt");

    std::unique_ptr<WindowManager> windowManager = WindowManager::Create("Aura-WM");

    if(!windowManager)
    {
        LOG_CRITICAL("Failed to initialize Aura!\n", NULL);
        return EXIT_FAILURE;
    }

    windowManager->Run();

    return EXIT_SUCCESS;
} 