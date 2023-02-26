#include <Btk/plugins/media.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/widget.hpp>
#include <list>

#include "bilibili.hpp"

using Btk::TextLayout;
using Btk::FRect;

template <typename T>
using List = std::list<T>;

// 视频选择播放窗口
class DanmakuNode : public FRect {
    public:
        const Danmaku *d;
        TextLayout text;
};

// 展示弹幕的一个View
class DanmakuView : public Btk::Widget {
    public:
        DanmakuView(Widget *parent, std::list<DanmakuNode> *n) : Widget(parent), nodes(n) { 
            auto pal = palette();
            // 背景透明
            pal.set_color(Btk::Palette::Window, Btk::Color::Transparent);

            set_palette(pal);
        }
    private:
        bool paint_event(Btk::PaintEvent &) override;

        std::list<DanmakuNode> *nodes;
};

class VideoPlayer : public Btk::Widget {
    public:
        VideoPlayer(Bilibili &client, int season_id);
        ~VideoPlayer();

    protected:
        bool key_press(Btk::KeyEvent &) override;
        bool resize_event(Btk::ResizeEvent &) override;
        bool close_event(Btk::CloseEvent &) override;
        bool timer_event(Btk::TimerEvent &) override;
    private:
        void on_ep_selected(Btk::ListItem *box);
        void danmaku_start();
        void danmaku_stop();
        void danmaku_seek(double where);

        Bilibili &client;

        Btk::VideoWidget view{this};
        Btk::AudioDevice device;
        Btk::MediaPlayer player;
        Btk::ListBox eps_box{this};
        Btk::Slider slider{this, Btk::Horizontal};
        DanmakuView danmaku_view{this, &danmaku_nodes};

        bool fullscreen = false;
        bool paused = false;
        bool played = false;

        Vec<Eps> eps;
        Vec<Danmaku> danmakus;


        // 弹幕部分
        bool cur_danmakus_added = false;
        Vec<Danmaku>::iterator danmakus_iter;
        List<DanmakuNode> danmaku_nodes;
        Btk::timerid_t timerid = 0;
};