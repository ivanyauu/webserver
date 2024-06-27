#include "markdown_to_html.h"
#include <cmark.h>

std::string MarkdownToHtml::convert(const std::string& markdown) {
    cmark_node *document = cmark_parse_document(markdown.c_str(), markdown.size(), CMARK_OPT_DEFAULT);
    char *html = cmark_render_html(document, CMARK_OPT_DEFAULT);
    std::string result(html);
    cmark_node_free(document);
    free(html);
    return result;
}
