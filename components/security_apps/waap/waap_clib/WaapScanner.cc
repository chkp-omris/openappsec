// Copyright (C) 2022 Check Point Software Technologies Ltd. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "WaapScanner.h"
#include "WaapScores.h"
#include "i_transaction.h"
#include <string>
#include "debug.h"
#include "reputation_features_events.h"

USE_DEBUG_FLAG(D_WAAP_SCANNER);

double Waap::Scanner::getScoreData(Waf2ScanResult& res, const std::string &poolName)
{
    std::string source = m_transaction->getSourceIdentifier();

    // Extract set of keyword_matches from keyword_matches, then from ngtags
    Waap::Keywords::KeywordsSet keywordsSet;
    Waap::Keywords::computeKeywordsSet(keywordsSet, res.keyword_matches, res.found_patterns);

    std::string param_name = IndicatorsFiltersManager::generateKey(res.location, res.param_name, m_transaction);
    dbgTrace(D_WAAP_SCANNER) << "filter processing for parameter: " << param_name;
    m_transaction->getAssetState()->logIndicatorsInFilters(param_name, keywordsSet, m_transaction);
    m_transaction->getAssetState()->filterKeywords(param_name, keywordsSet, res.filtered_keywords);
    if (m_transaction->getSiteConfig() != nullptr)
    {
        auto waapParams = m_transaction->getSiteConfig()->get_WaapParametersPolicy();
        if (waapParams != nullptr && waapParams->getParamVal("filtersVerbose", "false") == "true") {
            m_transaction->getAssetState()->filterVerbose(param_name, res.filtered_keywords);
        }
    }
    m_transaction->getAssetState()->filterKeywordsByParameters(res.param_name, keywordsSet);

    // The keywords are only removed in production, they are still used while building scores
    if (!m_transaction->get_ignoreScore()) {
        m_transaction->getAssetState()->removeKeywords(keywordsSet);
    }

    // Filter keywords due to wbxml data format
    DeepParser &dp = m_transaction->getDeepParser();
    bool isBrokenWBXML = (m_transaction->getContentType() == Waap::Util::CONTENT_TYPE_WBXML) && (dp.depth() == 0) &&
        (dp.m_key.first().size() == 4 && dp.m_key.first() == "body" && !dp.isWBXmlData());

    // If wbxml data detected heuristically, or if not detected but declared by content-type in header
    if (dp.isWBXmlData() || isBrokenWBXML) {
        dbgTrace(D_WAAP_SCANNER) << "Filtering out wbxml keywords. isWbXmlData: " << dp.isWBXmlData() <<
            ", isBrokenWBXml:" << isBrokenWBXML;
        m_transaction->getAssetState()->removeWBXMLKeywords(keywordsSet, res.filtered_keywords);
    }

    // update keywords_matches
    res.keyword_matches.clear();
    for (auto keyword : keywordsSet) {
        res.keyword_matches.push_back(keyword);
    }
    std::sort(res.keyword_matches.begin(), res.keyword_matches.end());

    std::string keywords_string;
    for (auto pKeyword = keywordsSet.begin(); pKeyword != keywordsSet.end(); ++pKeyword) {
        // Add spaces between the items, but not before the first one
        if (pKeyword != keywordsSet.begin()) {
            keywords_string += " ";
        }

        std::string k = *pKeyword;
        stripSpaces(k);
        keywords_string += k;
    }

    std::vector<std::string> newKeywords;
    for (auto pKeyword = keywordsSet.begin(); pKeyword != keywordsSet.end(); ++pKeyword) {
        std::string k = *pKeyword;
        stripSpaces(k);
        // if keyword_string.count(key) < 2: new_keywords.append(key)
        if (countSubstrings(keywords_string, k) < 2) {
            newKeywords.push_back(k);
        }
    }

    std::sort(newKeywords.begin(), newKeywords.end());

    res.scoreArray.clear();
    res.keywordCombinations.clear();

    if (!newKeywords.empty()) {
        // Collect scores of individual keywords
        Waap::Scores::calcIndividualKeywords(m_transaction->getAssetState()->scoreBuilder, poolName, newKeywords,
            res.scoreArray);
        // Collect keyword combinations and their scores. Append scores to scoresArray,
        // and also populate m_scanResultKeywordCombinations list
        Waap::Scores::calcCombinations(m_transaction->getAssetState()->scoreBuilder, poolName, newKeywords,
            res.scoreArray, res.keywordCombinations);
    }

    return Waap::Scores::calcArrayScore(res.scoreArray);
}

