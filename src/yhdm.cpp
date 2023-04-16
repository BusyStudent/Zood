#include "video_source.hpp"
#include <cpr/cpr.h>
#include <Btk/pixels.hpp>
#include <Btk/plugins/webview.hpp>
#include <Btk/plugins/barrier.hpp>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <iostream>

static cpr::Header DefaultHeader() {
    cpr::Header header;
    header.insert({"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.114 Safari/537.36"});
    header.insert({"Referer", "https://www.bilibili.com/"});

    return header;
}
class YhdmProvider;
class YhdmVideo : public Video {
    public:
        u8string title() override {
            return title_str;
        }
        Result<u8string> media_url() override;

        u8string url; //< 播放页面的URL
        u8string title_str; //< 小标题

        YhdmProvider *master;
};
class YhdmVideoCollection : public VideoCollection {
    public:
        Result<Vec<Ptr<Video>>> videos() override;
        Result<PixBuffer> covers() override {
            return std::nullopt;
        }
        u8string          title() override {
            return title_str;
        }


        u8string url; //< 对于那部分的URL
        u8string title_str; //< 标题
        u8string description;// 简介
        u8string cover_url;

        YhdmProvider *master = nullptr;
};
class YhdmProvider : public VideoProvider, public Object {
    public:
        YhdmProvider() {
            // webinf.reset(WebView::AllocHeadless());
            // webinf->signal_ready().connect([this]() {
            //     ready = true;
            // });
        }
        ~YhdmProvider() {}

        u8string hostname = "www.yhdmp.net";
        Ptr<WebInterface> webinf;
        bool ready = false;
        Latch ready_latch{1};

