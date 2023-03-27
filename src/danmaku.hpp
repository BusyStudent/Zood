#pragma once

#include <Btk/plugins/media.hpp>
#include <Btk/painter.hpp>
#include <Btk/string.hpp>
#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>
#include "stl.hpp"

using namespace BTK_NAMESPACE;

class Danmaku {
    public:
        // 类型
        enum Type : int {
            Regular1 = 1,
            Regular2 = 2,
            Regular3 = 3,
            Bottom = 4,
            Top = 5,
            Reserve = 6,
            Advanced = 7,
            Code = 8,
            Bas = 9,
        } type;
    
        // 弹幕池
        enum Pool : int {
            RegularPool = 1,
            SubtitlePool = 2,
            SpecialPool = 3,
        } pool;
        

        // 大小
        enum Size : int {
            Small = 18,
            Medium = 25,
            Large = 32,
        } size;

        // 颜色
        Color color;
        Color shadow;

        // 文本
        u8string text;

        // 出现时间
        double position;

        // 屏蔽等级
        uint32_t level;

        bool is_regular() const noexcept {
            return type == Regular1 || type == Regular2 || type == Regular3;
        }
        bool is_bottom() const noexcept {
            return type == Bottom;
        }
        bool is_top() const noexcept {
            return type == Top;
        }
};

// 视频选择播放窗口
class DanmakuNode : public FRect {
    public:
        const Danmaku *d;
        TextLayout text;
};

/**
 * @brief 一个透明的控件来展示弹幕
 * 
 */
class DanmakuView : public Widget {
    public:
        DanmakuView(Widget *parent, MediaPlayer *player);
        ~DanmakuView();

        // 加载
        void load(const Vec<Danmaku> &dans);
        void play();
        void stop();
        void pause(bool v);
        // 移动
        void set_position(double pos);
    protected:
        bool timer_event(TimerEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool resize_event(ResizeEvent &) override;
    private:
        void add_danmaku(const Danmaku &dan);

        bool projection = true;
        bool outline = false;
        int  fps     = 60;
        int  spacing = 2; //上下两条轨道的间隔

        MediaPlayer *player;
        // 弹幕部分
        float danmakus_size = 0.8f; //< 弹幕字号
        float danmaku_dpi_scale = -1.0f; //< DPI 缩放字号
        Font  danmakus_font{"黑体", 10};
        Vec<Danmaku>           danmakus;
        Vec<Danmaku>::iterator danmakus_iter;

        // 普通的弹幕
        List<List<DanmakuNode>> danmaku_regular_planes;
        // 头顶的弹幕
        List<DanmakuNode> danmaku_center_nodes;
        
        timerid_t timerid = 0;
};