bool Waap::Scanner::suspiciousHit(Waf2ScanResult& res, const std::string& location, const std::string& param_name) {
    dbgTrace(D_WAAP_SCANNER) << "suspiciousHit processing for parameter: " << param_name << " at " << location <<
        " num of keywords " << res.keyword_matches.size();

    res.location = location;
    res.param_name = param_name; // remember the param name (analyzer needs it for reporting)

    // Select scores pool by location
    std::string poolName = Waap::Scores::getScorePoolNameByLocation(location);

    double score = getScoreData(res, poolName);

    dbgTrace(D_WAAP_SCANNER) << "score: " << score;
    // Add record about scores to the notes[] log (also reported in logs)
    if (score > 1.0f) {
        DetectionEvent(location, res.keyword_matches).notify();
        char buf[128];
        sprintf(buf, "%.3f", score);
        const std::string& res_location = m_transaction->getDeepParser().m_key.first();
        const std::string& res_param_name = m_transaction->getDeepParser().m_key.str();
        m_transaction->addNote(
            "sc:" + res_location + (res_param_name.empty() ? "" : "/" + res_param_name) + ":" + std::string(buf)
        );
    }

    if (m_transaction->shouldIgnoreOverride(res)) {
        dbgTrace(D_WAAP_SCANNER) << "Ignoring parameter key/value " << res.param_name <<
            " due to ignore action in override";
        m_bIgnoreOverride = true;
        return false;
    }

    res.score = score;
    return m_transaction->reportScanResult(res);
}

