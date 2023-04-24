#include "player.hpp"

#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/dialog.hpp>
#include <Btk/plugins/barrier.hpp>
#include <Btk/event.hpp>
#include <iostream>

class ChooseBox : public Dialog {
    public:
        ChooseBox();

        ComboBox *cbbox;
    private:
        HBoxLayout layout {this};
};
class InputBox : public Dialog {
    public:
        InputBox();

        TextEdit *edit;
    private:
        HBoxLayout layout {this};
};


ChooseBox::ChooseBox() {
    cbbox = new ComboBox();
    layout.add_widget(new Label("选择一个项"));
    layout.add_widget(cbbox);
    layout.set_margin(20);
}
InputBox::InputBox() {
    edit = new TextEdit();
    layout.add_widget(new Label("请输入要替代的名字"));
    layout.add_widget(edit);
    layout.set_margin(20);
}


VideoPlayer::VideoPlayer(Bilibili &client, Bangumi b, const Vec<Eps> &ieps) : client(client), bangumi(b) {
    auto ft = font();
    ft.set_bold(true);
    ft.set_family("黑体");
    set_font(ft);

    resize(800, 600);
    
    eps = ieps;

    for (auto &ep : eps)  {
        std::cout << ep.title << " long_title:" << ep.long_title << std::endl;
        std::cout << ep.bvid << " " << ep.cid << std::endl;

        Btk::ListItem item;
        item.text = ep.title;
        item.font = font();

        eps_box.add_item(item);
    }

    player.set_video_output(&view);
    player.set_audio_output(&device);
    player.signal_error().connect([this]() {
        std::cout <<  "Player Error " <<  player.error_string() << std::endl; 
        played = false;
    });
    player.signal_position_changed().connect([this](double pos) {
        root()->set_window_title(u8string::format("Zood 进度 %lf - %lf 音量 %f", pos, player.duration(), device.volume()));
        slider.set_value(pos);
    });
    player.signal_duration_changed().connect([this](double dur) {
        slider.set_range(0, dur);
    });
    player.signal_state_changed().connect(&VideoPlayer::on_state_changed, this);

    slider.signal_slider_moved().connect([this]() {
        if (!played) {
            return;
        }
        player.set_position(slider.value());
        danmaku_view.set_position(slider.value());
    });

    eps_box.signal_item_clicked().connect(&VideoPlayer::on_ep_selected, this);
    setbtn.signal_clicked().connect([this]() {
        if (settings) {
            // 还没有关闭
            settings->raise();
            return;
        }
        auto container = new PopupWidget;
        container->set_attribute(WidgetAttrs::DeleteOnClose, true);

        settings = new PlayerSettings(this);

        auto [w, h] = settings->size_hint();
        auto [x, y] = setbtn.rect().position();
        container->resize(w, h);
        settings->resize(w, h);

        container->move(x - w + setbtn.width(), y - h);
        settings->move(x - w + setbtn.width(), y - h);

        container->add_child(settings);

        container->popup(this);

        container->signal_destoryed().connect([this]() {
            settings = nullptr;
        });
    });
    setbtn.resize(setbtn.width() / 2, setbtn.height());

    danmaku_view.set_opacity(0.8f);


    // 开始获取视频源头
    auto provider = GetProviders();
    if (provider.size() > 0) {
        auto p = provider[0];
        std::thread([this, p]() {
            // 搜索
            size_t wanted = 0;
            video_source = p->search_bangumi(bangumi.title);
            if (video_source.empty()) {
                // 靠 没有视频源 试试把 空格给切了
                video_source = p->search_bangumi(bangumi.title.split(" ")[0]);
                if (video_source.empty()) {
                    // 没辙了
                    u8string wanted;
                    Latch latch(1);
                    defer_call([&]() {
                        InputBox box;
                        box.set_window_title("没有找到结果");
                        box.edit->signal_enter_pressed().connect([&]() {
                            box.close();
                        });
                        box.run();
                        wanted = box.edit->text();
                        latch.count_down();
                    });
                    latch.wait();

                    if (wanted.empty()) {
                        return;
                    }
                    
                    video_source = p->search_bangumi(wanted);
                    if (video_source.empty()) {
                        // 没辙了
                        fetched = true;
                        return;
                    }
                }

            }



            if (video_source.size() > 1) {
                // 多余一个 要选择一下
                // TODO
                Latch latch(1);
                defer_call([&]() {
                    ChooseBox box;
                    for (auto &item : video_source) {
                        box.cbbox->add_item(item->title());
                    }
                    box.cbbox->signal_current_index_changed().connect([&](int idx) {
                        wanted = idx;
                        box.close();
                    });
                    box.run();
                    latch.count_down();
                });
                latch.wait();
            }
            auto ret = video_source[wanted]->videos();
            if (ret.has_value()) {
                videos = std::move(ret.value());
            }
            fetched = true;
        }).detach();
    }
    else {
        // 没有附加视频源 也算拿到
        fetched = true;
    }
}
VideoPlayer::~VideoPlayer() {

}

