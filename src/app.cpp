#include <Btk/event.hpp>
#include <cpr/cpr.h>
#include <iostream>

#undef min
#undef max

#include "player.hpp"
#include "app.hpp"

App::App() {
    root()->set_window_title("Zood SearchPage");
    // set_window_icon(PixBuffer::FromFile("icon.jpg"));

    // Put tabwidget at center
    set_widget(&tabwidget);

    tabwidget.add_tab(new SearchPage(), "搜索");
    tabwidget.set_current_index(0); // default to search page, change if needed later.

    // Add MenuItem
    auto sub =  menubar().add_menu("文件");

    menubar().add_item("设置");
    menubar().add_item("关于")->signal_triggered().connect([]() {
        ::MessageBoxW(nullptr, L"Powered by Btk", L"关于", MB_ICONINFORMATION | MB_OK);
    });
}
App::~App() {

}
bool App::key_press(KeyEvent &event) {
    if (event.key() == Key::F11) {
        fullscreen = !fullscreen;
        root()->set_fullscreen(fullscreen);
        return true;
    }
    return false;
}

SearchPage::SearchPage() {
    set_font(Btk::Font("宋体", 15));

    // resize(800, 600);

    search_input.set_placeholder("输入你要看的Bilibili番剧 或者Bilibili网址");
    search_input.signal_enter_pressed().connect(&SearchPage::on_search_required, this);
    search_button.set_text("搜索");
    search_button.signal_clicked().connect(&SearchPage::on_search_required, this);

    result_box.signal_item_clicked().connect(&SearchPage::on_item_selected, this);
}
SearchPage::~SearchPage() {

}
bool SearchPage::resize_event(Btk::ResizeEvent &event) {
    auto [width, height] = size();

    search_input.move(x(), y());
    search_input.resize(width - search_button.width(), search_button.height());

    search_button.move(search_input.x() + search_input.width(), y());

    // 放置列表框
    result_box.resize(width, height - search_input.height());
    result_box.move(x(), y() + search_input.height());

    return true;
}
void SearchPage::on_search_required() {
    if (searching) {
        return;
    }
    u8string_view content = search_input.text();
    if (content.empty()) {
        return;
    }
    if (content.contains("bilibi.com")) {
        // 是网址
        assert(false);
    }

    root()->set_window_title(u8string::format("Zood SearchPage 搜索中:%s", content.copy().c_str()));

    std::thread([this, content]() {
        searching = true;
        auto list = client.search_bangumi(u8string(content));
        if (!list.has_value()) {
            // Bad
            std::cout << "Bad search" << std::endl;
            return;
        }
        auto cvs = client.fetch_covers(list.value());

        // Got, back to main thread
        defer_call([this, list, cvs]() {
            result_box.clear();
            bangumi_list = list.value();
            auto convers = cvs.value_or(Vec<PixBuffer>());

            size_t n = 0;
            for (auto &elem : bangumi_list) {
                Btk::ListItem item;
                item.text = elem.title;
                item.font = font();
                if (convers.size() > n) {
                    item.image = convers[n];
                    item.image_size = Size(160, 200);
                }

                result_box.add_item(item);
                n += 1;
            }
            searching = false;
            root()->set_window_title(u8string::format("Zood App 搜索完成"));
        });
    }).detach();
}
void SearchPage::on_item_selected(Btk::ListItem *item) {
    if (loading) {
        return;
    }
    int idx = result_box.index_of(item);

    auto &bangumi = bangumi_list.at(idx);

    // Try fetch
    std::thread([this, seasonid = bangumi.season_id, bangumi]() {
        loading = true;
        auto eps_result = client.fetch_eps(seasonid);
        if (!eps_result.has_value()) {
            ::MessageBoxA(nullptr, "Failed to fetch eps", "Error", MB_OK);
            loading = false;
            return;
        }
        defer_call([this, eps_result, bangumi]() {
            auto *player = new VideoPlayer(client, bangumi, eps_result.value());
            // player->set_attribute(Btk::WidgetAttrs::DeleteOnClose, true);
            // player->show();
            auto index = root()->as<App*>()->tabwidget.add_tab(player, "播放");
            // root()->as<App*>()->tabwidget.set_current_index(index);
            loading = false;
        });
    }).detach();
}