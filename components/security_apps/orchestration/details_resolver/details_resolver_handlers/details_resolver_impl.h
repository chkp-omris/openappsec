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

#ifndef __DETAILS_RESOLVER_HANDLER_CC__
#error details_resolver_handlers/details_resolver_impl.h should not be included directly.
#endif // __DETAILS_RESOLVER_HANDLER_CC__

// Retrieve artifacts by incorporating nano service names into additional metadata:
// To include a required nano service in the additional metadata sent to the manifest generator,
// add a handler in this file. The key to use is 'requiredNanoServices', and its value should be
// a string representing an array of nano service prefix names, separated by semicolons.
// For example: "httpTransactionHandler_linux;iotSnmp_gaia;"
//
// Handler example for reading the content of a configuration file:
// FILE_CONTENT_HANDLER("requiredNanoServices", "/tmp/nano_services_list", getRequiredNanoServices)

// use SHELL_CMD_HANDLER(key as string, shell command as string, ptr to Maybe<string> handler(const string&))
// to return a string value for an attribute key based on a logic executed in a handler that receives
// shell command execution output as its input

#ifdef SHELL_PRE_CMD
#if defined(gaia) || defined(smb)
SHELL_PRE_CMD("read sdwan data",
    "(cpsdwan get_data > /tmp/cpsdwan_getdata_orch.json~) "
    "&& (mv /tmp/cpsdwan_getdata_orch.json~ /tmp/cpsdwan_getdata_orch.json)")
#endif //gaia || smb
#if defined(smb)
SHELL_PRE_CMD("gunzip local.cfg", "gunzip -c $FWDIR/state/local/FW1/local.cfg.gz > /tmp/local.cfg")
#endif  //smb
#endif

#ifdef SHELL_CMD_HANDLER
#if defined(gaia) || defined(smb)
SHELL_CMD_HANDLER("cpProductIntegrationMgmtObjectType", "cpprod_util CPPROD_IsMgmtMachine", getMgmtObjType)
SHELL_CMD_HANDLER("prerequisitesForHorizonTelemetry",
    "FS_PATH=<FILESYSTEM-PREFIX>; [ -f ${FS_PATH}/cp-nano-horizon-telemetry-prerequisites.log ] "
    "&& head -1 ${FS_PATH}/cp-nano-horizon-telemetry-prerequisites.log || echo ''",
    checkIsInstallHorizonTelemetrySucceeded)
SHELL_CMD_HANDLER("QUID", "[ -d /opt/CPquid ] "
    "&& python3 /opt/CPquid/Quid_Api.py -i /opt/CPotelcol/quid_api/get_global_id.json | jq -r .message || echo ''",
    getQUID)
SHELL_CMD_HANDLER("hasSDWan", "[ -f $FWDIR/bin/sdwan_steering ] && echo '1' || echo '0'", checkHasSDWan)
SHELL_CMD_HANDLER(
    "canUpdateSDWanData",
    "jq -r .can_update_sdwan_data /tmp/cpsdwan_getdata_orch.json",
    checkCanUpdateSDWanData
)
SHELL_CMD_HANDLER(
    "isSdwanRunning",
    "[ -v $(pidof cp-nano-sdwan) ] && echo 'false' || echo 'true'",
    checkIfSdwanRunning)
SHELL_CMD_HANDLER(
    "lsmProfileName",
    "jq -r .lsm_profile_name /tmp/cpsdwan_getdata_orch.json",
    checkLsmProfileName
)
SHELL_CMD_HANDLER(
    "lsmProfileUuid",
    "jq -r .lsm_profile_uuid /tmp/cpsdwan_getdata_orch.json",
    checkLsmProfileUuid
)
SHELL_CMD_HANDLER(
    "IP Address",
    "[ $(cpprod_util FWisDAG) -eq 1 ] && echo \"Dynamic Address\" "
    "|| (jq -r .main_ip /tmp/cpsdwan_getdata_orch.json)",
    getGWIPAddress
)
SHELL_CMD_HANDLER(
    "Version",
    "cat /etc/cp-release | grep -oE 'R[0-9]+(\\.[0-9]+)?'",
    getGWVersion
)
SHELL_CMD_HANDLER(
    "cpProductIntegrationMgmtParentObjectIP",
    "obj=\"$(jq -r .cluster_name /tmp/cpsdwan_getdata_orch.json)\";"
    " awk -v obj=\"$obj\" '$1 == \":\" && $2 == \"(\" obj, $1 == \":ip_address\" { if ($1 == \":ip_address\")"
    " { gsub(/[()]/, \"\", $2); print $2; exit; } }'"
    " $FWDIR/state/local/FW1/local.gateway_cluster",
    getClusterObjectIP
)
SHELL_CMD_HANDLER(
    "isFecApplicable",
    "fw ctl get int support_fec |& grep -sq \"support_fec =\";echo $?",
    getFecApplicable
)
#endif //gaia || smb

