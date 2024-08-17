#include <utility>

#include <gtest-all.cc>
#include <gtest_main.cc>

#include "power.hpp"

namespace wxpower
{
    TEST(TestBTH, TestAll)
    {
        EXPECT_EQ(binToHex(UINT8_C(0x0)), '0');
        EXPECT_EQ(binToHex(UINT8_C(0x1)), '1');
        EXPECT_EQ(binToHex(UINT8_C(0x2)), '2');
        EXPECT_EQ(binToHex(UINT8_C(0x3)), '3');
        EXPECT_EQ(binToHex(UINT8_C(0x4)), '4');
        EXPECT_EQ(binToHex(UINT8_C(0x5)), '5');
        EXPECT_EQ(binToHex(UINT8_C(0x6)), '6');
        EXPECT_EQ(binToHex(UINT8_C(0x7)), '7');
        EXPECT_EQ(binToHex(UINT8_C(0x8)), '8');
        EXPECT_EQ(binToHex(UINT8_C(0x9)), '9');
        EXPECT_EQ(binToHex(UINT8_C(0xa)), 'a');
        EXPECT_EQ(binToHex(UINT8_C(0xb)), 'b');
        EXPECT_EQ(binToHex(UINT8_C(0xc)), 'c');
        EXPECT_EQ(binToHex(UINT8_C(0xd)), 'd');
        EXPECT_EQ(binToHex(UINT8_C(0xe)), 'e');
        EXPECT_EQ(binToHex(UINT8_C(0xf)), 'f');
    }

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    TEST(Test_Bigint_V0, ZeroInit)
    {
        Bigint obj;
        u8* seed = obj.getBytes();

        for (size_t i = 0; i < 32; i++)
            EXPECT_EQ(seed[i], UINT8_C(0xff));
    }

    TEST(Test_Bigint_V0, Diff1)
    {
        Bigint obj;
        u8* seed = obj.getBytes();

        memset(seed, 0, 32);

        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(256));

