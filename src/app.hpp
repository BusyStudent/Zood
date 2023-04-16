#pragma once

#include <Btk/widgets/mainwindow.hpp>
#include <Btk/widgets/container.hpp>
#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widget.hpp>
#include "bilibili.hpp"

using namespace BTK_NAMESPACE;

// Main Window of the Player
class App final : public Btk::MainWindow { 
    public:
        App();
        ~App();
    protected:
        bool key_press(KeyEvent &) override;
    private:
        TabWidget tabwidget;
        bool      fullscreen = false;
    friend class SearchPage;
};

class SearchPage final : public Btk::Widget {
    public:
        SearchPage();
        ~SearchPage();
    protected:
        // For relayout
        bool resize_event(Btk::ResizeEvent &event) override;
    private:
        void on_search_required();
        void on_item_selected(Btk::ListItem *item);

        ListBox result_box {this};
        TextEdit search_input {this};
        Button  search_button {this};

        Bilibili client;

        // Current bangumi
        Vec<Bangumi> bangumi_list;

        // Current state
        bool searching = false;
        bool loading = false;

};