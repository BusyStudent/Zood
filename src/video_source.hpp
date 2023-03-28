#pragma once

#include <memory>
#include <Btk/string.hpp>
#include "stl.hpp"

using namespace BTK_NAMESPACE;

class VideoCollection {
    public:
        virtual ~VideoCollection() {}
};

class VideoProvider {
    public:
        virtual ~VideoProvider() { }
        virtual Ptr<VideoCollection> search_bangumi(u8string_view) = 0;
        virtual u8string             name()                        = 0;
};

Vec<Ptr<VideoProvider>> CreateProviders();
void                    RegisterProviders();