bool VideoPlayer::resize_event(Btk::ResizeEvent& e) {
    relayout();
    return true;
}
bool VideoPlayer::move_event(Btk::MoveEvent& e) {
    relayout();
    return true;
}
bool VideoPlayer::close_event(Btk::CloseEvent &event) {
    player.stop();
    if (settings) {
        settings->close();
        settings = nullptr;
    }
    return false;
}
bool VideoPlayer::key_press(Btk::KeyEvent &event) {
    if (event.key() == Btk::Key::Space && played) {
        if (player.state() == Btk::MediaPlayer::Playing) {
            player.pause();
            paused = true;
        }
        else {
            player.resume();
            paused = false;
        }
        return true;
    }
    else if (event.key() == Btk::Key::F11) {
        fullscreen = !fullscreen;
        eps_box.set_visible(!fullscreen);
        slider.set_visible(!fullscreen);
        setbtn.set_visible(!fullscreen);

        // set_fullscreen(fullscreen);
        relayout();
        return false; //< Forward to parent
    }
    else if (event.key() == Btk::Key::Right) {
        // Add position
        player.set_position(min(player.position() + 10.0, player.duration()));
        danmaku_view.set_position(player.position());
        return true;
    }
    else if (event.key() == Btk::Key::Left) {
        // Add position
        player.set_position(max(player.position() - 10.0, 0.0));
        danmaku_view.set_position(player.position());
        return true;
    }
    else if (event.key() == Btk::Key::Up) {
        // Add volume
        device.set_volume(min(device.volume() + 0.1f, 1.0f));
        return true;
    }
    else if (event.key() == Btk::Key::Down) {
        // Add volume
        device.set_volume(max(device.volume() - 0.1f, 0.0f));
        return true;
    }
    return false;
}
void VideoPlayer::relayout() {
    if (fullscreen) {
        // Fullscreen only need it
        view.resize(width(), height());
        danmaku_view.resize(width(), height());
        danmaku_view.raise();
        return;
    }

    eps_box.resize(40, height());
    eps_box.move(x() + width() - eps_box.width(), y());

    // View 和 danmakuView 在一个位置上 不过弹幕 在他上面
    view.move(x(), y());
    view.resize(width() - eps_box.width(), height() - slider.height());

    danmaku_view.move(x(), y());
    danmaku_view.resize(width() - eps_box.width(), height() - slider.height());
    danmaku_view.raise();

    slider.move(x(), y() + height() - slider.height());
    slider.resize(width() - eps_box.width() - setbtn.width(), slider.height());

    setbtn.move(slider.x() + slider.width(), slider.y());
    setbtn.resize(setbtn.width(), slider.height());
    return;
}
void VideoPlayer::on_ep_selected(Btk::ListItem* item) {
    danmaku_view.stop();
    player.stop();
    played = false;
    paused = false;

    if (loading) {
        // 加载中 忽略
        return;
    }
    if (!fetched) {
        ::MessageBoxW(nullptr, L"等待源", L"错误", MB_ICONERROR);
        return;
    }

    auto idx = eps_box.index_of(item);
    
    auto ep = eps.at(idx);


    std::thread([this, ep, idx]() {
        loading = true;
        auto dan = client.fetch_danmaku(ep.cid);

        // 没有， 那只好用官方源了
        Result<u8string> url;
        if (!videos.empty()) {
            // 取用提供的
            for (auto &v : videos) {
                std::cout << v->title() << std::endl;
                if (v->title().contains(ep.title)) {
                    // 找到了
                    url = v->media_url();
                    break;
                }
            }
            // 没有 huihui
        }
        else {
            url = client.fetch_video_url(ep.bvid, ep.cid);
        }

        if (!url.has_value()) {
            ::MessageBoxA(nullptr, "Failed to fetch url", "Error", MB_ICONERROR);
            loading = false;
            return;
        }
        if (!dan.has_value()) {
            ::MessageBoxA(nullptr, "Failed to fetch Danmaku", "Error", MB_ICONERROR);
            loading = false;
            return;
        }

        // for (auto &d : dan.value()) {
        //     std::cout << d.position << " " << d.text << std::endl;
        // }

        // Try fetch danmaku

        defer_call([url, dan ,this]() {
            danmaku_view.load(dan.value());
            player.set_url(url.value());
            player.set_option("referer", "https://www.bilibili.com/");
            player.set_option("user_agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
            player.play();


            danmaku_view.play();
            paused = false;
            played = true;
            loading = false;
        });

    }).detach();
}
void VideoPlayer::on_state_changed(Btk::MediaPlayer::State state) {
    switch (state) {
        case Btk::MediaPlayer::Playing : {
            // 30 FPS
            danmaku_view.pause(false);
            break;
        }
        case Btk::MediaPlayer::Stopped : {
            danmaku_view.stop();
            break;
        }
        case Btk::MediaPlayer::Paused : {
            danmaku_view.pause(true);
            break;
        }
    }
}
// void VideoPlayer::danmaku_start() {
//     // 重置状态
//     danmakus_iter = danmakus.begin();
//     cur_danmakus_added = false;
// }
// void VideoPlayer::danmaku_stop() {
//     if (timerid) {
//         del_timer(timerid);
//         timerid = 0;
//     }
//     danmaku_regular_nodes.clear();
//     danmaku_center_nodes.clear();

//     repaint();
// }
// void VideoPlayer::danmaku_seek(double pos) {
//     danmaku_regular_nodes.clear();
//     danmaku_center_nodes.clear();

//     danmakus_iter = danmakus.begin();
//     while (danmakus_iter->position < pos && danmakus_iter != danmakus.end()) {
//         ++danmakus_iter;
//     }
//     if (danmakus_iter != danmakus.end()) {
//         std::cout << "Seek to pos: " << danmakus_iter->position << " text: " << danmakus_iter->text << std::endl;
//     }

//     repaint();
// }
// bool VideoPlayer::timer_event(Btk::TimerEvent &event) {
// }
bool VideoPlayer::mouse_wheel(Btk::WheelEvent &e) {
    return slider.handle(e);
}
bool VideoPlayer::mouse_release(MouseEvent &e) {
    return true;
}

PlayerSettings::PlayerSettings(VideoPlayer *player) : player(player) {
    auto op_lay = new BoxLayout(LeftToRight);
    auto audio_lay = new BoxLayout(LeftToRight);
    auto ft_lay = new BoxLayout(LeftToRight);

    layout.attach(this);
    layout.set_margin(10);
    layout.add_layout(op_lay);
    layout.add_spacing(2);
    layout.add_layout(audio_lay);
    layout.add_spacing(2);
    layout.add_layout(ft_lay);
    layout.add_spacing(2);
    layout.add_stretch(1);

    // 弹幕 lay
    auto op_label = new Label("透明度");
    auto op_slider = new Slider(Horizontal);
    auto op_slider_label = new Label(u8string::format("%d%%", int(player->danmaku_view.opacity() * 100)));

    op_slider->set_range(0, 100);
    op_slider->set_value(player->danmaku_view.opacity() * 100);
    op_slider->signal_value_changed().connect([=]() {
        op_slider_label->set_text(u8string::format("%d%%", int(op_slider->value())));
        player->danmaku_view.set_opacity(float(op_slider->value()) / 100);
    });

    op_lay->add_widget(op_label);
    op_lay->add_widget(op_slider, 1);
    op_lay->add_spacing(2);
    op_lay->add_widget(op_slider_label);

    // 字体缩放
    auto ft_label = new Label("弹幕大小");
    auto ft_slider = new Slider(Horizontal);
    auto ft_slider_label = new Label(u8string::format("%d%%", int(player->danmaku_view.danmaku_scale() * 100)));

    ft_slider->set_range(0, 120);
    ft_slider->set_value(player->danmaku_view.danmaku_scale() * 100); // 保证小数点后两
    ft_slider->signal_value_changed().connect([=]() {
        ft_slider_label->set_text(u8string::format("%d%%", int(player->danmaku_view.danmaku_scale() * 100))); // 保证小数点
        player->danmaku_view.set_danmaku_scale(ft_slider->value() / 100.0f); // 保证小数点后两
    });

    ft_lay->add_widget(ft_label);
    ft_lay->add_widget(ft_slider, 1);
    ft_lay->add_spacing(2);
    ft_lay->add_widget(ft_slider_label);

    // 音频
    auto ia_label = new Label("音量 ");
    auto ia_slider = new Slider(Horizontal);
    auto ia_slider_label = new Label(u8string::format("%d%%", int(player->device.volume() * 100)));
    
    ia_slider->set_value(player->device.volume() * 100);
    ia_slider->set_range(0, 100);
    ia_slider->signal_value_changed().connect([=]() {
        ia_slider_label->set_text(u8string::format("%d%%", int(ia_slider->value())));
        player->device.set_volume(float(ia_slider->value()) / 100);
    });

    audio_lay->add_widget(ia_label);
    audio_lay->add_widget(ia_slider, 1);
    audio_lay->add_spacing(2);
    audio_lay->add_widget(ia_slider_label);

}
Size PlayerSettings::size_hint() const {
    return layout.size_hint();
}
bool PlayerSettings::close_event(CloseEvent &event) {
    // player->settings = nullptr;
    return true;
}