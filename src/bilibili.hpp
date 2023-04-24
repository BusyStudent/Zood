#pragma once

#include <Btk/painter.hpp>
#include <Btk/string.hpp>
#include <cpr/cpr.h>
#include <optional>
#include <future>

#include "danmaku.hpp"
#include "stl.hpp"

using Btk::u8string_view;
using Btk::u8string;
using Btk::StringList;
using Btk::Color;



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
         * @brief 抓取番剧的封面
         * 
         * @return Result<Vec<PixBuffer>> 
         */
        Result<Vec<PixBuffer>> fetch_covers(const Vec<Bangumi> &ban);

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
        /**
         * @brief 抓取搜索建议
         * 
         */
        Result<Vec<u8string>> fetch_search_suggests(const u8string &str);
    private:
        cpr::Cookies cookie;
};