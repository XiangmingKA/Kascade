// Kascade.cpp : Defines the entry point for the application.
//

#include "Kascade.h"
#include "core/Application.h"

int main() {
    try {
        Application app;
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
