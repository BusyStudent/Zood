#include "bilibili.hpp"
#include <cpr/cpr.h>
#include <iostream>
#include <nlohmann/json.hpp>

cpr::Header DefaultHeader() {
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