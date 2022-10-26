#include "intelligence_comp_v2.h"

#include "intelligence_is_v2/intelligence_query_v2.h"
#include "read_attribute_v2.h"
#include "cptest.h"
#include "singleton.h"
#include "config.h"
#include "config_component.h"
#include "mock/mock_messaging.h"
#include "mock/mock_mainloop.h"
#include "mock/mock_time_get.h"

using namespace std;
using namespace testing;
using namespace chrono;
using namespace Intelligence_IS_V2;

USE_DEBUG_FLAG(D_METRICS);

class IntelligenceComponentTestV2
        :
    public Test,
    Singleton::Consume<I_Intelligence_IS_V2>
{
public:
    IntelligenceComponentTestV2()
    {
        debug_output.clear();
        Debug::setNewDefaultStdout(&debug_output);
        Debug::setUnitTestFlag(D_METRICS, Debug::DebugLevel::TRACE);
        setConfiguration<bool>(false, string("metric"), string("fogMetricSendEnable"));

        EXPECT_CALL(
            mock_ml,
            addRecurringRoutine(I_MainLoop::RoutineType::System, chrono::microseconds(600000000), _, _, _))
                .WillRepeatedly(DoAll(SaveArg<2>(&routine), Return(0)));

        string offline_intel_path = cptestFnameInExeDir("offline_intelligence_files_v2");
        setConfiguration<string>(offline_intel_path, string("intelligence"), string("offline intelligence path"));

        intelligence.preload();
        intelligence.init();
    }

    ~IntelligenceComponentTestV2()
    {
        Debug::setNewDefaultStdout(&cout);
    }

    stringstream debug_output;
    StrictMock<MockMainLoop> mock_ml;
    NiceMock<MockTimeGet> mock_time;
    ::Environment env;
    ConfigComponent conf;
    StrictMock<MockMessaging> messaging_mock;
    IntelligenceComponentV2 intelligence;
    I_MainLoop::Routine routine;
};

class Profile
{
public:
    Profile() {}

    DataString getUser() const{ return user;}
    DataString getPhase() const{ return phase;}

    template <typename Archive>
    void
    serialize(Archive &ar)
    {
        try {
            ReadAttribute<DataString>("user", user).serialize(ar);
        } catch (const exception &e) {}
        try {
            ReadAttribute<DataString>("phase", phase).serialize(ar);
        } catch (const exception &e) {}
    }

private:
    DataString user;
    DataString phase;
};

TEST_F(IntelligenceComponentTestV2, fakeOnlineIntelligenceTest)
{
    I_Intelligence_IS_V2 *intell = Singleton::Consume<I_Intelligence_IS_V2>::by<IntelligenceComponentTestV2>();
    QueryRequest request(Condition::EQUALS, "category", "cloud", true);

    string response_str(
    "{\n"
    "  \"assetCollections\": [\n"
    "    {\n"
    "      \"schemaVersion\": 1,\n"
    "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
    "      \"assetTypeSchemaVersion\": 1,\n"
    "      \"permissionType\": \"tenant\",\n"
    "      \"permissionGroupId\": \"fake-online-test-group\",\n"
    "      \"name\": \"fake-online-test-asset\",\n"
    "      \"class\": \"workload\",\n"
    "      \"category\": \"cloud\",\n"
    "      \"family\": \"fake-online-test\",\n"
    "      \"mainAttributes\": {\n"
    "          \"ipv4Addresses\": \"1.1.1.1\",\n"
    "          \"phase\": \"testing\"\n"
    "      },\n"
    "      \"sources\": [\n"
    "        {\n"
    "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
    "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
    "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
    "          \"ttl\": 120,\n"
    "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
    "          \"confidence\": 500,\n"
    "          \"attributes\": {\n"
    "            \"phase\": \"testing\",\n"
    "            \"user\": \"Omry\",\n"
    "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
    "          }\n"
    "        }\n"
    "      ]\n"
    "    }\n"
    "  ],\n"
    "  \"status\": \"done\",\n"
    "  \"totalNumAssets\": 2,\n"
    "  \"cursor\": \"start\"\n"
    "}\n"
    );

    EXPECT_CALL(
        messaging_mock,
        sendMessage(true, _, I_Messaging::Method::POST, _, _, _, _, MessageTypeTag::INTELLIGENCE)
    ).WillOnce(Return(response_str));
    auto maybe_ans = intell->queryIntelligence<Profile>(request);
    EXPECT_TRUE(maybe_ans.ok());
    auto vec = maybe_ans.unpack();
    EXPECT_EQ(vec.size(), 1u);
    auto iter = vec.begin();
    EXPECT_EQ(iter->getData().begin()->getUser().toString(), "Omry");
    EXPECT_EQ(iter->getData().begin()->getPhase().toString(), "testing");
}