        seed[0] = 0x80;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(0));

        seed[0] = 0x40;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(1));

        seed[0] = 0x20;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(2));

        seed[0] = 0x10;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(3));

        seed[0] = 0x08;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(4));

        seed[0] = 0x04;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(5));

        seed[0] = 0x02;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(6));

        seed[0] = 0x01;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(7));

        seed[0] = 0;
        seed[1] = 0x80;
        EXPECT_EQ(PowerV0::calcLZCDiff(obj), UINT32_C(8));
    }

    TEST(Test_Bigint_V0, Diff2)
    {
        Bigint obj;
        u8* seed = obj.getBytes();

        for (u32 i = 0; i < 32; i++)
            for (u32 j = 0; j < 8; j++)
            {
                memset(seed, 0, 32);

                seed[i] = UINT8_C(1) << (7 - j);

                const u32 expectedDiff = i * 8 + j;
                const u32 actualDiff = PowerV0::calcLZCDiff(obj);

                EXPECT_EQ(actualDiff, expectedDiff);
            }

        {
            memset(seed, 0, 32);

            const u32 expectedDiff = 256;
            const u32 actualDiff = PowerV0::calcLZCDiff(obj);

            EXPECT_EQ(actualDiff, expectedDiff);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    class TestB64Ctor32 : public ::testing::TestWithParam<
        std::tuple<u32, std::string>>
    {
    public:
        static const std::vector<std::tuple<u32, std::string>> tests;
    };

    const std::vector<std::tuple<u32, std::string>> TestB64Ctor32::tests =
    {
        {          0, "A" },
        {          1, "B" },
        {         12, "M" },
        {      0x37e, "-N" },
        {     0x3cb2, "yyD" },
        {    0x572db, "bLXB" },
        {   0x678ec9, "J74Z" },
        {  0xe67d28b, "LK9ZO" },
        { 0x57ce65ab, "rWmzXB" },
        { 0x89abcdef, "v38qJC" },
        { 0xffffffff, "_____D" }
    };

    TEST_P(TestB64Ctor32, TestProper)
    {
        const std::tuple<u32, std::string>& param = GetParam();
        const u32 val = std::get<0>(param);
        const std::string& expected = std::get<1>(param);

        Base64 obj(val);

        ASSERT_EQ(expected, obj.toString());
    }

    INSTANTIATE_TEST_SUITE_P(
        ParamTest,
        TestB64Ctor32,
        ::testing::ValuesIn(TestB64Ctor32::tests));

    class TestB64Ctor64 : public ::testing::TestWithParam<
        std::tuple<u64, std::string>>
    {
    public:
        static const std::vector<std::tuple<u64, std::string>> tests;
    };

    const std::vector<std::tuple<u64, std::string>> TestB64Ctor64::tests =
    {
        {                  0, "A" },
        {                  1, "B" },
        {                 12, "M" },
        {              0x37e, "-N" },
        {             0x3cb2, "yyD" },
        {            0x572db, "bLXB" },
        {           0x678ec9, "J74Z" },
        {          0xe67d28b, "LK9ZO" },
        {         0x57ce65ab, "rWmzXB" },
        {         0x89abcdef, "v38qJC" },
        {         0xffffffff, "_____D" },
        {        0x8bc41fd99, "Z2fQ8i" },
        {       0x8b4e3ea1cd, "NHqPOtI" },
        {      0xfeb832d1a9c, "cqRLDu-D" },
        {   0x1a2b3c4d5e6fee, "u_mXNxzKa" },
        { 0xf9e8d7c6b5a43210, "QIDp1a81onP" }
    };

    TEST_P(TestB64Ctor64, TestProper)
    {
        const std::tuple<u64, std::string>& param = GetParam();
        const u64 val = std::get<0>(param);
        const std::string& expected = std::get<1>(param);

        Base64 obj(val);

        ASSERT_EQ(expected, obj.toString());
    }

    INSTANTIATE_TEST_SUITE_P(
        ParamTest,
        TestB64Ctor64,
        ::testing::ValuesIn(TestB64Ctor64::tests));

    TEST(TestB64, Incr)
    {
        Base64 obj;

        EXPECT_EQ("A", obj.toString());

        obj.incr();
        EXPECT_EQ("B", obj.toString());

        obj.incr();
        EXPECT_EQ("C", obj.toString());

        obj.incr();
        EXPECT_EQ("D", obj.toString());

        for (u32 i = 0; i < 60; i++)
            obj.incr();

        EXPECT_EQ("_", obj.toString());

        obj.incr();
        EXPECT_EQ("AB", obj.toString());

        for (u32 i = 0; i < 63; i++)
            obj.incr();

        EXPECT_EQ("_B", obj.toString());

        obj.incr();
        EXPECT_EQ("AC", obj.toString());


        for (u32 i = 0; i < (62 * 64 - 1); i++)
            obj.incr();

        EXPECT_EQ("__", obj.toString());

        obj.incr();
        EXPECT_EQ("AAB", obj.toString());
    }

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    TEST(TestSha256, SanityChecks)
    {
        Sha256 obj;

        std::string toHash = "Hello world!";
        Bigint ret = obj.doHash(toHash.c_str(), static_cast<u32>(toHash.size()));

        ASSERT_STREQ(
            ret.toString().c_str(),
            "c0535e4be2b79ffd93291305436bf889314e4a3faec05ecffcbb7df31ad9e51a");

        toHash = "304";
        ret = obj.doHash(toHash.c_str(), static_cast<u32>(toHash.size()));

        ASSERT_STREQ(
            ret.toString().c_str(),
            "d874e4e4a5df21173b0f83e313151f813bea4f488686efe670ae47f87c177595");
    }

    class Test_PowerV0 : public PowerV0
    {
    public:
        bool msgToContent(ProofContent& rxParams, const std::string& msg)
        {
            return PowerV0::msgToContent(rxParams, msg);
        }

        void trimBody(std::string& body)
        {
            PowerV0::trimBody(body);
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    class Test_PowerV0_msgToContent : public ::testing::TestWithParam<
        std::tuple<std::string, bool, PowerV0::ProofContent>>
    {
    public:
        static const std::vector<std::tuple<std::string, bool, PowerV0::ProofContent>> tests;
        static const PowerV0::ProofContent emptyProof;
    };

    const PowerV0::ProofContent Test_PowerV0_msgToContent::emptyProof;

    const std::vector<std::tuple<std::string, bool, PowerV0::ProofContent>> Test_PowerV0_msgToContent::tests =
    {
        {
            "",
            false,
            emptyProof
        },
        {
            "Hello world!",
            false,
            emptyProof
        },
        {
            "Hello world!wxPoW0|",
            false,
            emptyProof
        },
        {
            "Hello world!|wxPoW0|",
            false,
            emptyProof
        },
        {
            "Hello world!wxPoW|0|",
            false,
            emptyProof
        },
        {
            "Hello world!|wxPoW0||",
            true,
            { "Hello world!", 0, "", "" }
        },
        {
            "Hello world!|wxPoW0|qwerty|",
            true,
            { "Hello world!", 0, "qwerty", "" }
        },
        {
            "Hello world!|wxPoW0|qwerty|uiop",
            true,
            { "Hello world!", 0, "qwerty", "uiop" }
        },
        {
            "Hello world!|wxPoW00|qwerty|uiop",
            false,
            emptyProof
        },
        {
            "Hello world!|wxPoW1|qwerty|uiop",
            false,
            emptyProof
        },
        {
            " Hello world!|wxPoW0|qwerty|uiop",
            true,
            { "Hello world!", 0, "qwerty", "uiop" }
        },
        {
            " Hello world!|wxPoW1|qwerty|uiop",
            false,
            emptyProof
        },
        {
            "|wxPoW0|Hello world!|wxPoW0|qwerty|uiop",
            true,
            { "|wxPoW0|Hello world!", 0, "qwerty", "uiop" }
        },
        {
            "wxPoW0Hello world!|wxPoW0|qwerty|uiop",
            true,
            { "wxPoW0Hello world!", 0, "qwerty", "uiop" }
        },
        {
            "    |wxPoW0|    |wxPoW0|    |wxPoW0|Hello world!|wxPoW0|qwerty|uiop",
            true,
            { "|wxPoW0|    |wxPoW0|    |wxPoW0|Hello world!", 0, "qwerty", "uiop" }
        },
    };

    TEST_P(Test_PowerV0_msgToContent, TestProper)
    {
        const std::tuple<std::string, bool, PowerV0::ProofContent>& param = GetParam();
        const std::string& msg = std::get<0>(param);
        const bool expectedRet = std::get<1>(param);
        const PowerV0::ProofContent& expectedProof = std::get<2>(param);

        PowerV0::ProofContent actualProof;
        Test_PowerV0 power;
        const bool actualRet = power.msgToContent(actualProof, msg);

        EXPECT_EQ(expectedRet, actualRet);
        EXPECT_EQ(expectedProof.body, actualProof.body);
        EXPECT_EQ(expectedProof.version, actualProof.version);
        EXPECT_EQ(expectedProof.userId, actualProof.userId);
        EXPECT_EQ(expectedProof.context, actualProof.context);
    }

    INSTANTIATE_TEST_SUITE_P(
        ParamTest,
        Test_PowerV0_msgToContent,
        ::testing::ValuesIn(Test_PowerV0_msgToContent::tests));

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    class Test_PowerV0_trimBody : public ::testing::TestWithParam<
        std::pair<std::string, std::string>>
    {
    public:
        static const std::vector<std::pair<std::string, std::string>> tests;
    };

    const std::vector<std::pair<std::string, std::string>> Test_PowerV0_trimBody::tests =
    {
        {
            "",
            ""
        },
        {
            " ",
            ""
        },
        {
            "\t ",
            ""
        },
        {
            "  \r\t ",
            ""
        },
        {
            "hello world!",
            "hello world!"
        },
        {
            " hello world!",
            "hello world!"
        },
        {
            "hello world! ",
            "hello world!"
        },
        {
            " hello world! ",
            "hello world!"
        },
        {
            "\thello world!",
            "hello world!"
        },
        {
            "hello world!\t",
            "hello world!"
        },
        {
            "\thello world!\t",
            "hello world!"
        },
        {
            "\n\thello world!",
            "hello world!"
        },
        {
            "hello world!\n\t",
            "hello world!"
        },
        {
            "\n\thello world!\n\t",
            "hello world!"
        },
        {
            " \r\n\t hello world!",
            "hello world!"
        },
        {
            "hello world! \r\n\t ",
            "hello world!"
        },
        {
            " \r\n\t hello world! \r\n\t ",
            "hello world!"
        },
        {
            " \r\n\t hello\t world!",
            "hello\t world!"
        },
        {
            "hello\t world! \r\n\t ",
            "hello\t world!"
        },
        {
            " \r\n\t hello\t world! \r\n\t ",
            "hello\t world!"
        },
        {
            " \r\n\t hello\t world! \r\n\t hello\t world! \r\n\t hello\t world! \r\n\t ",
            "hello\t world! \r\n\t hello\t world! \r\n\t hello\t world!"
        },

        /* Japanese hiragana and whitespace examples */
        {
            " Hello world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81\xe3\x80\x80",
            "Hello world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            " Hello world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81\xe3\x80\x80",
            "Hello world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            " Hello world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
            "Hello world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            " Hello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
            "Hello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            "\xe3\x80\x80Hello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
            "Hello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            "\xe3\x80\x80Hello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81\xe3\x80\x80\xe3\x80\x80",
            "Hello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            "\xc2\x85" "qweHello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81\xc2\x85",
            "qweHello\xe3\x80\x80world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            "\xe3\x80\x80\xc2\x85" "asdHello\xc2\x85world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81\xe3\x80\x80\xe3\x80\x80\xc2\x85\xe3\x80\x80",
            "asdHello\xc2\x85world!\xe3\x80\x80\xe3\x81\x93\xe3\x82\x93\xe3\x81"
            "\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81",
        },
        {
            "\xe3\x80\x80\xc2\x85",
            ""
        },
        {
            " \xe3\x80\x80 \xc2\x85  ",
            ""
        },
        {
            "\xe3\x80\x80\xc2\x85\t   \xc2\xa0",
            ""
        },
        {
            " \xc2\xa0 \r\t ",
            ""
        },
    };

    TEST_P(Test_PowerV0_trimBody, TestProper)
    {
        const std::pair<std::string, std::string>& param = GetParam();
        std::string body = std::get<0>(param);
        const std::string& expected = std::get<1>(param);

        Test_PowerV0 power;
        power.trimBody(body);

        ASSERT_EQ(expected, body);
    }

    INSTANTIATE_TEST_SUITE_P(
        ParamTest,
        Test_PowerV0_trimBody,
        ::testing::ValuesIn(Test_PowerV0_trimBody::tests));
}
