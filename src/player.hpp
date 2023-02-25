#include <Btk/plugins/media.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/widget.hpp>

#include "bilibili.hpp"

// 视频选择播放窗口
class VideoPlayer : public Btk::Widget {
    public:
        VideoPlayer(Bilibili &client, int season_id);
        ~VideoPlayer();

    protected:
        bool key_press(Btk::KeyEvent &) override;
        bool resize_event(Btk::ResizeEvent &) override;
        bool close_event(Btk::CloseEvent &) override;
    private:
        void on_ep_selected(Btk::ListItem *box);

        Bilibili &client;

        Btk::VideoWidget view{this};
        Btk::AudioDevice device;
        Btk::MediaPlayer player;
        Btk::ListBox eps_box{this};
        Btk::Slider slider{this, Btk::Horizontal};

        bool fullscreen = false;

        Vec<Eps> eps;
};