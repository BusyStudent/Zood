#include "player.hpp"

#include <Btk/event.hpp>
#include <iostream>


VideoPlayer::VideoPlayer(Bilibili &client, const Vec<Eps> &ieps) : client(client) {
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
        set_window_title(u8string::format("Zood 进度 %lf - %lf 音量 %f", pos, player.duration(), device.volume()));
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

    danmaku_view.set_opacity(0.8f);
}
VideoPlayer::~VideoPlayer() {

}

bool VideoPlayer::resize_event(Btk::ResizeEvent& e) {
    if (fullscreen) {
        // Fullscreen only need it
        view.resize(width(), height());
        danmaku_view.resize(width(), height());
        danmaku_view.raise();
        return true;
    }

    eps_box.resize(40, height());
    eps_box.move(width() - eps_box.width(), 0);

    // View 和 danmakuView 在一个位置上 不过弹幕 在他上面
    view.move(0, 0);
    view.resize(width() - eps_box.width(), height() - slider.height());

    danmaku_view.move(0, 0);
    danmaku_view.resize(width() - eps_box.width(), height() - slider.height());
    danmaku_view.raise();

    slider.move(0, height() - slider.height());
    slider.resize(width() - eps_box.width(), slider.height());
    return true;
}
bool VideoPlayer::close_event(Btk::CloseEvent &event) {
    player.stop();
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

        set_fullscreen(fullscreen);
        return true;
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
void VideoPlayer::on_ep_selected(Btk::ListItem* item) {
    danmaku_view.stop();
    player.stop();
    played = false;
    paused = false;

    if (loading) {
        // 加载中 忽略
        return;
    }

    auto idx = eps_box.index_of(item);
    
    auto ep = eps.at(idx);


    std::async([this, ep]() {
        loading = true;
        auto url = client.fetch_video_url(ep.bvid, ep.cid);
        auto dan = client.fetch_danmaku(ep.cid);

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

    });
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