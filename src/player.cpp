#include "player.hpp"

#include <Btk/event.hpp>
#include <iostream>


bool DanmakuView::paint_event(Btk::PaintEvent &event) {
    float w = width();
    float h = height();

    auto &p = painter();
    p.save();
    p.scissor(rect());

    p.set_text_align(Btk::AlignLeft + Btk::AlignTop);
    for (auto &n : *nodes) {

        // 画每一条弹幕
        p.set_color(n.d->color);
        p.draw_text(n.text, n.x, n.y);
    }
    p.restore();
    return true;
}

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
        played = false;
    });
    player.signal_position_changed().connect([this](double pos) {
        set_window_title(u8string::format("Zood 进度 %lf - %lf", pos, player.duration()));
        slider.set_value(pos);
    });
    player.signal_duration_changed().connect([this](double dur) {
        slider.set_range(0, dur);
    });

    slider.signal_slider_moved().connect([this]() {
        if (!played) {
            return;
        }
        player.set_position(slider.value());
        danmaku_seek(slider.value());
    });

    eps_box.signal_item_clicked().connect(&VideoPlayer::on_ep_selected, this);
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
    return true;
}
bool VideoPlayer::key_press(Btk::KeyEvent &event) {
    if (event.key() == Btk::Key::Space && played) {
        if (!paused) {
            player.pause();
            paused = true;
        }
        else {
            player.resume();
            paused = false;
        }
        return true;
    }
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
    danmaku_stop();
    player.stop();
    played = false;
    paused = false;

    auto idx = eps_box.index_of(item);
    
    auto ep = eps.at(idx);
    auto url = client.fetch_video_url(ep.bvid, ep.cid);
    auto dan = client.fetch_danmaku(ep.cid);

    if (!url.has_value()) {
        ::MessageBoxA(nullptr, "Failed to fetch url", "Error", MB_ICONERROR);
        return;
    }
    if (!dan.has_value()) {
        ::MessageBoxA(nullptr, "Failed to fetch Danmaku", "Error", MB_ICONERROR);
    }

    for (auto &d : dan.value()) {
        std::cout << d.position << " " << d.text << std::endl;\
    }
    danmakus = dan.value();
    danmaku_nodes.clear();

    // Try fetch danmaku

    defer_call([url, this]() {
        player.set_url(url.value());
        player.set_option("referer", "https://www.bilibili.com/");
        player.set_option("user_agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
        player.play();
        danmaku_start();

        paused = false;
        played = true;
    });
}
void VideoPlayer::danmaku_start() {
    // 30 FPS
    timerid = add_timer(1000 / 60);

    // 重置状态
    danmakus_iter = danmakus.begin();
    cur_danmakus_added = false;
}
void VideoPlayer::danmaku_stop() {
    if (timerid) {
        del_timer(timerid);
    }
    danmaku_nodes.clear();

    repaint();
}
void VideoPlayer::danmaku_seek(double pos) {
    danmaku_nodes.clear();

    danmakus_iter = danmakus.begin();
    while (danmakus_iter->position < pos && danmakus_iter != danmakus.end()) {
        ++danmakus_iter;
    }

    repaint();
}
bool VideoPlayer::timer_event(Btk::TimerEvent &event) {
    auto rng_at = [](int begin, int end) {
        return begin + rand() % (end - begin);
    };
    if (event.timerid() != timerid) {
        return false;
    }
    double pos = player.position();
    double alive_time = 10; // 每一条保存10秒
    if (danmakus_iter != danmakus.end()) {
        // Add danmakus
        while (danmakus_iter->position < pos && danmakus_iter != danmakus.end()) {
            // 加
            DanmakuNode node;

            auto ft = font();
            ft.set_size(danmakus_iter->size);

            node.text.set_text(danmakus_iter->text);
            node.text.set_font(ft);

            node.d = &*danmakus_iter;

            auto [w, h] = node.text.size();
            node.w = w;
            node.h = h;


            // 普通弹幕
            if (danmakus_iter->is_regular()) {
                int hint = 0;
                do {
                    again : hint += 1;
                    if (hint > 100) {
                        // 扔了
                        continue;
                    }

                    node.x = danmaku_view.x() + danmaku_view.width() + rng_at(0, 10);
                    node.y = danmaku_view.y() + rng_at(0, danmaku_view.height() - node.h);

                    // 检查有没有挡住
                    for (auto &n : danmaku_nodes) {
                        if (n.is_intersected(node)) {
                            goto again;
                        }
                    }
                    break;
                }
                while (true);
            }
            else if (danmakus_iter->is_top()) {
                // 放在中间
                node.x = danmaku_view.x() + danmaku_view.width() / 2 - node.w / 2;
                node.y = danmaku_view.y();

                for (auto &n : danmaku_nodes) {
                    if (n.d->is_regular()) {
                        continue;
                    }
                    if (n.is_intersected(node)) {
                        node.y += node.h;
                    }
                }
            }
            else if (danmakus_iter->is_bottom()) {
                // 放在中间 但是从下向上放
                node.x = danmaku_view.x() + danmaku_view.width() / 2 - node.w / 2;
                node.y = danmaku_view.y() + danmaku_view.height() - node.h;

                for (auto &n : danmaku_nodes) {
                    if (n.d->is_regular()) {
                        continue;
                    }
                    if (n.is_intersected(node)) {
                        node.y -= node.h;
                    }
                }
            }
            else {
                // 不支持 扔了
                continue;
            }


            danmaku_nodes.emplace_back(std::move(node));
            ++danmakus_iter;
        }
    }
    // 移动danmaku
    int w = view.width();
    for (auto iter = danmaku_nodes.begin(); iter != danmaku_nodes.end(); ) {

        if (iter->d->is_regular()) {
            // 移动普通弹幕
            iter->x -= w / alive_time / 60;

            if ((iter->x + iter->w) < 0) {
                iter = danmaku_nodes.erase(iter);
                continue;
            }
        }
        else if (iter->d->is_bottom() || iter->d->is_top()) {
            // 站在中间不动的
            if (iter->d->position + alive_time < pos) {
                // 超时 
                iter = danmaku_nodes.erase(iter);
                continue;
            }
        }
        ++iter;
    }

    repaint();
}