#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <iostream>

#if defined(_MSC_VER) && defined(NDEBUG)
    #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "video_source.hpp"
#include "app.hpp"

Vec<VideoProvider*> &GetProviders() {
    static Vec<VideoProvider*> p;
    return p;
}
void RegisterProvider(VideoProvider *(*f)()) {
    GetProviders().push_back(f());
}
int main() {
    Btk::UIContext ctxt;

    App app;
    app.show();

// #if !defined(NDEBUG)
//     TextEdit search_edit;
//     ListBox lbox;

//     search_edit.show();
//     lbox.show();

//     search_edit.signal_enter_pressed().connect([&]() {
//         auto r = GetProviders()[0]->search_bangumi(search_edit.text());
//         lbox.clear();
//         for (auto &v : r) {
//             ListItem item;
//             item.text = v->title();
//             lbox.add_item(item);
//             for (auto &i : v->videos().value()) {
//                 std::cout << i->title() << std::endl;
//             }
//         }
//     });
// #endif

    int ret = ctxt.run();

    // Release providers;
    for (auto p : GetProviders()) {
        delete p;
    }

    return ret;
}