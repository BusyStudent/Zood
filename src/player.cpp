#include "player.hpp"

#include <Btk/event.hpp>
#include <iostream>

VideoPlayer::VideoPlayer(Bilibili &client, int season_id) : client(client) {
    resize(800, 600);
    
    // Try fetch
    auto eps_result = client.fetch_eps(season_id);
    if (!eps_result.has_value()) {
        ::MessageBoxA(nullptr, "Failed to fetch eps", "Error", MB_OK);
        return;
    }

    eps = eps_result.value();

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
    });
    player.signal_position_changed().connect([this](double pos) {
        set_window_title(u8string::format("Zood 进度 %lf - %lf", pos, player.duration()));
        slider.set_value(pos);
    });
    player.signal_duration_changed().connect([this](double dur) {
        slider.set_range(0, dur);
    });

    slider.signal_slider_moved().connect([this]() {
        player.set_position(slider.value());
    });

    eps_box.signal_item_clicked().connect(&VideoPlayer::on_ep_selected, this);
}
VideoPlayer::~VideoPlayer() {

}

bool VideoPlayer::resize_event(Btk::ResizeEvent& e) {
    if (fullscreen) {
        // Fullscreen only need it
        view.resize(width(), height());
        return true;
    }

    eps_box.resize(40, height());
    eps_box.move(width() - eps_box.width(), 0);

    view.move(0, 0);
    view.resize(width() - eps_box.width(), height() - slider.height());

    slider.move(0, height() - slider.height());
    slider.resize(width() - eps_box.width(), slider.height());
    return true;
}
bool VideoPlayer::close_event(Btk::CloseEvent &event) {
    player.stop();
    return true;
}
bool VideoPlayer::key_press(Btk::KeyEvent &event) {
    if (event.key() != Btk::Key::F11) {
        return false;
    }
    fullscreen = !fullscreen;
    eps_box.set_visible(!fullscreen);
    slider.set_visible(!fullscreen);

    set_fullscreen(fullscreen);
    return true;
}
void VideoPlayer::on_ep_selected(Btk::ListItem* item) {
    auto idx = eps_box.index_of(item);
    
    auto ep = eps.at(idx);
    auto url = client.fetch_video_url(ep.bvid, ep.cid);

    if (!url.has_value()) {
        ::MessageBoxA(nullptr, "Failed to fetch url", "Error", MB_ICONERROR);
        return;
    }

    player.stop();

    defer_call([url, this]() {
        player.set_url(url.value());
        player.set_option("referer", "https://www.bilibili.com/");
        player.set_option("user_agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
        player.play();
    });
}