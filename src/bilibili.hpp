#pragma once

#include <Btk/painter.hpp>
#include <Btk/string.hpp>
#include <cpr/cpr.h>
#include <optional>
#include <future>

using Btk::u8string_view;
using Btk::u8string;
using Btk::StringList;
using Btk::Color;

template <typename T>
using Optional = std::optional<T>;
template <typename T>
using Result = std::optional<T>;
template <typename T>
using Vec = std::vector<T>;

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

// B站 分
class Eps {
    public:
        int id; //< EPID
        int cid; //< CID
        u8string bvid; //< BVID

        u8string title; //< 标题
        u8string cover; //< 封面
        u8string long_title; //< 长标题
};
// 一部剧
class Bangumi {
    public:
        u8string title; // 标题
        u8string cover; // 封面
        u8string url; // 链接

        int      season_id; //< 剧集ID
};

// Bilibili API Collection 
class Bilibili {
    public:
        Bilibili();
        ~Bilibili();

        /**
         * @brief 去查B站的番剧信息
         * 
         * @param what 
         * @return Result<u8string> 
         */
        Result<u8string> search(const u8string &what);

        /**
         * @brief 搜索番剧列表
         * 
         * @param what 
         * @return Result<Vec<Bangumi>>
         */
        Result<Vec<Bangumi>> search_bangumi(const u8string &what);

        /**
         * @brief 抓取剧集列表
         * 
         */
        Result<Vec<Eps>> fetch_eps(int season_id);
        /**
         * @brief 抓取B站的 视频链接
         * 
         * @param bvid 
         * @param cid 
         * @return Result<u8string> 播放地址
         */
        Result<u8string> fetch_video_url(const u8string &bvid, int cid);
        /**
         * @brief 抓取弹幕
         * 
         * @param cid 
         * @return Result<Vec<Danmaku>> 
         */
        Result<Vec<Danmaku>> fetch_danmaku(int cid);
    private:
        cpr::Cookies cookie;
};