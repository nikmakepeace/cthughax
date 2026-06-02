// Graphical executable entry point.

#include "Application.h"

int main(int argc, char* argv[]) {
    Application application(argc, argv);

    if (application.initialize())
        application.run();

    return application.exitStatus();
}