TEST_F(IntelligenceComponentTestV2, multiAssetsIntelligenceTest)
{
    I_Intelligence_IS_V2 *intell = Singleton::Consume<I_Intelligence_IS_V2>::by<IntelligenceComponentTestV2>();
    QueryRequest request(Condition::EQUALS, "category", "cloud", true);

    string response_str1(
    "{\n"
        "  \"assetCollections\": [\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset-1\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.1\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd0\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test1\",\n"
        "            \"user\": \"Omry\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"no\", \"name2\": \"one\" } ] }\n"
        "          }\n"
        "        },\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa01\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa01\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test1\",\n"
        "            \"user\": \"Max\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"every\", \"name2\": \"one\" } ] }\n"
        "          }\n"
        "        },\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa01\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa01\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test1\",\n"
        "            \"user\": \"Roy\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Aviv\", \"name2\": \"Cochavi\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    },\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset-2\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.2\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa02\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa02\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd2\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test2\",\n"
        "            \"user\": \"Daniel\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Omry\", \"name2\": \"David\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    },\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset-2\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.3\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa03\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa03\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd3\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test3\",\n"
        "            \"user\": \"Oren\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Omry\", \"name2\": \"David\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"status\": \"done\",\n"
        "  \"totalNumAssets\": 2\n"
    "}\n"
    );

    EXPECT_CALL(
        messaging_mock,
        sendMessage(true, _, I_Messaging::Method::POST, _, _, _, _, MessageTypeTag::INTELLIGENCE)
    ).WillOnce(Return(response_str1));
    auto maybe_ans = intell->queryIntelligence<Profile>(request);
    EXPECT_TRUE(maybe_ans.ok());
    auto vec = maybe_ans.unpack();
    EXPECT_EQ(vec.size(), 3);

    auto iter = vec.begin();

    auto asset_sources_vec = iter->getData();
    auto data_it = asset_sources_vec.begin();
    EXPECT_EQ(data_it->getUser().toString(), "Omry");
    EXPECT_EQ(data_it->getPhase().toString(), "fake online test1");

    data_it++;
    EXPECT_EQ(data_it->getUser().toString(), "Max");
    EXPECT_EQ(data_it->getPhase().toString(), "fake online test1");

    data_it++;
    EXPECT_EQ(data_it->getUser().toString(), "Roy");
    EXPECT_EQ(data_it->getPhase().toString(), "fake online test1");

    iter++;
    EXPECT_EQ(iter->getData().begin()->getUser().toString(), "Daniel");
    EXPECT_EQ(iter->getData().begin()->getPhase().toString(), "fake online test2");

    iter++;
    EXPECT_EQ(iter->getData().begin()->getUser().toString(), "Oren");
    EXPECT_EQ(iter->getData().begin()->getPhase().toString(), "fake online test3");
}

TEST_F(IntelligenceComponentTestV2, inProgressQueryTest)
{
    I_Intelligence_IS_V2 *intell = Singleton::Consume<I_Intelligence_IS_V2>::by<IntelligenceComponentTestV2>();
    QueryRequest request(Condition::EQUALS, "category", "cloud", true);

    string in_progress_response_str(
    "{\n"
        "  \"assetCollections\": [\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.1\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test\",\n"
        "            \"user\": \"Omry\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"no\", \"name2\": \"one\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"status\": \"inProgress\",\n"
        "  \"totalNumAssets\": 1,\n"
        "  \"cursor\": \"start\"\n"
    "}\n"
    );

    string done_response_str(
    "{\n"
        "  \"assetCollections\": [\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.1\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test\",\n"
        "            \"user\": \"Omry\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"no\", \"name2\": \"one\" } ] }\n"
        "          }\n"
        "        },\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229bb11\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7bbb11\",\n"
        "          \"assetId\": \"cb068860528cb6bfb000cc35e79f11aeefed2\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 600,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test\",\n"
        "            \"user\": \"Max\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"status\": \"done\",\n"
        "  \"totalNumAssets\": 2\n"
    "}\n"
    );

    EXPECT_CALL(
        messaging_mock,
        sendMessage(true, _, I_Messaging::Method::POST, _, _, _, _, MessageTypeTag::INTELLIGENCE)
    ).WillOnce(Return(in_progress_response_str)).WillOnce(Return(done_response_str));

    EXPECT_CALL(
        mock_ml,
        yield(true)
    );

    auto maybe_ans = intell->queryIntelligence<Profile>(request);
    EXPECT_TRUE(maybe_ans.ok());
    auto vec = maybe_ans.unpack();
    EXPECT_EQ(vec.size(), 1u);
    vector<AssetReply<Profile>>::iterator assets_iter = vec.begin();
    vector<SerializableAssetSource<Profile>>::const_iterator sources_iter = assets_iter->getSources().begin();
    EXPECT_EQ(sources_iter->getAttributes().begin()->getUser().toString(), "Omry");
    sources_iter++;
    EXPECT_EQ(sources_iter->getAttributes().begin()->getUser().toString(), "Max");
}

