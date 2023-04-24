#pragma once

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <string>


// RAII wrapper
class HtmlDocoument {
    public:
        HtmlDocoument() = default;
        HtmlDocoument(htmlDocPtr p) : doc(p) { }
        HtmlDocoument(HtmlDocoument && d) {
            doc = d.release();
        } 
        ~HtmlDocoument() {
            reset();
        }

        void reset(htmlDocPtr nptr = nullptr) {
            if (doc) {
                xmlFreeDoc(doc);
            }
            doc = nptr;
        }
        htmlDocPtr release(htmlDocPtr nptr = nullptr) {
            htmlDocPtr tmp = doc; 
            doc = nptr; 
            return tmp; 
        }
        htmlDocPtr get() const noexcept {
            return doc;
        }
        htmlDocPtr operator ->() const noexcept {
            return doc;
        }

        HtmlDocoument &operator =(HtmlDocoument &&d) {
            reset(d.release());
            return *this;
        }

        operator bool() const noexcept {
            return doc;
        }
        

        static HtmlDocoument Parse(const std::string &v) {
            return htmlReadDoc(
                BAD_CAST v.c_str(),
                nullptr, "utf8", 
                HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET
            );
        }
    private:
        htmlDocPtr doc = nullptr;
};
class XPathContext {
    public:
        XPathContext(const HtmlDocoument &doc) {
            ctxt = xmlXPathNewContext(doc.get());
        }
        XPathContext(const XPathContext &&) = delete;
        ~XPathContext() {
            if (object) {
                xmlXPathFreeObject(object);
            }
            if (ctxt) {
                xmlXPathFreeContext(ctxt);
            }
        }

        xmlXPathObjectPtr eval(const char *str) {
            if (object) {
                xmlXPathFreeObject(object);
            }
            object = xmlXPathEvalExpression(BAD_CAST str, ctxt);
            return object;
        }
    private:
        xmlXPathContextPtr ctxt = nullptr;
        xmlXPathObjectPtr object = nullptr;
};