#include <gtest/gtest.h>
#include "markdown_to_html.h"

TEST(MarkdownToHtmlTest, EmptyInput) {
    MarkdownToHtml converter;
    std::string markdown = "";
    std::string expectedHtml = "";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}

TEST(MarkdownToHtmlTest, SimpleText) {
    MarkdownToHtml converter;
    std::string markdown = "This is a simple text.";
    std::string expectedHtml = "<p>This is a simple text.</p>\n";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}

TEST(MarkdownToHtmlTest, Header) {
    MarkdownToHtml converter;
    std::string markdown = "# Header";
    std::string expectedHtml = "<h1>Header</h1>\n";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}

TEST(MarkdownToHtmlTest, Emphasis) {
    MarkdownToHtml converter;
    std::string markdown = "*Emphasized text*";
    std::string expectedHtml = "<p><em>Emphasized text</em></p>\n";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}

TEST(MarkdownToHtmlTest, List) {
    MarkdownToHtml converter;
    std::string markdown = "- Item 1\n- Item 2\n- Item 3";
    std::string expectedHtml = "<ul>\n<li>Item 1</li>\n<li>Item 2</li>\n<li>Item 3</li>\n</ul>\n";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}

TEST(MarkdownToHtmlTest, Link) {
    MarkdownToHtml converter;
    std::string markdown = "[Link](https://www.example.com)";
    std::string expectedHtml = "<p><a href=\"https://www.example.com\">Link</a></p>\n";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}

TEST(MarkdownToHtmlTest, CodeBlock) {
    MarkdownToHtml converter;
    std::string markdown = "```\n#include <iostream>\nint main() {\n    std::cout << \"Hello, world!\";\n    return 0;\n}\n```";
    std::string expectedHtml = "<pre><code>#include &lt;iostream&gt;\nint main() {\n    std::cout &lt;&lt; &quot;Hello, world!&quot;;\n    return 0;\n}\n</code></pre>\n";
    EXPECT_EQ(converter.convert(markdown), expectedHtml);
}
