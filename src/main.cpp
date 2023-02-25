#include <Btk/context.hpp>
#include <Btk/widget.hpp>

// #if defined(_MSC_VER) && defined(NDEBUG)
//     #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
// #endif

#include "app.hpp"

int main() {
    Btk::UIContext ctxt;

    App app;
    app.show();

    return ctxt.run();
}