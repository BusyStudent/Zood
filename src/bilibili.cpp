#include "bilibili.hpp"
#include <cpr/cpr.h>
#include <iostream>
#include <charconv>
#include <nlohmann/json.hpp>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>

static cpr::Header DefaultHeader() {
    cpr::Header header;
    header.insert({"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.114 Safari/537.36"});
    header.insert({"Referer", "https://www.bilibili.com/"});

    return header;
}

Bilibili::Bilibili() {
    auto response = cpr::Get(cpr::Url("https://bilibili.com/"), DefaultHeader());
    cookie = response.cookies;


    std::cout << response.status_code << std::endl;
    for (auto &[key, value] : cookie) {
        std::cout << key << " :" << value << std::endl;
    }
}
Bilibili::~Bilibili() {

}

Result<Vec<Bangumi>> Bilibili::search_bangumi(const u8string &name) {
    auto req_url = u8string::format("https://api.bilibili.com/x/web-interface/search/type?search_type=media_bangumi&keyword=%s/", cpr::util::urlDecode(name).c_str());

    // Get it
    auto response = cpr::Get(cpr::Url(req_url), DefaultHeader(), cookie);
    if (response.status_code != 200) {
        return std::nullopt;
    }

    // Parse
    try {
        nlohmann::json json = nlohmann::json::parse(response.text);
        if (json["code"] != 0) {
            // 错误
            return std::nullopt;
        }
        auto result = json["data"]["result"];

        Vec<Bangumi> list;

        for (auto &elem : result) {
            u8string us(elem["title"]);
            us.replace("\u003cem class=\"keyword\"\u003e", "");
            us.replace("\u003c/em\u003e", "");

            Bangumi bangumi;
            bangumi.title = us;
            bangumi.url = elem["url"];
            bangumi.cover = elem["cover"];
            bangumi.season_id = elem["season_id"];

            list.emplace_back(bangumi);
        }
        return list;
    }
    catch (std::exception &err) {
        std::cout << err.what() << std::endl;
        return std::nullopt;
    }
    catch (...) {
        return std::nullopt;
    }
}
Result<Vec<PixBuffer>> Bilibili::fetch_covers(const Vec<Bangumi> &ban) {
    Vec<PixBuffer> result;
    for (auto &e : ban) {
        try {
            auto response = cpr::Get(cpr::Url(e.cover), DefaultHeader());
            if (response.status_code != 200) {
                // 失败了 压入一个空的
                result.emplace_back();
                std::cout << "Failed to Get : " << e.cover << " Status code " << response.status_code << std::endl; 
                continue;
            }
            result.emplace_back(
                PixBuffer::FromMem(response.text.data(), response.text.size())
            );
        }
        catch (std::exception &err) {
            result.emplace_back();
            std::cout << "Failed to Get : " << e.cover << " What() " << err.what() << std::endl; 
        }
    }
    return result;
}
Result<Vec<Eps>> Bilibili::fetch_eps(int season_id) {
    auto url = u8string::format("https://api.bilibili.com/pgc/view/web/season?season_id=%d", season_id);
    auto response = cpr::Get(cpr::Url(url), DefaultHeader(), cookie);

    if (response.status_code != 200) {
        std::cout << "Failed to fetch eps" << std::endl;
        return std::nullopt;
    }

    try {
        nlohmann::json json = nlohmann::json::parse(response.text);
        if (json["code"] != 0) {
            // 错误
            return std::nullopt;
        }
        Vec<Eps> eps;
        for (auto &ep : json["result"]["episodes"]) {
            Eps ep_info;
            ep_info.long_title = ep["long_title"];
            ep_info.title = ep["title"];
            ep_info.cover = ep["cover"];
            ep_info.cid = ep["cid"];
            ep_info.bvid = ep["bvid"];
            ep_info.id = ep["id"];

            std::cout << ep_info.title << " long_title:" << ep_info.long_title << std::endl;
            std::cout << ep_info.bvid << " " << ep_info.cid << std::endl;

            eps.emplace_back(ep_info);
        }
        return eps;
    }
    catch (std::exception &err) {
        std::cout << err.what() << std::endl;
        return std::nullopt;
    }
    catch (...) {
        return std::nullopt;
    }

}

