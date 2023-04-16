#pragma once

#include <memory>
#include <Btk/string.hpp>
#include "stl.hpp"

using namespace BTK_NAMESPACE;

class Video {
    public:
        virtual ~Video() {}
        /**
         * @brief 短标题
         * 
         * @return u8string 
         */
        virtual u8string title() = 0;
        /**
         * @brief 媒体URL
         * 
         * @return u8string 
         */
        virtual Result<u8string> media_url() = 0;
};
/**
 * @brief 一部番剧的信息接口
 * 
 */
class VideoCollection {
    public:
        virtual ~VideoCollection() {}
        virtual u8string          title() = 0;
        virtual Result<PixBuffer> covers() = 0;
        virtual Result<Vec<Ptr<Video>>> videos() = 0;
};

class VideoProvider {
    public:
        virtual ~VideoProvider() { }
        /**
         * @brief 搜索番剧 可能返回0 到多个结果 (堵塞)
         * 
         * @return Vec<Ptr<VideoCollection>> 
         */
        virtual Vec<Ptr<VideoCollection>> search_bangumi(u8string_view) = 0;
        /**
         * @brief 当前这个提供者的信息
         * 
         * @return u8string 
         */
        virtual u8string                  name()                        = 0;
};

Vec<VideoProvider*> &   GetProviders();
void                    RegisterProvider(VideoProvider *(*)());
template <typename T>
void                    RegisterProvider() {
    RegisterProvider([]() -> VideoProvider* {
        return new T;
    });
}