#if defined(gaia)
SHELL_CMD_HANDLER("hasSAMLSupportedBlade", "enabled_blades", checkSAMLSupportedBlade)
SHELL_CMD_HANDLER("hasIDABlade", "enabled_blades", checkIDABlade)
SHELL_CMD_HANDLER("hasSAMLPortal", "mpclient status nac", checkSAMLPortal)
SHELL_CMD_HANDLER(
    "hasAgentIntelligenceInstalled",
    "<FILESYSTEM-PREFIX>/watchdog/cp-nano-watchdog "
    "--status --service <FILESYSTEM-PREFIX>/agentIntelligence/cp-nano-agent-intelligence-service",
    checkAgentIntelligence
)
SHELL_CMD_HANDLER("hasIdaIdnEnabled", "pep control IDN_nano_Srv_support status", checkPepIdaIdnStatus)
SHELL_CMD_HANDLER("requiredNanoServices", "ida_packages", getIDAGaiaPackages)
SHELL_CMD_HANDLER(
    "cpProductIntegrationMgmtParentObjectName",
    "cat $FWDIR/database/myself_objects.C "
    "| awk -F '[:()]' '/:cluster_object/ {found=1; next} found && /:Name/ {print $3; exit}'",
    getMgmtParentObjName
)
SHELL_CMD_HANDLER(
    "cpProductIntegrationMgmtParentObjectUid",
    "cat $FWDIR/database/myself_objects.C "
    "| awk -F'[{}]' '/:cluster_object/ { found=1; next } found && /:Uid/ { uid=tolower($2); print uid; exit }'",
    getMgmtParentObjUid
)
SHELL_CMD_HANDLER(
    "Hardware",
    "cat $FWDIR/database/myself_objects.C | awk -F '[:()]' '/:appliance_type/ {print $3}' | head -n 1",
    getGWHardware
)
SHELL_CMD_HANDLER(
    "Application Control",
    "cat $FWDIR/database/myself_objects.C | awk -F '[:()]' '/:application_firewall_blade/ {print $3}' | head -n 1",
    getGWApplicationControlBlade
)
SHELL_CMD_HANDLER(
    "URL Filtering",
    "cat $FWDIR/database/myself_objects.C | awk -F '[:()]' '/:advanced_uf_blade/ {print $3}' | head -n 1",
    getGWURLFilteringBlade
)
SHELL_CMD_HANDLER(
    "IPSec VPN",
    "cat $FWDIR/database/myself_objects.C | awk -F '[:()]' '/:VPN_1/ {print $3}' | head -n 1",
    getGWIPSecVPNBlade
)
SHELL_CMD_HANDLER(
    "SMCBasedMgmtId",
    "domain_uuid=$(jq -r .domain_uuid /tmp/cpsdwan_getdata_orch.json);"
    "[ \"$domain_uuid\" != \"null\" ] && echo \"$domain_uuid\" ||"
    "cat $FWDIR/database/myself_objects.C "
    "| awk -F'[{}]' '/:masters/ { found=1; next } found && /:Uid/ { uid=tolower($2); print uid; exit }'",
    getSMCBasedMgmtId
)
SHELL_CMD_HANDLER(
    "SMCBasedMgmtName",
    "domain_name=$(jq -r .domain_name /tmp/cpsdwan_getdata_orch.json);"
    "[ \"$domain_name\" != \"null\" ] && echo \"$domain_name\" ||"
    "cat $FWDIR/database/myself_objects.C "
    "| awk -F '[:()]' '/:masters/ {found=1; next} found && /:Name/ {print $3; exit}'",
    getSMCBasedMgmtName
)
SHELL_CMD_HANDLER(
    "managements",
    "sed -n '/:masters (/,$p' $FWDIR/database/myself_objects.C |"
    " sed -e ':a' -e 'N' -e '$!ba' -e 's/\\n//g' -e 's/\t//g' -e 's/ //g' | sed 's/))):.*/)))):/'",
    extractManagements
)
#endif //gaia

