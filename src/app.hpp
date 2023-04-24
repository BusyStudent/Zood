#pragma once

#include <Btk/widgets/mainwindow.hpp>
#include <Btk/widgets/container.hpp>
#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widget.hpp>
#include <Btk/layout.hpp>
#include "bilibili.hpp"

using namespace BTK_NAMESPACE;

// Main Window of the Player
class App final : public MainWindow { 
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

class CompeleteBox final : public TextEdit {
    public:
        CompeleteBox(Widget *parent, Bilibili &client);
        ~CompeleteBox();
    protected:
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
        bool key_press(KeyEvent &) override;
    private:
        void on_text_changed();
        void on_compelete_ready(const Result<Vec<u8string>> &);

        Bilibili &client;
        bool fetching = false;
        bool except_change = false; //< Ingore the text change
        ListBox listbox;
};

class SearchPage final : public Widget {
    public:
        SearchPage();
        ~SearchPage();
    protected:
        // For relayout
        bool resize_event(Btk::ResizeEvent &event) override;
    private:
        void on_search_required();
        void on_item_selected(Btk::ListItem *item);

        Bilibili client;

        ListBox result_box {this};
        CompeleteBox search_input {this, client};
        Button  search_button {this};

        // Current bangumi
        Vec<Bangumi> bangumi_list;

        // Current state
        bool searching = false;
        bool loading = false;

};

class TimelinePage final : public Btk::Widget {

};

class ToolPage final : public Widget {
    public:
        ToolPage();
        ~ToolPage();
    private:
        class Priv;

        BoxLayout layout{TopToBottom};

        Priv *priv;
};