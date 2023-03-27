#pragma once

#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widget.hpp>
#include "bilibili.hpp"

using Btk::ListBox;
using Btk::TextEdit;
using Btk::Button;
using Btk::u8string_view;

// Main Window of the Player
class App final : public Btk::Widget {
    public:
        App();
        ~App();
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