#if defined(smb)
SHELL_CMD_HANDLER(
    "cpProductIntegrationMgmtParentObjectName",
    "jq -r .cluster_name /tmp/cpsdwan_getdata_orch.json",
    getSmbMgmtParentObjName
)
SHELL_CMD_HANDLER(
    "cpProductIntegrationMgmtParentObjectUid",
    "jq -r .cluster_uuid /tmp/cpsdwan_getdata_orch.json",
    getSmbMgmtParentObjUid
)
SHELL_CMD_HANDLER(
    "cpProductIntegrationMgmtObjectName",
    "cpprod_util FwIsLocalMgmt",
    getSmbObjectName
)
SHELL_CMD_HANDLER(
    "Application Control",
    "cat $FWDIR/conf/active_blades.txt | grep -o 'APCL [01]' | cut -d ' ' -f2",
    getSmbGWApplicationControlBlade
)
SHELL_CMD_HANDLER(
    "URL Filtering",
    "cat $FWDIR/conf/active_blades.txt | grep -o 'URLF [01]' | cut -d ' ' -f2",
    getSmbGWURLFilteringBlade
)
SHELL_CMD_HANDLER(
    "IPSec VPN",
    "cat $FWDIR/conf/active_blades.txt | grep -o 'IPS [01]' | cut -d ' ' -f2",
    getSmbGWIPSecVPNBlade
)
SHELL_CMD_HANDLER(
    "SMCBasedMgmtId",
    "domain_uuid=$(jq -r .domain_uuid /tmp/cpsdwan_getdata_orch.json);"
    "[ \"$domain_uuid\" != \"null\" ] && echo \"$domain_uuid\" ||"
    "cat /tmp/local.cfg "
    "| awk -F'[{}]' '/:masters/ { found=1; next } found && /:Uid/ { uid=tolower($2); print uid; exit }'",
    getSMCBasedMgmtId
)

SHELL_CMD_HANDLER(
    "SMCBasedMgmtName",
    "domain_name=$(jq -r .domain_name /tmp/cpsdwan_getdata_orch.json);"
    "[ \"$domain_name\" != \"null\" ] && echo \"$domain_name\" ||"
    "cat /tmp/local.cfg "
    "| awk -F '[:()]' '/:masters/ {found=1; next} found && /:Name/ {print $3; exit}'",
    getSMCBasedMgmtName
)

SHELL_CMD_HANDLER(
    "managements",
    "sed -n '/:masters (/,$p' /tmp/local.cfg |"
    " sed -e ':a' -e 'N' -e '$!ba' -e 's/\\n//g' -e 's/\t//g' -e 's/ //g' | sed 's/))):.*/)))):/'",
    extractManagements
)
#endif//smb

SHELL_CMD_OUTPUT("kernel_version", "uname -r")
SHELL_CMD_OUTPUT("helloWorld", "cat /tmp/agentHelloWorld 2>/dev/null")
#endif // SHELL_CMD_OUTPUT


// use FILE_CONTENT_HANDLER(key as string, path to file as string, ptr to Maybe<string> handler(ifstream&))
// to return a string value for an attribute key based on a logic executed in a handler that receives file as input
#ifdef FILE_CONTENT_HANDLER

#if defined(gaia)

FILE_CONTENT_HANDLER(
    "hasIdpConfigured",
    (getenv("SAMLPORTAL_HOME") ? string(getenv("SAMLPORTAL_HOME")) : "") + "/phpincs/spPortal/idpPolicy.xml",
    checkIDP
)
FILE_CONTENT_HANDLER(
    "cpProductIntegrationMgmtObjectName",
    (getenv("FWDIR") ? string(getenv("FWDIR")) : "") + "/database/myown.C",
    getMgmtObjName
)
#endif //gaia

#if defined(alpine)
FILE_CONTENT_HANDLER("alpine_tag", "/usr/share/build/cp-alpine-tag", getCPAlpineTag)
#endif // alpine
#if defined(gaia) || defined(smb)
FILE_CONTENT_HANDLER("os_release", "/etc/cp-release", getOsRelease)
FILE_CONTENT_HANDLER(
    "cpProductIntegrationMgmtObjectUid",
    (getenv("FWDIR") ? string(getenv("FWDIR")) : "") + "/database/myown.C",
    getMgmtObjUid
)
#else // !(gaia || smb)
FILE_CONTENT_HANDLER("os_release", "/etc/os-release", getOsRelease)
#endif // gaia || smb

FILE_CONTENT_HANDLER("AppSecModelVersion", "<FILESYSTEM-PREFIX>/conf/waap/waap.data", getWaapModelVersion)

#endif // FILE_CONTENT_HANDLER

#ifdef SHELL_POST_CMD
#if defined(smb)
SHELL_POST_CMD("remove local.cfg", "rm -rf /tmp/local.cfg")
#endif  //smb
#endif
