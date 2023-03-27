#include <Btk/plugins/media.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/widget.hpp>
#include <list>

#include "bilibili.hpp"

using Btk::TextLayout;
using Btk::FRect;


class VideoPlayer;


class VideoPlayer : public Btk::Widget {
    public:
        VideoPlayer(Bilibili &client, const Vec<Eps> &ep);
        ~VideoPlayer();

    protected:
        bool key_press(Btk::KeyEvent &) override;
        bool resize_event(Btk::ResizeEvent &) override;
        bool close_event(Btk::CloseEvent &) override;
        bool mouse_wheel(Btk::WheelEvent &) override;
    private:
        void on_ep_selected(Btk::ListItem *box);
        void on_state_changed(Btk::MediaPlayer::State state);

        Bilibili &client;

        Btk::VideoWidget view{this};
        Btk::AudioDevice device;
        Btk::MediaPlayer player;
        Btk::ListBox eps_box{this};
        Btk::Slider slider{this, Btk::Horizontal};
        DanmakuView danmaku_view{this, &player};

        bool fullscreen = false;
        bool paused = false;
        bool played = false;
        bool loading = false;

        Vec<Eps> eps;
    friend class DanmakuView;
};

// inline DanmakuView::DanmakuView(VideoPlayer *parent) : Widget(parent), player(parent) { 
//     auto pal = palette();
//     // 背景透明
//     pal.set_color(Btk::Palette::Window, Btk::Color::Transparent);
//     auto ft = font();
//     ft.set_size(32);
//     set_font(ft);

//     set_palette(pal);
// }