Result<u8string> Bilibili::fetch_video_url(const u8string &bvid, int cid) {
    auto url = u8string::format("https://api.bilibili.com/x/player/playurl?qn=64&cid=%d&bvid=%s", cid, bvid.c_str());
    auto response = cpr::Get(cpr::Url(url), DefaultHeader(), cookie);

    if (response.status_code != 200) {
        std::cout << "Failed to fetch video url" << std::endl;
        return std::nullopt;
    }

    try {
        nlohmann::json json = nlohmann::json::parse(response.text);
        if (json["code"] != 0) {
            // 错误
            return std::nullopt;
        }

        // 得到最好的流
        return u8string(json["data"]["durl"][0]["url"]);
    }
    catch (std::exception &err) {
        std::cout << err.what() << std::endl;
        return std::nullopt;
    }
    catch (...) {
        return std::nullopt;
    }
}
Result<Vec<Danmaku>> Bilibili::fetch_danmaku(int cid) {
    auto url = u8string::format("https://api.bilibili.com/x/v1/dm/list.so?oid=%d", cid);

    // I didnot known why cpr didnot support deflate
    auto response = cpr::Get(cpr::Url(url), DefaultHeader(), cookie);

    if (response.status_code!= 200) {
        std::cout << "Failed to fetch danmaku" << std::endl;
        return std::nullopt;
    }

    // Parse xml
    auto doc = xmlParseDoc(BAD_CAST response.text.c_str());

    if (!doc) {
        return std::nullopt;
    }
    xmlNodePtr cur = xmlDocGetRootElement(doc);

    // For each children
    Vec<Danmaku> danmakus;

    for (cur = cur->children; cur; cur = cur->next) {
        if (xmlStrcmp(cur->name, BAD_CAST "d")) {
            continue;
        }

        // Get attr
        auto p = xmlGetProp(cur, BAD_CAST "p");
        auto text = xmlNodeGetContent(cur);
        auto list = u8string_view(reinterpret_cast<const char*>(p)).split_ref(",");

        Danmaku d;
        auto res = std::from_chars(list[0].data(), list[0].data() + list[0].size(), d.position);
        res = std::from_chars(list[1].data(), list[1].data() + list[1].size(), (int &)d.type);
        res = std::from_chars(list[2].data(), list[2].data() + list[2].size(), (int&)d.size);

        int color_num;
        res = std::from_chars(list[3].data(), list[3].data() + list[3].size(), color_num);
        int r = 
            (color_num & 0xFF0000) >> 16;
        int g = 
            (color_num & 0xFF00) >> 8;
        int b = 
            (color_num & 0xFF);
        
        d.color = Color(r, g, b);
        d.shadow = Color::Gray;

        res = std::from_chars(list[5].data(), list[5].data() + list[5].size(), (int &)d.pool);

        res = std::from_chars(list[8].data(), list[8].data() + list[8].size(), (int &)d.level);

        d.text = reinterpret_cast<const char*>(text);

        danmakus.emplace_back(std::move(d));
    }

    // Sort danmaku by position
    std::sort(danmakus.begin(), danmakus.end(), [](const Danmaku &a,const Danmaku &b){
        return a.position < b.position;
    });

    xmlFreeDoc(doc);

    return danmakus;
}
Result<Vec<u8string>> Bilibili::fetch_search_suggests(const u8string &what) {
    auto url = u8string::format("https://s.search.bilibili.com/main/suggest?term=%s&main_ver=v1", cpr::util::urlEncode(what).c_str());

    auto response = cpr::Get(cpr::Url(url), DefaultHeader(), cookie);
    if (response.status_code != 200) {
        return std::nullopt;
    }

    // Parse
    try {
        auto json = nlohmann::json::parse(response.text);
        if (json["code"] != 0) {
            return std::nullopt;
        }
        if (json["result"].empty()) {
            return std::nullopt;
        }

        Vec<u8string> results;

        for (auto &item : json["result"]["tag"]) {
            results.emplace_back(item["value"]);
        }
        
        return results;
    }
    catch (std::exception &err) {
        std::cout << err.what() << std::endl;
        return std::nullopt;
    }
    catch (...) {
        return std::nullopt;
    }
}