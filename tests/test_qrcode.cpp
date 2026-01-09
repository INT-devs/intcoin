// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/qrcode.h>
#include <gtest/gtest.h>
#include <string>

using namespace intcoin;

TEST(QRCodeTest, GenerateBasicQRCode) {
    std::string test_data = "Hello, INTcoin!";
    auto result = QRCode::Generate(test_data, QRCode::ECLevel::MEDIUM);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->width, 0);
    EXPECT_EQ(result->modules.size(), result->width * result->width);
}

TEST(QRCodeTest, GenerateAddressQRCode) {
    std::string address = "int1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080";
    auto result = QRCode::GenerateAddress(address);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->width, 0);
}

TEST(QRCodeTest, GenerateAddressWithAmountAndLabel) {
    std::string address = "int1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080";
    double amount = 1.5;
    std::string label = "Payment for services";

    auto result = QRCode::GenerateAddress(address, amount, label);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->width, 0);
}

TEST(QRCodeTest, GenerateLightningInvoice) {
    std::string invoice = "lnbc15u1p3xnhl2pp5jptserfk3zk4qy42tlucycrfwxhydvlemu9pqr93tuzlv9cc7g3sdqsvfhkcap3xyhx7un8cqzpgxqzjcsp5f8c52y2stc300gl6s4xswtjpc37hrnnr3c9wvtgjfuvqmpm35evq9qyyssqy4lgd8tj637qcjp05rdpxxykjenthxftej7a2zzmwrmrl70fyj9hvj0rewhzj7jfyuwkwcg9g2jpwtk3wkjtwnkdks84hsnu8xps5vsq4gj5hs";

    auto result = QRCode::GenerateLightningInvoice(invoice);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->width, 0);
}

TEST(QRCodeTest, GenerateSVG) {
    std::string test_data = "INT";
    auto result = QRCode::GenerateSVG(test_data, 4, 4, QRCode::ECLevel::HIGH);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->find("<svg") != std::string::npos);
    EXPECT_TRUE(result->find("</svg>") != std::string::npos);
}

TEST(QRCodeTest, GetModuleAccess) {
    std::string test_data = "Test";
    auto result = QRCode::Generate(test_data);

    ASSERT_TRUE(result.has_value());

    // Test boundary access
    EXPECT_FALSE(result->GetModule(-1, 0));  // Out of bounds
    EXPECT_FALSE(result->GetModule(0, -1));  // Out of bounds
    EXPECT_FALSE(result->GetModule(result->width, 0));  // Out of bounds
    EXPECT_FALSE(result->GetModule(0, result->width));  // Out of bounds

    // Valid access (should not crash)
    bool module = result->GetModule(0, 0);
    (void)module;  // Suppress unused variable warning
}

TEST(QRCodeTest, DifferentECLevels) {
    std::string test_data = "INTcoin";

    auto low = QRCode::Generate(test_data, QRCode::ECLevel::LOW);
    auto medium = QRCode::Generate(test_data, QRCode::ECLevel::MEDIUM);
    auto quartile = QRCode::Generate(test_data, QRCode::ECLevel::QUARTILE);
    auto high = QRCode::Generate(test_data, QRCode::ECLevel::HIGH);

    ASSERT_TRUE(low.has_value());
    ASSERT_TRUE(medium.has_value());
    ASSERT_TRUE(quartile.has_value());
    ASSERT_TRUE(high.has_value());

    // All should succeed
    EXPECT_GT(low->width, 0);
    EXPECT_GT(medium->width, 0);
    EXPECT_GT(quartile->width, 0);
    EXPECT_GT(high->width, 0);
}

TEST(QRCodeTest, EmptyString) {
    auto result = QRCode::Generate("", QRCode::ECLevel::MEDIUM);
    EXPECT_FALSE(result.has_value());
}

TEST(QRCodeTest, LargeData) {
    // Test with large data (should still work)
    std::string large_data(1000, 'A');
    auto result = QRCode::Generate(large_data, QRCode::ECLevel::LOW);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->width, 0);
}

TEST(QRCodeTest, GetRecommendedECLevel) {
    // Small data should use HIGH
    EXPECT_EQ(QRCode::GetRecommendedECLevel(30), QRCode::ECLevel::HIGH);

    // Medium data should use QUARTILE
    EXPECT_EQ(QRCode::GetRecommendedECLevel(100), QRCode::ECLevel::QUARTILE);

    // Larger data should use MEDIUM
    EXPECT_EQ(QRCode::GetRecommendedECLevel(200), QRCode::ECLevel::MEDIUM);

    // Very large data should use LOW
    EXPECT_EQ(QRCode::GetRecommendedECLevel(500), QRCode::ECLevel::LOW);
}

TEST(QRCodeTest, GetCapacity) {
    // Version 1 should have some capacity
    EXPECT_GT(QRCode::GetCapacity(1, QRCode::ECLevel::MEDIUM), 0);

    // Higher versions should have more capacity
    EXPECT_GT(QRCode::GetCapacity(10, QRCode::ECLevel::MEDIUM),
              QRCode::GetCapacity(1, QRCode::ECLevel::MEDIUM));

    // Higher EC level should reduce capacity
    EXPECT_GT(QRCode::GetCapacity(10, QRCode::ECLevel::LOW),
              QRCode::GetCapacity(10, QRCode::ECLevel::HIGH));

    // Invalid version should return 0
    EXPECT_EQ(QRCode::GetCapacity(0, QRCode::ECLevel::MEDIUM), 0);
    EXPECT_EQ(QRCode::GetCapacity(41, QRCode::ECLevel::MEDIUM), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
