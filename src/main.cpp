#include <Btk/context.hpp>
#include <Btk/widget.hpp>

int main() {
    Btk::UIContext ctxt;
    Btk::Widget window;

    window.set_window_title("Zood Player");
    window.show();
    
    return ctxt.run();
}