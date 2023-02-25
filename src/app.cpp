#include <Btk/event.hpp>
#include <cpr/cpr.h>
#include <iostream>

#undef min
#undef max

#include "player.hpp"
#include "app.hpp"

App::App() {
    set_font(Btk::Font("宋体", 15));
    show();

    set_window_title("Zood App");
    resize(800, 600);

    search_input.set_placeholder("输入你要看的Bilibili番剧 或者Bilibili网址");
    search_input.signal_enter_pressed().connect(&App::on_search_required, this);
    search_button.set_text("搜索");
    search_button.signal_clicked().connect(&App::on_search_required, this);

    result_box.signal_item_clicked().connect(&App::on_item_selected, this);
}
App::~App() {

}
bool App::resize_event(Btk::ResizeEvent &event) {
    auto [width, height] = size();

    search_input.move(0, 0);
    search_input.resize(width - search_button.width(), search_button.height());

    search_button.move(search_input.x() + search_input.width(), 0);

    // 放置列表框
    result_box.resize(width, height - search_input.height());
    result_box.move(0, search_input.height());

    return true;
}
void App::on_search_required() {
    u8string_view content = search_input.text();
    if (content.empty()) {
        return;
    }
    if (content.contains("bilibi.com")) {
        // 是网址
        assert(false);
    }
    auto list = client.search_bangumi(u8string(content));
    if (!list.has_value()) {
        // Bad
        std::cout << "Bad search" << std::endl;
        return;
    }
    result_box.clear();
    bangumi_list = list.value();

    for (auto &elem : bangumi_list) {
        Btk::ListItem item;
        item.text = elem.title;
        item.font = font();

        result_box.add_item(item);
    }
}
void App::on_item_selected(Btk::ListItem *item) {
    int idx = result_box.index_of(item);

    auto &bangumi = bangumi_list.at(idx);

    auto *player = new VideoPlayer(client, bangumi.season_id);
    player->set_attribute(Btk::WidgetAttrs::DeleteOnClose, true);
    player->show();
}