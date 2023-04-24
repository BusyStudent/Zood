#include <Btk/plugins/media.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/combobox.hpp>
#include <Btk/widget.hpp>
#include <Btk/layout.hpp>
#include <list>

#include "video_source.hpp"
#include "bilibili.hpp"

using Btk::TextLayout;
using Btk::FRect;


class PlayerSettings;
class VideoPlayer;

class VideoPlayer : public Btk::Widget {
    public:
        VideoPlayer(Bilibili &client, Bangumi bangumi, const Vec<Eps> &ep);
        ~VideoPlayer();

    protected:
        bool key_press(KeyEvent &) override;
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
        bool close_event(CloseEvent &) override;
        bool mouse_wheel(WheelEvent &) override;
        bool mouse_release(MouseEvent &) override;
    private:
        void on_ep_selected(ListItem *box);
        void on_state_changed(MediaPlayer::State state);
        void relayout();

        Bilibili &client;

        VideoWidget view{this};
        AudioDevice device;
        MediaPlayer player;
        ListBox eps_box{this};
        Slider slider{this, Horizontal};
        Button setbtn{this, "设置"};
        DanmakuView danmaku_view{this, &player};

        bool fullscreen = false;
        bool paused = false;
        bool played = false;
        bool loading = false;
        bool fetched = false; //< 详情是否拿到

        Vec<Eps> eps;
        Bangumi bangumi;

        // 视频源的结果
        Vec<Ptr<VideoCollection>> video_source;
        Vec<Ptr<Video>>           videos;

        // 当前弹出的
        PlayerSettings            *settings = nullptr;
    friend class PlayerSettings;
};
class PlayerSettings : public Widget {
    public:
        PlayerSettings(VideoPlayer *player);

        Size size_hint() const override;
    protected:
        bool close_event(CloseEvent &event) override;

        BoxLayout layout {TopToBottom};
        VideoPlayer *player;

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