TEST_F(IntelligenceComponentTestV2, pagingQueryTest)
{
    I_Intelligence_IS_V2 *intell = Singleton::Consume<I_Intelligence_IS_V2>::by<IntelligenceComponentTestV2>();
    QueryRequest request(Condition::EQUALS, "category", "cloud", true);
    request.activatePaging();

    string paging_done_response_str(
    "{\n"
        "  \"assetCollections\": [\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.1\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test\",\n"
        "            \"user\": \"Omry\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
        "          }\n"
        "        },\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229bb11\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7bbb11\",\n"
        "          \"assetId\": \"cb068860528cb6bfb000cc35e79f11aeefed2\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 600,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test\",\n"
        "            \"user\": \"Max\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"status\": \"done\",\n"
        "  \"totalNumAssets\": 2,\n"
        "  \"cursor\": \"abcd\"\n"
    "}\n"
    );

    string paging_in_progress_response_str1(
    "{\n"
        "  \"assetCollections\": [\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset1\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test1\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.1\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test1\",\n"
        "            \"user\": \"Omry\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"status\": \"inProgress\",\n"
        "  \"totalNumAssets\": 2,\n"
        "  \"cursor\": \"abcd\"\n"
    "}\n"
    );

    string paging_in_progress_response_str2(
    "{\n"
        "  \"assetCollections\": [\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset1\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test1\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.1\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test1\",\n"
        "            \"user\": \"Omry\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    },\n"
        "    {\n"
        "      \"schemaVersion\": 1,\n"
        "      \"assetType\": \"workload-cloud-fake-online-test\",\n"
        "      \"assetTypeSchemaVersion\": 1,\n"
        "      \"permissionType\": \"tenant\",\n"
        "      \"permissionGroupId\": \"fake-online-test-group\",\n"
        "      \"name\": \"fake-online-test-asset2\",\n"
        "      \"class\": \"workload\",\n"
        "      \"category\": \"cloud\",\n"
        "      \"family\": \"fake-online-test2\",\n"
        "      \"mainAttributes\": {\n"
        "          \"ipv4Addresses\": \"1.1.1.2\",\n"
        "          \"phase\": \"testing\"\n"
        "      },\n"
        "      \"sources\": [\n"
        "        {\n"
        "          \"tenantId\": \"175bb55c-e36f-4ac5-a7b1-7afa1229aa00\",\n"
        "          \"sourceId\": \"54d7de10-7b2e-4505-955b-cc2c2c7aaa00\",\n"
        "          \"assetId\": \"50255c3172b4fb7fda93025f0bfaa7abefd1\",\n"
        "          \"ttl\": 120,\n"
        "          \"expirationTime\": \"2020-07-29T11:21:12.253Z\",\n"
        "          \"confidence\": 500,\n"
        "          \"attributes\": {\n"
        "            \"phase\": \"fake online test2\",\n"
        "            \"user\": \"Max\",\n"
        "            \"owners\": { \"names\": [ { \"name1\": \"Bob\", \"name2\": \"Alice\" } ] }\n"
        "          }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"status\": \"done\",\n"
        "  \"totalNumAssets\": 2,\n"
        "  \"cursor\": \"efgh\"\n"
    "}\n"
    );

    EXPECT_CALL(
        messaging_mock,
        sendMessage(true, _, I_Messaging::Method::POST, _, _, _, _, MessageTypeTag::INTELLIGENCE)
    ).WillOnce(Return(paging_in_progress_response_str1));

    request.setAssetsLimit(2);
    EXPECT_EQ(request.getAssetsLimit(), 2);
    auto maybe_ans1 = intell->queryIntelligence<Profile>(request);
    EXPECT_TRUE(maybe_ans1.ok());
    auto vec1 = maybe_ans1.unpack();
    EXPECT_EQ(vec1.size(), 1);
    EXPECT_EQ(request.isPagingFinished(), false);

    EXPECT_CALL(
        messaging_mock,
        sendMessage(true, _, I_Messaging::Method::POST, _, _, _, _, MessageTypeTag::INTELLIGENCE)
    ).WillOnce(Return(paging_in_progress_response_str2));

    auto maybe_ans2 = intell->queryIntelligence<Profile>(request);
    EXPECT_TRUE(maybe_ans2.ok());
    auto vec2 = maybe_ans2.unpack();
    EXPECT_EQ(vec2.size(), 2);
    EXPECT_EQ(request.isPagingFinished(), false);

    EXPECT_CALL(
        messaging_mock,
        sendMessage(true, _, I_Messaging::Method::POST, _, _, _, _, MessageTypeTag::INTELLIGENCE)
    ).WillOnce(Return(paging_done_response_str));
    auto maybe_ans3 = intell->queryIntelligence<Profile>(request);
    EXPECT_TRUE(maybe_ans3.ok());
    auto vec3 = maybe_ans3.unpack();
    EXPECT_EQ(vec3.size(), 1);
    EXPECT_EQ(request.isPagingFinished(), true);

    vector<AssetReply<Profile>>::iterator assets_iter = vec3.begin();
    vector<SerializableAssetSource<Profile>>::const_iterator sources_iter = assets_iter->getSources().begin();
    EXPECT_EQ(sources_iter->getAttributes().begin()->getUser().toString(), "Omry");
}

