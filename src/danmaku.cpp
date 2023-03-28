#include <Btk/event.hpp>
#include <iostream>
#include "danmaku.hpp"

DanmakuView::DanmakuView(Widget *parent, MediaPlayer *player) : Widget(parent), player(player) {
    danmakus_font.set_bold(true);
    // outline = true;
    danmaku_dpi_scale = window_dpi().x / 96.0f;
}
DanmakuView::~DanmakuView() {

}
bool DanmakuView::timer_event(TimerEvent &event) {
    if (event.timerid() != timerid) {
        return false;
    }
    double pos = player->position();
    double alive_time = 10; // 每一条保存10秒

    double diff = pos - danmakus_iter->position;

    // BTK_LOG("cur pos %lf\n", pos);
    if (danmakus_iter != danmakus.end() && std::abs(diff) < 5) {
        // 加弹幕 外加 差别不能太大
        if (abs(diff) > 10) {
            // Too big
            BTK_LOG("%lf\n", diff);
        }

        while (danmakus_iter->position < pos && danmakus_iter != danmakus.end()) {
            // 加
            add_danmaku(*danmakus_iter);

            ++danmakus_iter;
        }
    }
    // 移动danmaku
    int w = width();
    for (auto &plane : danmaku_regular_planes) {
        for (auto iter = plane.begin(); iter != plane.end(); ) {
            // 移动普通弹幕
            iter->x -= (w + iter->w) / alive_time / fps;

            if ((iter->x + iter->w) < 0) {
                iter = plane.erase(iter);
                continue;
            }
        
            ++iter;
        }
    }

    for (auto iter = danmaku_center_nodes.begin(); iter != danmaku_center_nodes.end(); ) {
        // 站在中间不动的
        if (iter->d->position + alive_time < pos) {
            // 超时 
            iter = danmaku_center_nodes.erase(iter);
            continue;
        }

        ++iter;
    }

    repaint();
    return true;
}
void DanmakuView::add_danmaku(const Danmaku &dan) {
    DanmakuNode node;

    auto ft = danmakus_font;
    ft.set_size(dan.size * danmakus_size * danmaku_dpi_scale);

    node.text.set_text(dan.text);
    node.text.set_font(ft);

    node.d = &*danmakus_iter;

    auto [w, h] = node.text.size();
    node.w = w;
    node.h = h;


    // 普通弹幕
    if (dan.is_regular()) {
        // 遍历轨道
        node.x = x() + width();
        node.y = y();
        for (auto &plane : danmaku_regular_planes) {
            // 检查最后一个
            if (!plane.empty()) {
                if (plane.back().is_intersected(node)) {
                    // 向下走
                    node.y += h;
                    node.y += spacing;
                    continue;
                }
            }
            // 有空位置
            plane.push_back(std::move(node));
            return;
        }
        // 没位置 扔了
    }
    else if (dan.is_top()) {
        // 放在中间
        node.x = x() + width() / 2 - node.w / 2;
        node.y = y();

        for (auto &n : danmaku_center_nodes) {
            if (n.is_intersected(node)) {
                node.y += node.h;
            }
        }
        danmaku_center_nodes.emplace_back(std::move(node));
    }
    else if (dan.is_bottom()) {
        // 放在中间 但是从下向上放
        node.x = x() + width() / 2 - node.w / 2;
        node.y = y() + height() - node.h;

        for (auto &n : danmaku_center_nodes) {
            if (n.is_intersected(node)) {
                node.y -= node.h;
            }
        }
        danmaku_center_nodes.emplace_back(std::move(node));
    }
    // 不支持 扔了
}
bool DanmakuView::resize_event(Btk::ResizeEvent &) {
    // 切分轨道
    int h = height();
    int n = h / (danmakus_font.size() * danmakus_size + spacing);

    danmaku_regular_planes.resize(n);
    return true;
}
bool DanmakuView::paint_event(Btk::PaintEvent &event) {
    float w = width();
    float h = height();

    auto &p = painter();
    p.save();
    p.scissor(rect());

    p.set_text_align(Btk::AlignLeft + Btk::AlignTop);
    // 普通
    // 画每一条弹幕
    for (auto &plane : danmaku_regular_planes) {
        for (auto &n : plane) {
            // 拿到outline 画上把
            if (outline) {
                p.save();
                p.translate(n.x, n.y);
                p.set_color(Color::Black);
                p.draw_path(n.text.outline());
                p.restore();
            }
            if (projection) {
                p.set_color(n.d->shadow);
                p.draw_text(n.text, n.x + 1, n.y + 1);
            }
            p.set_color(n.d->color);
            p.draw_text(n.text, n.x, n.y);
        }
    }
    // 在中间的
    p.set_text_align(Btk::AlignCenter + Btk::AlignTop);
    for (auto &n : danmaku_center_nodes) {
        // p.save();
        // p.translate(w / 2.0f - n.w / 2.0f, n.y);
        // p.set_color(Color::Black);
        // p.draw_path(n.text.outline());
        // p.restore();
        if (outline) {
            p.save();
            p.translate(n.x, n.y);
            p.set_color(Color::Black);
            p.draw_path(n.text.outline());
            p.restore();
        }
        if (projection) {
            p.set_color(n.d->shadow);
            p.draw_text(n.text, w / 2.0f + 1, n.y + 1);
        }
        p.set_color(n.d->color);
        p.draw_text(n.text, w / 2.0f, n.y);
    }


    if (player->media_status() == Btk::MediaStatus::BufferingMedia) {
        p.set_color(Btk::Color::White);
        p.set_font(font());
        p.set_text_align(Btk::AlignCenter + Btk::AlignMiddle);
        p.draw_text("缓冲中", x() + w / 2.0f, y() + h / 2.0f);
    }
    if (player->media_status() == Btk::MediaStatus::LoadingMedia) {
        p.set_color(Btk::Color::White);
        p.set_font(font());
        p.set_text_align(Btk::AlignCenter + Btk::AlignMiddle);
        p.draw_text("加载中", x() + w / 2.0f, y() + h / 2.0f);
    }

    p.restore();
    return true;
}
void DanmakuView::play() {
    danmaku_dpi_scale = window_dpi().x / 96.0f;
    danmakus_iter = danmakus.begin();
    if (timerid == 0) {
        timerid = add_timer(1000 / fps);
    }
}
void DanmakuView::load(const Vec<Danmaku> &d) {
    danmakus = d;
    danmakus_iter = danmakus.begin();
}
void DanmakuView::pause(bool v) {
    if (v) {
        if (timerid != 0) {
            del_timer(timerid);
            timerid = 0;
        }
    }
    else {
        if (timerid == 0) {
            timerid = add_timer(1000 / fps);
        }
    }
}
void DanmakuView::stop() {
    if (timerid != 0) {
        del_timer(timerid);
        timerid = 0;
    }
    for (auto &plane : danmaku_regular_planes) {
        plane.clear();
    }
    danmaku_center_nodes.clear();
}
void DanmakuView::set_position(double pos) {
    for (auto &plane : danmaku_regular_planes) {
        plane.clear();
    }
    danmaku_center_nodes.clear();

    danmakus_iter = danmakus.begin();
    while (danmakus_iter->position < pos && danmakus_iter != danmakus.end()) {
        ++danmakus_iter;
    }
    if (danmakus_iter != danmakus.end()) {
        std::cout << "Seek to pos: " << danmakus_iter->position << " text: " << danmakus_iter->text << std::endl;
    }

    repaint();
}