int Waap::Scanner::onKv(const char* k, size_t k_len, const char* v, size_t v_len, int flags) {
    Waf2ScanResult& res = m_lastScanResult;
    DeepParser &dp = m_transaction->getDeepParser();
    std::string key = std::string(k, k_len);
    std::string value = std::string(v, v_len);
    res.clear();
    dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: k='" << key <<
        "' v='" << value << "'";
    bool isCookiePayload = dp.m_key.first().size() == 6 && dp.m_key.first() == "cookie";
    bool isUrlParamPayload = dp.m_key.first().size() == 9 && dp.m_key.first() == "url_param";
    bool isSplitUrl = dp.m_key.first().size() == 3 &&
        dp.m_key.first() == "url" &&
        dp.m_key.str() != "";
    bool isHeaderPayload = dp.m_key.first().size() == 6 && dp.m_key.first() == "header";
    bool isRefererParamPayload =
        (dp.m_key.first().size() == 13 && dp.m_key.first() == "referer_param");
    bool isBodyPayload = dp.m_key.first().size() == 4 && dp.m_key.first() == "body";
    dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: depth=" <<
        dp.depth() << "; first='" << dp.m_key.first().c_str() << "'; key='" <<
        dp.m_key.str().c_str() << "'";

    // Collect URLs from values for openRedirect feature.
    m_transaction->getOpenRedirectState().collect(v, v_len, m_transaction->getHost());

    // Do not scan our own anti-bot cookie (match by name), it often false alarms.
    const std::string& fullKeyStr = dp.m_key.str();
    dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: fullKeyStr: '" << fullKeyStr << "'";
    //Get Anti bot cookie
    if(isCookiePayload && fullKeyStr == "__fn1522082288") {
        m_antibotCookie = value;
        dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: found Antibot Cookie: '" << m_antibotCookie << "'";
    }

    // Do not scan our own anti-bot cookie (match by name), it often false alarms.
    if (isCookiePayload &&
        (fullKeyStr.find("fnserr") != std::string::npos ||
            fullKeyStr.find("__fn1522082288") != std::string::npos ||
            fullKeyStr.find("_fn_nsess") != std::string::npos)) {
        dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: skip scanning our own anti-bot cookie, by name";
        return 0;
    }
    // scan for csrf token.
    if (isCookiePayload && fullKeyStr == "x-chkp-csrf-token") {
        m_transaction->getCsrfState().set_CsrfToken(v, v_len);
    }
    if (isHeaderPayload && fullKeyStr == "x-chkp-csrf-token") {
        m_transaction->getCsrfState().set_CsrfHeaderToken(v, v_len);
    }
    if (isBodyPayload && fullKeyStr == "x-chkp-csrf-token") {
        m_transaction->getCsrfState().set_CsrfFormToken(v, v_len);
    }

    if (dp.depth() == 0 &&
        isCookiePayload &&
        (v_len >= 2) &&
        ((v[0] == '"' && v[v_len - 1] == '"') || (v[0] == '\'' && v[v_len - 1] == '\''))
        ) {
        dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: removing quotes around cookie value: '" <<
            value << "'";
        // remove the quotes around the value
        v++;
        v_len -= 2;
        value = std::string(v, v_len);
    }
    res.location = dp.m_key.first();
    res.param_name = dp.m_key.str();
    res.unescaped_line = unescape(value);

    m_transaction->getAssetState()->logParamHit(res, m_transaction);

    std::set<std::string> paramTypes = m_transaction->getAssetState()->m_filtersMngr->getParameterTypes(
        IndicatorsFiltersManager::generateKey(res.location, res.param_name, m_transaction));

    if (paramTypes.size() == 1 && paramTypes.find("local_file_path") != paramTypes.end())
    {
        dbgTrace(D_WAAP_SCANNER) << "found parameter as local path, val : " << value;
        if ((value.find("http://") == 0 || value.find("https://") == 0) && !m_transaction->shouldIgnoreOverride(res))
        {
            res.score = 10.0;
            res.unescaped_line = value;
            res.keyword_matches.push_back("url_instead_of_file");
            m_transaction->addNote("sv: found url in " + res.location + "#" + res.param_name);
            m_transaction->reportScanResult(res);
            return 0;
        }
    }
    // Special value only matched when XML <!ENTITY> atribute is found.
    if (v_len == 36) {
        if (value == "08a80340-06d3-11ea-9f87-0242ac11000f" && !m_transaction->shouldIgnoreOverride(res)) {
            // Always return max score when <!ENTITY tag is encoutered during XML parsing.
            res.score = 10.0;
            res.unescaped_line = "<!ENTITY";
            res.keyword_matches.push_back("xml_entity");
            m_transaction->addNote("sv: found xml_entity in " + res.location + "#" + res.param_name);
            m_transaction->reportScanResult(res);
            return 0;
        }
    }

    // Scan parameter name
    bool badUrlEncoding =
        dp.m_key.depth() == 2 &&
        isUrlParamPayload && key != unescape(key) &&
        (!checkUrlEncoded(k, k_len) || !checkUrlEncoded(v, v_len));
    bool scanNameDueToSplitUrl = dp.m_key.depth() == 2 && isSplitUrl && key != "url.id";
    bool suspiciousName = dp.depth() == 0 &&
        (isCookiePayload || isRefererParamPayload || isUrlParamPayload || isBodyPayload) &&
        (!m_transaction->getAssetState()->getSignatures()-> good_header_name_re.hasMatch(key));

    dbgTrace(D_WAAP_SCANNER)
        << "badUrlEncoding="
        << badUrlEncoding
        << ", scanNameDueToSplitUrl="
        << scanNameDueToSplitUrl
        << ", suspiciousName"
        << suspiciousName;

    if (badUrlEncoding || scanNameDueToSplitUrl || suspiciousName) {
        dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: candidate to scan parameter names";

        // Deep-scan parameter names
        if (m_transaction->getAssetState()->apply(key, res, dp.m_key.first())) {
            if (suspiciousHit(res, dp.m_key.first(), dp.m_key.str())) {
                // Scanner found enough evidence to report this res
                dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: SUSPICIOUS PARAM NAME: k='" <<
                    key << "' v='" << value << "'";
#ifdef ENABLE_WAAP_ATTACK_IN_PARAM
                res.param_name = ATTACK_IN_PARAM;
                if (m_transaction->getScanResultPtr()) {
                    m_transaction->getScanResultPtr()->m_isAttackInParam = true;
                    m_transaction->getScanResultPtr()->param_name = ATTACK_IN_PARAM;
                }
                else {
                    dbgWarning(D_WAAP_SCANNER) << "Uninitialized m_scanResult during scanning parameter name (!!!)";
                }
#endif
                m_transaction->addNote("sn:" + res.location + (res.param_name.empty() ? "" : "/" + res.param_name));
            }
        }
    }
    Waf2ScanResult param_name_res = res;
    res.clear();

    // Scan parameter value
    if (m_transaction->getAssetState()->apply(value, res, dp.m_key.first(), dp.isBinaryData(),
        dp.getSplitType()))
    {
        if (!param_name_res.keyword_matches.empty() && !res.keyword_matches.empty() &&
            param_name_res.location == "url_param")
        {
            dbgTrace(D_WAAP_SCANNER) << "Found suspicios content in param name and value. Merging scans";
            res.mergeFrom(param_name_res);
        }

        if (suspiciousHit(res, dp.m_key.first(), dp.m_key.str())) {
            // Scanner found enough evidence to report this res
            dbgTrace(D_WAAP_SCANNER) << "Waap::Scanner::onKv: SUSPICIOUS VALUE: k='" << key <<
                "' v='" << value << "'";
            m_transaction->addNote("sv:" + res.location + (res.param_name.empty() ? "" : "/" + res.param_name));
        }
    }

    return 0;
}