TEST_F(IntelligenceComponentTestV2, offlineIntelligenceTest)
{
    setConfiguration<bool>(true, string("intelligence"), string("offline intelligence only"));
    intelligence.init();

    I_Intelligence_IS_V2 *intell = Singleton::Consume<I_Intelligence_IS_V2>::by<IntelligenceComponentTestV2>();
    QueryRequest request(Condition::EQUALS, "ip", "1.2.3.4", true);
    auto maybe_ans = intell->queryIntelligence<Profile>(request);

    ASSERT_TRUE(maybe_ans.ok());
    vector<AssetReply<Profile>> vec = maybe_ans.unpack();
    vector<AssetReply<Profile>>::iterator assets_iter = vec.begin();
    EXPECT_EQ(assets_iter->getAssetSchemaVersion(), 1);
    EXPECT_EQ(assets_iter->getAssetType(), "workload-cloud-ip");
    EXPECT_EQ(assets_iter->getAssetTypeSchemaVersion(), 1);
    EXPECT_EQ(assets_iter->getAssetPermissionGroupId(), "offline-group-id");
    EXPECT_EQ(assets_iter->getAssetName(), "offline-asset");
    EXPECT_EQ(assets_iter->getAssetClass(), "workload");
    EXPECT_EQ(assets_iter->getAssetCategory(), "cloud");
    EXPECT_EQ(assets_iter->getAssetFamily(), "offline family");
    EXPECT_EQ(assets_iter->getAssetGroup(), "offline testing");
    EXPECT_EQ(assets_iter->getAssetOrder(), "");
    EXPECT_EQ(assets_iter->getAssetKind(), "");

    map<string, vector<string>> attributes_map = assets_iter->getMainAttributes();
    EXPECT_EQ(attributes_map["ip"].front(), "1.2.3.4");

    vector<SerializableAssetSource<Profile>>::const_iterator sources_iter = assets_iter->getSources().begin();
    EXPECT_EQ(sources_iter->getTenantId(), "175bb55c-e36f-4ac5-a7b1-7afa1229aa00");
    EXPECT_EQ(sources_iter->getSourceId(), "54d7de10-7b2e-4505-955b-cc2c2c7aaa00");
    EXPECT_EQ(sources_iter->getAssetId(), "50255c3172b4fb7fda93025f0bfaa7abefd1");
    EXPECT_EQ(sources_iter->getTTL(), chrono::seconds(120));
    EXPECT_EQ(sources_iter->getExpirationTime(), "2010-05-15T21:50:12.253Z");
    EXPECT_EQ(sources_iter->getConfidence(), 700);
    EXPECT_EQ(sources_iter->getAttributes().begin()->getUser().toString(), "Omry");
    EXPECT_EQ(sources_iter->getAttributes().begin()->getPhase().toString(), "offline test");

    sources_iter++;
    EXPECT_EQ(sources_iter->getTenantId(), "175bb55c-e36f-4ac5-a7b1-7afa1229bb11");
    EXPECT_EQ(sources_iter->getSourceId(), "54d7de10-7b2e-4505-955b-cc2c2c7bbb11");
    EXPECT_EQ(sources_iter->getAssetId(), "cb068860528cb6bfb000cc35e79f11aeefed2");
    EXPECT_EQ(sources_iter->getTTL(), chrono::seconds(120));
    EXPECT_EQ(sources_iter->getExpirationTime(), "2010-05-15T21:50:12.253Z");
    EXPECT_EQ(sources_iter->getConfidence(), 600);
    EXPECT_EQ(sources_iter->getAttributes().begin()->getUser().toString(), "Max");
    EXPECT_EQ(sources_iter->getAttributes().begin()->getPhase().toString(), "offline test");
}
