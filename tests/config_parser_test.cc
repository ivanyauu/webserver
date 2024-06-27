#include "gtest/gtest.h"
#include "config_parser.h"

TEST(NginxConfigParserTest, SimpleConfig) {
  NginxConfigParser parser;
  NginxConfig out_config;
  bool success = parser.Parse("test_configs/example_config", &out_config);
  EXPECT_TRUE(success);
}

class NginxConfigParserTestFixture : public::testing::Test {
protected:
  NginxConfigParser parser;
  NginxConfig out_config;
};

// Test to cover specific part of config_parser //
TEST(NginxConfigStatementTest, TestToStringWithChildBlock) {
  NginxConfig parent_config;

  std::unique_ptr<NginxConfig> child_config(new NginxConfig);
  child_config->statements_.emplace_back(new NginxConfigStatement);
  child_config->statements_.back()->tokens_ = {"child", "block"};

  NginxConfigStatement statement;
  statement.tokens_ = {"parent", "block"};
  statement.child_block_ = std::move(child_config);

  std::string expected_output = "parent block {\n  child block;\n}\n";

  EXPECT_EQ(statement.ToString(0), expected_output);
}

// Test to cover specific part of config_parser //
TEST(NginxConfigTest, ToString_SingleStatement) {
  NginxConfig config;

  config.statements_.emplace_back(std::make_unique<NginxConfigStatement>());
  config.statements_.back()->tokens_.push_back("server_name");
  config.statements_.back()->tokens_.push_back("example.com");

  EXPECT_EQ(config.ToString(), "server_name example.com;\n");
}

TEST_F(NginxConfigParserTestFixture, SimplePort) {
  bool success = parser.Parse("test_configs/example_config", &out_config);
  std::string port = out_config.FindPortNumber();
  EXPECT_EQ(port, "80");
}

TEST_F(NginxConfigParserTestFixture, SimpleNoPort) {
  bool success = parser.Parse("test_configs/example_config_no_port", &out_config);
  std::string port = out_config.FindPortNumber();
  EXPECT_EQ(port, "");
}

TEST_F(NginxConfigParserTestFixture, NestedPort) {
  bool success = parser.Parse("test_configs/config_nested_braces", &out_config);
  std::string port = out_config.FindPortNumber();
  EXPECT_EQ(port, "80");
}

TEST_F(NginxConfigParserTestFixture, NestedNoPort) {
  bool success = parser.Parse("test_configs/config_nested_braces_no_port", &out_config);
  std::string port = out_config.FindPortNumber();
  EXPECT_EQ(port, "");
}

TEST_F(NginxConfigParserTestFixture, ConfigWithComments) {
  bool success = parser.Parse("test_configs/config_with_comments", &out_config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigMissingSemicolon) {
  bool success = parser.Parse("test_configs/config_missing_semicolon", &out_config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigMissingStartBrace) {
  bool success = parser.Parse("test_configs/config_missing_start_brace", &out_config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigMissingEndBrace) {
  bool success = parser.Parse("test_configs/config_missing_end_brace", &out_config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ValidConfig) {
  bool success = parser.Parse("test_configs/example_config", &out_config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigWithEscapeCharaters) {
  bool success = parser.Parse("test_configs/config_escape_characters", &out_config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigExtraSemicolon) {
  bool success = parser.Parse("test_configs/config_extra_semicolon", &out_config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigMissingQuote) {
  bool success = parser.Parse("test_configs/config_missing_quote", &out_config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigMissingWhitespace) {
  bool success = parser.Parse("test_configs/config_missing_whitespace", &out_config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigMixedQuotes) {
  bool success = parser.Parse("test_configs/config_mixed_quotes", &out_config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigEmptyBraces) {
  bool success = parser.Parse("test_configs/config_empty_braces", &out_config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigNestedBraces) {
  bool success = parser.Parse("test_configs/config_nested_braces", &out_config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, GetBlockSuccess) {
  bool success = parser.Parse("test_configs/example_config", &out_config);
  EXPECT_TRUE(success);
  
  NginxConfig* block = out_config.GetBlock("server");
  EXPECT_NE(block, nullptr);
}

TEST_F(NginxConfigParserTestFixture, GetBlockDoesNotExist) {
  bool success = parser.Parse("test_configs/example_config", &out_config);
  EXPECT_TRUE(success);

  NginxConfig* block = out_config.GetBlock("nonexistent_block");
  EXPECT_EQ(nullptr, nullptr);
}

TEST_F(NginxConfigParserTestFixture, GetBlockStatement) {
  bool success = parser.Parse("test_configs/example_config", &out_config);
  EXPECT_TRUE(success);

  NginxConfig* block = out_config.GetBlock("foo");
  EXPECT_EQ(nullptr, nullptr);
}

TEST_F(NginxConfigParserTestFixture, GetBlockNotFirstToken) {
  bool success = parser.Parse("test_configs/config_multiple_tokens", &out_config);
  EXPECT_TRUE(success);

  NginxConfig* block = out_config.GetBlock("bar");
  EXPECT_EQ(nullptr, nullptr);
}

TEST_F(NginxConfigParserTestFixture, GetRequestHandlersNoServer) {
  bool success = parser.Parse("test_configs/config_single_statement", &out_config);
  EXPECT_TRUE(success);

  std::vector<HandlerConfig> block = out_config.GetRequestHandlers();
  EXPECT_EQ(block.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetRequestHandlersNoHandlers) {
  bool success = parser.Parse("test_configs/example_config", &out_config);
  EXPECT_TRUE(success);

  std::vector<HandlerConfig> block = out_config.GetRequestHandlers();
  EXPECT_EQ(block.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetRequestHandlersSuccess) {
  bool success = parser.Parse("test_configs/config_with_handlers", &out_config);
  EXPECT_TRUE(success);

  std::vector<HandlerConfig> block = out_config.GetRequestHandlers();
  EXPECT_EQ(block.size(), 2);
}

TEST_F(NginxConfigParserTestFixture, GetRequestHandlersAbsPathSuccess) {
  bool success = parser.Parse("test_configs/config_with_handlers_absolute_path", &out_config);
  EXPECT_TRUE(success);

  std::string abs_path = out_config.GetBlock("location", "/static")->GetRoot();
  EXPECT_EQ(abs_path, "/usr/src/projects/new-grad-ten-years-experience/tests/files");
}

TEST_F(NginxConfigParserTestFixture, GetRequestHandlersDuplicateLocations) {
  bool success = parser.Parse("test_configs/config_duplicate_locations", &out_config);
  EXPECT_TRUE(success);

  std::vector<HandlerConfig> block = out_config.GetRequestHandlers();
  EXPECT_EQ(block.size(), 0);
}
