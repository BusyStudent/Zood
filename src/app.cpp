#include <Btk/event.hpp>
#include <cpr/cpr.h>
#include <iostream>

#undef min
#undef max

#include "player.hpp"
#include "app.hpp"

App::App() {
    resize(600, 800);
    root()->set_window_title("Zood App");
    // set_window_icon(PixBuffer::FromFile("icon.jpg"));

    // Put tabwidget at center
    set_widget(&tabwidget);

    tabwidget.add_tab(new SearchPage(), "搜索");
    tabwidget.set_current_index(0); // default to search page, change if needed later.

    tabwidget.add_tab(new ToolPage(), "工具测试");

    tabwidget.add_tab(new ListBox(), "日志");

    // Add MenuItem
    auto sub =  menubar().add_menu("文件");

    menubar().add_item("设置");
    menubar().add_item("关于")->signal_triggered().connect([]() {
        ::MessageBoxW(nullptr, L"Powered by Btk, Version 0.0114514", L"关于", MB_ICONINFORMATION | MB_OK);
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
    set_focus_policy(FocusPolicy::Mouse);

    // resize(800, 600);

    search_input.set_placeholder("输入你要看的Bilibili番剧 或者Bilibili网址");
    search_input.signal_enter_pressed().connect(&SearchPage::on_search_required, this);
    search_button.set_text("搜索");
    search_button.signal_clicked().connect(&SearchPage::on_search_required, this);

    result_box.signal_item_clicked().connect(&SearchPage::on_item_selected, this);

    search_input.set_flat(true);
    result_box.set_flat(true);
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
            player->set_attribute(Btk::WidgetAttrs::DeleteOnClose, true);
            player->show();
            // auto index = root()->as<App*>()->tabwidget.add_tab(player, "播放");
            // root()->as<App*>()->tabwidget.set_current_index(index);
            loading = false;
        });
    }).detach();
}

CompeleteBox::CompeleteBox(Widget *parent, Bilibili &client) : TextEdit(parent), client(client) {
    signal_text_changed().connect(&CompeleteBox::on_text_changed, this);
    signal_enter_pressed().connect(&ListBox::hide, &listbox);

    listbox.signal_item_clicked().connect([this](ListItem *item) {
        except_change = true;
        set_text(item->text);
        except_change = false;

        listbox.hide();

        // Like user enter it
        signal_enter_pressed().emit();
    });
}
CompeleteBox::~CompeleteBox() {

}
void CompeleteBox::on_text_changed() {
    if (text().empty()) {
        // No values
        listbox.hide();
        return;
    }
    if (fetching || except_change) {
        return;
    }
    std::thread([this, t = text().copy()]() {
        fetching = true;
        auto result = client.fetch_search_suggests(t);
        if (result.has_value()) {
            for (auto &item : result.value()) {
                std::cout << u8string_view("搜索建议 :") <<  item << std::endl;
            }
        }
        // defer_call(&CompeleteBox::on_compelete_ready, this, );

        defer_call([this, res = result]() {
            fetching = false;
            on_compelete_ready(res);
        });
    }).detach();
}
void CompeleteBox::on_compelete_ready(const Result<Vec<u8string>> &items) {
    listbox.clear();
    if (!items.has_value() || text().empty()) {
        // No values
        listbox.hide();
        return;
    }
    if (items.value().empty()) {
        // No values
        listbox.hide();
        return;
    }
    // Has value
    for (auto &item : items.value()) {
        listbox.add_item(ListItem(item));
    }
    listbox.set_current_item(listbox.item_at(0)); // Select first item. 
    if (listbox.parent() != parent()) {
        listbox.set_parent(parent());
        listbox.raise();

        listbox.resize(width(), 200);
        listbox.move(rect().bottom_left());
    }
    if (!listbox.visible()) {
        listbox.show();
    }
}
bool CompeleteBox::move_event(MoveEvent &event) {
    listbox.move(rect().bottom_left());

    return TextEdit::move_event(event);
}
bool CompeleteBox::resize_event(ResizeEvent &event) {
    listbox.resize(width(), 200);

    return TextEdit::resize_event(event);
}
bool CompeleteBox::key_press(KeyEvent &event) {
    if (TextEdit::key_press(event)) {
        return true;
    }
    if (listbox.visible()) {
        // Has datas
        auto idx = listbox.index_of(listbox.current_item());
        if (event.key() == Key::Up) {
            idx -= 1;
        }
        else if (event.key() == Key::Down) {
            idx += 1;
        }
        else {
            return false;
        }

        if (idx < 0) {
            // To end
            idx = listbox.count_items() - 1;
        }
        else if(idx >= listbox.count_items()) {
            idx = 0;
        }
        listbox.set_current_item(listbox.item_at(idx)); // Select the item.
        except_change = true;
        set_text(listbox.current_item()->text);
        except_change = false;
        return true;
    }
    return false;
}


// Tool Page
class ToolPage::Priv {
    public:
        Vec<Ptr<VideoCollection>> videos;
};

ToolPage::ToolPage() {
    priv = new Priv;

    layout.attach(this);

    auto sublay = new BoxLayout(LeftToRight);
    layout.add_layout(sublay);

    auto tedit = new TextEdit();
    auto cbbox = new ComboBox();
    auto btn = new Button("搜");

    sublay->add_widget(tedit, 1);
    sublay->add_widget(cbbox);
    sublay->add_widget(btn);

    auto lbox = new ListBox();
    layout.add_widget(lbox, 1);

    cbbox->set_fixed_size(btn->size_hint());

    // Add data
    for (auto &item : GetProviders()) {
        cbbox->add_item(item->name());
    }
    cbbox->set_current_index(0);


    btn->signal_clicked().connect([=]() {
        VideoProvider *provider = nullptr;
        for (auto &item : GetProviders()) {
            if (item->name() != cbbox->current_text()) {
                continue;
            }
            else {
                provider = item;
                break;
            }
        }
        BTK_ASSERT(provider);

        lbox->clear();
        priv->videos = provider->search_bangumi(tedit->text());
        if (priv->videos.empty()) {
            ::MessageBoxW(nullptr, L"没有结果", L"Error", MB_OK);
        }
        for (auto &i : priv->videos) {
            auto covers = i->covers();
            if (covers.has_value()) {
                lbox->add_item(ListItem(i->title(), covers.value(), Size(160, 200)));
            }
            else {
                lbox->add_item(ListItem(i->title()));
            }
        }
    });
    tedit->set_placeholder("测试数据源搜索");
    tedit->signal_enter_pressed().connect([=]() {
        btn->signal_clicked().emit();
    });

}
ToolPage::~ToolPage() {
    delete priv;
}