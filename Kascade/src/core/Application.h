#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <iostream>

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void InitWindow();
    void CreateInstance();
    void MainLoop();
    void Cleanup();

    GLFWwindow* window;
    VkInstance instance;
    VkDevice device;
    const int width = 800;
    const int height = 600;
};