        Vec<Ptr<VideoCollection>> search_bangumi(u8string_view name) {
            Vec<Ptr<VideoCollection>> result;
            auto url = u8string::format("https://%s/s_all??ex=1&kw=%s", hostname.c_str(),cpr::util::urlEncode(name.copy()).c_str());

            // Get 
            cpr::Header header;
            auto response = cpr::Get(cpr::Url(url), DefaultHeader());
            if (response.status_code != 200) {
                return result;
            }
            // XPath exp //div[@class='lpic']/ul/li
            auto doc = htmlReadDoc(BAD_CAST response.text.c_str(), nullptr, "utf8", 
                HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET
            );
            if (!doc) {
                return result;
            }
            
            // Parse it
            auto xpath_ctx = xmlXPathNewContext(doc);
            if (!xpath_ctx) {
                xmlFreeDoc(doc);
                return result;
            }
            auto xpath_exp = BAD_CAST "//div[@class='lpic']/ul/li";
            auto xpath_result = xmlXPathEvalExpression(xpath_exp, xpath_ctx);
            if (!xpath_result) {
                xmlXPathFreeContext(xpath_ctx);
                xmlFreeDoc(doc);
                return result;
            }
            auto nodes = xpath_result->nodesetval;
            for (int i = 0; i < nodes->nodeNr; i++) {
                auto node = nodes->nodeTab[i];
                if (node->type != XML_ELEMENT_NODE) {
                    continue;
                }
                result.push_back(std::make_unique<YhdmVideoCollection>());
                auto cur = (YhdmVideoCollection*) result.back().get();
                cur->master = this;

                // 遍历孩子
                for (auto child = node->children; child != nullptr; child = child->next) {
                    if (xmlStrcmp(child->name, BAD_CAST "a") == 0) {
                        // Get herf as url
                        auto href = xmlGetProp(child, BAD_CAST "href");
                        cur->url = u8string::format("https://%s%s", hostname.c_str(), href);
                        xmlFree(href);

                        // Children is a image
                        auto c = child->children;
                        while (c) {
                            if (xmlStrcmp(c->name, BAD_CAST "img") == 0) {
                                auto u = xmlGetProp(c, BAD_CAST "src");
                                cur->cover_url = (const char *)u;
                                xmlFree(u);

                                u = xmlGetProp(c, BAD_CAST "alt");
                                cur->title_str = (const char *)u;
                                xmlFree(u);
                            }
                            c = c->next;
                        }
                    }
                    else if (xmlStrcmp(child->name, BAD_CAST "p") == 0) {
                        cur->description = (const char*) xmlNodeGetContent(child);
                    }
                }

                std::cout << cur->title_str << " " << cur->url << std::endl;

            }
            xmlXPathFreeContext(xpath_ctx);
            xmlXPathFreeObject(xpath_result);
            xmlFreeDoc(doc);

            return result;
        }
        u8string name() override {
            return "樱花动漫";
        }
};
Result<Vec<Ptr<Video>>> YhdmVideoCollection::videos() {
    Vec<Ptr<Video>> result;

    auto response = cpr::Get(cpr::Url(url), DefaultHeader());
    if (response.status_code!= 200) {
        return std::nullopt;
    }
    auto doc = htmlReadDoc(BAD_CAST response.text.c_str(), nullptr, "utf8", 
        HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET
    );
    if (!doc) {
        return std::nullopt;
    }
    // Parse it
    auto xpath_ctx = xmlXPathNewContext(doc);
    if (!xpath_ctx) {
        xmlFreeDoc(doc);
        return result;
    }
    auto xpath_exp = BAD_CAST "//div[@class='movurl']/ul/li/a";
    auto xpath_result = xmlXPathEvalExpression(xpath_exp, xpath_ctx);
    if (!xpath_result) {
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return result;
    }
    auto nodes = xpath_result->nodesetval;
    for (int i = 0; i < nodes->nodeNr; i++) {
        auto node = nodes->nodeTab[i];
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }
        result.push_back(std::make_unique<YhdmVideo>());
        auto cur = (YhdmVideo*) result.back().get();
        // cur->master = this;

        // 遍历孩子
        auto href = xmlGetProp(node, BAD_CAST "href");
        cur->title_str = (const char*)xmlNodeGetContent(node);
        cur->url = u8string::format("https://%s%s", master->hostname.c_str(), href);
        cur->master = master;

        xmlFree(href);

        std::cout << cur->title_str << " " << cur->url << std::endl;
    }
    xmlXPathFreeContext(xpath_ctx);
    xmlXPathFreeObject(xpath_result);
    xmlFreeDoc(doc);

    return result;

}
Result<u8string> YhdmVideo::media_url() {
    if (!master->ready) {
        master->defer_call([this]() {
            auto webview = new WebView;
            webview->show();
            master->webinf.reset(webview->interface());
            master->webinf->signal_ready().connect([this]() {
                // 环境完成
                master->webinf->signal_navigation_starting().connect([]() {
                    std::cout << u8string_view("开始加载") << std::endl;
                });
                    master->webinf->signal_navigation_compeleted().connect([]() {
                    std::cout << u8string_view("加载结束") << std::endl;
                });

                master->ready = true;
                master->ready_latch.count_down();
            });
        });
        master->ready_latch.wait();
    }
    // 准备查询 在主线程
    Latch latch {1};
    Result<u8string> result;
    pointer_t token;
    master->defer_call([&, this]() {
        auto timer = new Timer;
        timer->set_repeat(false);
        timer->set_interval(5000);
        timer->start();

        master->webinf->navigate(url);
        timer->signal_timeout().connect([&, this]() {
            master->defer_call([&, this]() {
                BTK_LOG("超时");

                master->webinf->navigate("about:blank");
                master->webinf->interop()->del_request_watcher(token);
                latch.count_down();

                delete timer;
            });
        });
        token = master->webinf->interop()->add_request_watcher([&, this](u8string_view url) {
            if (url.contains(".m3u8") && !url.contains("getplay_url=")) {
                // Got
                result = url;

                master->defer_call([&, this]() {
                    master->webinf->navigate("about:blank");
                    master->webinf->interop()->del_request_watcher(token);
                    latch.count_down();

                    delete timer;
                });
            }
        });
    });
    latch.wait();
    return result;
}

BTK_INITCALL([]() {
    RegisterProvider<YhdmProvider>();
});