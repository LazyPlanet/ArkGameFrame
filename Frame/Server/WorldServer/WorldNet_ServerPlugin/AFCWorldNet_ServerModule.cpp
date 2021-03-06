/*****************************************************************************
// * This source file is part of ArkGameFrame                                *
// * For the latest info, see https://github.com/ArkGame                     *
// *                                                                         *
// * Copyright(c) 2013 - 2017 ArkGame authors.                               *
// *                                                                         *
// * Licensed under the Apache License, Version 2.0 (the "License");         *
// * you may not use this file except in compliance with the License.        *
// * You may obtain a copy of the License at                                 *
// *                                                                         *
// *     http://www.apache.org/licenses/LICENSE-2.0                          *
// *                                                                         *
// * Unless required by applicable law or agreed to in writing, software     *
// * distributed under the License is distributed on an "AS IS" BASIS,       *
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
// * See the License for the specific language governing permissions and     *
// * limitations under the License.                                          *
// *                                                                         *
// *                                                                         *
// * @file      AFCWorldNet_ServerModule.cpp                                              *
// * @author    Ark Game Tech                                                *
// * @date      2015-12-15                                                   *
// * @brief     AFCWorldNet_ServerModule                                                  *
*****************************************************************************/
#include "AFCWorldNet_ServerModule.h"
#include "AFWorldNetServerPlugin.h"
#include "SDK/Proto/AFMsgDefine.h"

bool AFCWorldNet_ServerModule::Init()
{
    m_pNetModule = ARK_NEW AFINetServerModule(pPluginManager);
    return true;
}

bool AFCWorldNet_ServerModule::AfterInit()
{
    m_pKernelModule = pPluginManager->FindModule<AFIKernelModule>();
    m_pWorldLogicModule = pPluginManager->FindModule<AFIWorldLogicModule>();
    m_pLogModule = pPluginManager->FindModule<AFILogModule>();
    m_pElementModule = pPluginManager->FindModule<AFIElementModule>();
    m_pClassModule = pPluginManager->FindModule<AFIClassModule>();

    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_PTWG_PROXY_REFRESH, this, &AFCWorldNet_ServerModule::OnRefreshProxyServerInfoProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_PTWG_PROXY_REGISTERED, this, &AFCWorldNet_ServerModule::OnProxyServerRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_PTWG_PROXY_UNREGISTERED, this, &AFCWorldNet_ServerModule::OnProxyServerUnRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_GTW_GAME_REGISTERED, this, &AFCWorldNet_ServerModule::OnGameServerRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_GTW_GAME_UNREGISTERED, this, &AFCWorldNet_ServerModule::OnGameServerUnRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_GTW_GAME_REFRESH, this, &AFCWorldNet_ServerModule::OnRefreshGameServerInfoProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_ACK_ONLINE_NOTIFY, this, &AFCWorldNet_ServerModule::OnOnlineProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_ACK_OFFLINE_NOTIFY, this, &AFCWorldNet_ServerModule::OnOfflineProcess);

    m_pNetModule->AddEventCallBack(this, &AFCWorldNet_ServerModule::OnSocketEvent);

    ARK_SHARE_PTR<AFIClass> xLogicClass = m_pClassModule->GetElement("Server");
    if(nullptr != xLogicClass)
    {
        NFList<std::string>& xNameList = xLogicClass->GetConfigNameList();
        std::string strConfigName;
        for(bool bRet = xNameList.First(strConfigName); bRet; bRet = xNameList.Next(strConfigName))
        {
            const int nServerType = m_pElementModule->GetPropertyInt(strConfigName, "Type");
            const int nServerID = m_pElementModule->GetPropertyInt(strConfigName, "ServerID");
            if(nServerType == NF_SERVER_TYPES::NF_ST_WORLD && pPluginManager->AppID() == nServerID)
            {
                const int nPort = m_pElementModule->GetPropertyInt(strConfigName, "Port");
                const int nMaxConnect = m_pElementModule->GetPropertyInt(strConfigName, "MaxOnline");
                const int nCpus = m_pElementModule->GetPropertyInt(strConfigName, "CpuCount");
                const std::string strName(m_pElementModule->GetPropertyString(strConfigName, "Name"));
                const std::string strIP(m_pElementModule->GetPropertyString(strConfigName, "IP"));

                int nRet = m_pNetModule->Initialization(nMaxConnect, strIP, nPort, nCpus, nServerID);
                if(nRet < 0)
                {
                    std::ostringstream strLog;
                    strLog << "Cannot init server net, Port = " << nPort;
                    m_pLogModule->LogError(NULL_GUID, strLog, __FUNCTION__, __LINE__);
                    ARK_ASSERT(nRet, "Cannot init server net", __FILE__, __FUNCTION__);
                    exit(0);
                }
            }
        }
    }

    return true;
}

bool AFCWorldNet_ServerModule::Shut()
{

    return true;
}

bool AFCWorldNet_ServerModule::Execute()
{
    LogGameServer();

    return m_pNetModule->Execute();
}

void AFCWorldNet_ServerModule::OnGameServerRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ServerInfoReportList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    for(int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const AFMsg::ServerInfoReport& xData = xMsg.server_list(i);
        ARK_SHARE_PTR<ServerData> pServerData =  mGameMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mGameMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "GameServerRegistered");
    }

    SynGameToProxy();
}

void AFCWorldNet_ServerModule::OnGameServerUnRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ServerInfoReportList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    for(int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const AFMsg::ServerInfoReport& xData = xMsg.server_list(i);
        mGameMap.RemoveElement(xData.server_id());

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "GameServerRegistered");
    }
}

void AFCWorldNet_ServerModule::OnRefreshGameServerInfoProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ServerInfoReportList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    for(int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const AFMsg::ServerInfoReport& xData = xMsg.server_list(i);

        ARK_SHARE_PTR<ServerData> pServerData =  mGameMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mGameMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "GameServerRegistered");
    }

    SynGameToProxy();
}

void AFCWorldNet_ServerModule::OnProxyServerRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ServerInfoReportList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    for(int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const AFMsg::ServerInfoReport& xData = xMsg.server_list(i);

        ARK_SHARE_PTR<ServerData> pServerData =  mProxyMap.GetElement(xData.server_id());
        if(!pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mProxyMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "Proxy Registered");

        SynGameToProxy(xClientID);
    }
}

void AFCWorldNet_ServerModule::OnProxyServerUnRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ServerInfoReportList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    for(int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const AFMsg::ServerInfoReport& xData = xMsg.server_list(i);

        mGameMap.RemoveElement(xData.server_id());

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "Proxy UnRegistered");
    }
}

void AFCWorldNet_ServerModule::OnRefreshProxyServerInfoProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ServerInfoReportList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    for(int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const AFMsg::ServerInfoReport& xData = xMsg.server_list(i);

        ARK_SHARE_PTR<ServerData> pServerData =  mProxyMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mGameMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "Proxy Registered");

        SynGameToProxy(xClientID);
    }
}

int AFCWorldNet_ServerModule::OnLeaveGameProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{

    return 0;
}

void AFCWorldNet_ServerModule::OnSocketEvent(const NetEventType eEvent, const AFGUID& xClientID, const int nServerID)
{
    if(eEvent == DISCONNECTED)
    {
        m_pLogModule->LogInfo(xClientID, "NF_NET_EVENT_EOF", "Connection closed", __FUNCTION__, __LINE__);
        OnClientDisconnect(xClientID);
    }
    else  if(eEvent == CONNECTED)
    {
        m_pLogModule->LogInfo(xClientID, "NF_NET_EVENT_CONNECTED", "connectioned success", __FUNCTION__, __LINE__);
        OnClientConnected(xClientID);
    }
}

void AFCWorldNet_ServerModule::SynGameToProxy()
{
    AFMsg::ServerInfoReportList xData;

    ARK_SHARE_PTR<ServerData> pServerData =  mProxyMap.First();
    while(nullptr != pServerData)
    {
        SynGameToProxy(pServerData->xClient);

        pServerData = mProxyMap.Next();
    }
}

void AFCWorldNet_ServerModule::SynGameToProxy(const AFGUID& xClientID)
{
    AFMsg::ServerInfoReportList xData;

    ARK_SHARE_PTR<ServerData> pServerData =  mGameMap.First();
    while(nullptr != pServerData)
    {
        AFMsg::ServerInfoReport* pData = xData.add_server_list();
        *pData = *(pServerData->pData);

        pServerData = mGameMap.Next();
    }

    m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_STS_NET_INFO, xData, xClientID, AFGUID(0));
}

void AFCWorldNet_ServerModule::OnClientDisconnect(const AFGUID& xClientID)
{
    //不管是game还是proxy都要找出来,替他反注册
    ARK_SHARE_PTR<ServerData> pServerData =  mGameMap.First();
    while(nullptr != pServerData)
    {
        if(xClientID == pServerData->xClient)
        {
            pServerData->pData->set_server_state(AFMsg::EST_CRASH);
            pServerData->xClient = 0;

            SynGameToProxy();
            return;
        }

        pServerData = mGameMap.Next();
    }

    //////////////////////////////////////////////////////////////////////////

    int nServerID = 0;
    pServerData =  mProxyMap.First();
    while(pServerData)
    {
        if(xClientID == pServerData->xClient)
        {
            nServerID = pServerData->pData->server_id();
            break;
        }

        pServerData = mProxyMap.Next();
    }

    mProxyMap.RemoveElement(nServerID);
}

void AFCWorldNet_ServerModule::OnClientConnected(const AFGUID& xClientID)
{


}

void AFCWorldNet_ServerModule::LogGameServer()
{
    if(mnLastCheckTime + 10 > GetPluginManager()->GetNowTime())
    {
        return;
    }

    mnLastCheckTime = GetPluginManager()->GetNowTime();

    m_pLogModule->LogInfo(AFGUID(), "Begin Log GameServer Info", "");

    ARK_SHARE_PTR<ServerData> pGameData = mGameMap.First();
    while(pGameData)
    {
        std::ostringstream stream;
        stream << "Type: " << pGameData->pData->server_type() << " ID: " << pGameData->pData->server_id() << " State: " <<  AFMsg::EServerState_Name(pGameData->pData->server_state()) << " IP: " << pGameData->pData->server_ip() << " xClient: " << pGameData->xClient.n64Value;

        m_pLogModule->LogInfo(AFGUID(), stream);

        pGameData = mGameMap.Next();
    }

    m_pLogModule->LogInfo(AFGUID(), "End Log GameServer Info", "");

    m_pLogModule->LogInfo(AFGUID(), "Begin Log ProxyServer Info", "");

    pGameData = mProxyMap.First();
    while(pGameData)
    {
        std::ostringstream stream;
        stream << "Type: " << pGameData->pData->server_type() << " ID: " << pGameData->pData->server_id() << " State: " <<  AFMsg::EServerState_Name(pGameData->pData->server_state()) << " IP: " << pGameData->pData->server_ip() << " xClient: " << pGameData->xClient.n64Value;

        m_pLogModule->LogInfo(AFGUID(), stream);

        pGameData = mProxyMap.Next();
    }

    m_pLogModule->LogInfo(AFGUID(), "End Log ProxyServer Info", "");
}


void AFCWorldNet_ServerModule::OnOnlineProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    CLIENT_MSG_PROCESS_NO_OBJECT(nMsgID, msg, nLen, AFMsg::RoleOnlineNotify);

}

void AFCWorldNet_ServerModule::OnOfflineProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    CLIENT_MSG_PROCESS_NO_OBJECT(nMsgID, msg, nLen, AFMsg::RoleOfflineNotify);
}

bool AFCWorldNet_ServerModule::SendMsgToGame(const int nGameID, const AFMsg::EGameMsgID eMsgID, google::protobuf::Message& xData, const AFGUID nPlayer)
{
    ARK_SHARE_PTR<ServerData> pData = mGameMap.GetElement(nGameID);
    if(nullptr != pData)
    {
        m_pNetModule->SendMsgPB(eMsgID, xData, pData->xClient, nPlayer);
    }

    return true;
}

bool AFCWorldNet_ServerModule::SendMsgToGame(const AFIDataList& argObjectVar, const AFIDataList& argGameID, const AFMsg::EGameMsgID eMsgID, google::protobuf::Message& xData)
{
    if(argGameID.GetCount() != argObjectVar.GetCount())
    {
        return false;
    }

    for(int i = 0; i < argObjectVar.GetCount(); i++)
    {
        const AFGUID& identOther = argObjectVar.Object(i);
        const int64_t  nGameID = argGameID.Int(i);

        SendMsgToGame(nGameID, eMsgID, xData, identOther);
    }

    return true;
}

bool AFCWorldNet_ServerModule::SendMsgToPlayer(const AFMsg::EGameMsgID eMsgID, google::protobuf::Message& xData, const AFGUID nPlayer)
{
    int nGameID = GetPlayerGameID(nPlayer);
    if(nGameID < 0)
    {
        return false;
    }

    return SendMsgToGame(nGameID, eMsgID, xData, nPlayer);
}

int AFCWorldNet_ServerModule::OnObjectListEnter(const AFIDataList& self, const AFIDataList& argVar)
{
    if(self.GetCount() <= 0 || argVar.GetCount() <= 0)
    {
        return 0;
    }

    AFMsg::AckPlayerEntryList xPlayerEntryInfoList;
    for(int i = 0; i < argVar.GetCount(); i++)
    {
        AFGUID identOld = argVar.Object(i);
        //排除空对象
        if(identOld.IsNull())
        {
            continue;
        }

        AFMsg::PlayerEntryInfo* pEntryInfo = xPlayerEntryInfoList.add_object_list();
        *(pEntryInfo->mutable_object_guid()) = AFINetServerModule::NFToPB(identOld);
        //*pEntryInfo->mutable_pos() = AFINetServerModule::NFToPB(m_pKernelModule->GetObject(identOld, "Pos")); todo
        pEntryInfo->set_career_type(m_pKernelModule->GetPropertyInt(identOld, "Job"));
        pEntryInfo->set_player_state(m_pKernelModule->GetPropertyInt(identOld, "State"));
        pEntryInfo->set_config_id(m_pKernelModule->GetPropertyString(identOld, "ConfigID"));
        pEntryInfo->set_scene_id(m_pKernelModule->GetPropertyInt(identOld, "SceneID"));
        pEntryInfo->set_class_id(m_pKernelModule->GetPropertyString(identOld, "ClassName"));

    }

    if(xPlayerEntryInfoList.object_list_size() <= 0)
    {
        return 0;
    }

    for(int i = 0; i < self.GetCount(); i++)
    {
        AFGUID ident = self.Object(i);
        if(ident.IsNull())
        {
            continue;
        }

        //可能在不同的网关呢,得到后者所在的网关FD
        SendMsgToPlayer(AFMsg::EGMI_ACK_OBJECT_ENTRY, xPlayerEntryInfoList, ident);
    }

    return 1;
}


int AFCWorldNet_ServerModule::OnObjectListLeave(const AFIDataList& self, const AFIDataList& argVar)
{
    if(self.GetCount() <= 0 || argVar.GetCount() <= 0)
    {
        return 0;
    }

    AFMsg::AckPlayerLeaveList xPlayerLeaveInfoList;
    for(int i = 0; i < argVar.GetCount(); i++)
    {
        AFGUID identOld = argVar.Object(i);
        //排除空对象
        if(identOld.IsNull())
        {
            continue;
        }

        AFMsg::Ident* pIdent = xPlayerLeaveInfoList.add_object_list();
        *pIdent = AFINetServerModule::NFToPB(argVar.Object(i));
    }

    for(int i = 0; i < self.GetCount(); i++)
    {
        AFGUID ident = self.Object(i);
        if(ident.IsNull())
        {
            continue;
        }
        //可能在不同的网关呢,得到后者所在的网关FD
        SendMsgToPlayer(AFMsg::EGMI_ACK_OBJECT_LEAVE, xPlayerLeaveInfoList, ident);
    }

    return 1;
}


int AFCWorldNet_ServerModule::OnRecordEnter(const AFIDataList& argVar, const AFIDataList& argGameID, const AFGUID& self)
{
    if(argVar.GetCount() <= 0 || self.IsNull())
    {
        return 0;
    }

    if(argVar.GetCount() != argGameID.GetCount())
    {
        return 1;
    }

    AFMsg::MultiObjectRecordList xPublicMsg;
    AFMsg::MultiObjectRecordList xPrivateMsg;

    ARK_SHARE_PTR<AFIObject> pObject = m_pKernelModule->GetObject(self);
    if(nullptr == pObject)
    {
        return 1;
    }

    AFMsg::ObjectRecordList* pPublicData = NULL;
    AFMsg::ObjectRecordList* pPrivateData = NULL;

    ARK_SHARE_PTR<AFIRecordMgr> pRecordManager = pObject->GetRecordManager();
    int nRecordCount = pRecordManager->GetCount();
    for(int i = 0; i < nRecordCount; ++i)
    {
        AFRecord* pRecord = pRecordManager->GetRecordByIndex(i);
        if(NULL == pRecord)
        {
            continue;
        }

        if(!pRecord->IsPublic() && !pRecord->IsPrivate())
        {
            continue;
        }

        AFMsg::ObjectRecordBase* pPrivateRecordBase = NULL;
        AFMsg::ObjectRecordBase* pPublicRecordBase = NULL;
        if(pRecord->IsPublic())
        {
            if(!pPublicData)
            {
                pPublicData = xPublicMsg.add_multi_player_record();
                *(pPublicData->mutable_player_id()) = AFINetServerModule::NFToPB(self);
            }
            pPublicRecordBase = pPublicData->add_record_list();
            pPublicRecordBase->set_record_name(pRecord->GetName());

            OnRecordEnterPack(pRecord, pPublicRecordBase);
        }

        if(pRecord->IsPrivate())
        {
            if(!pPrivateData)
            {
                pPrivateData = xPrivateMsg.add_multi_player_record();
                *(pPrivateData->mutable_player_id()) = AFINetServerModule::NFToPB(self);
            }
            pPrivateRecordBase = pPrivateData->add_record_list();
            pPrivateRecordBase->set_record_name(pRecord->GetName());

            OnRecordEnterPack(pRecord, pPrivateRecordBase);
        }
    }

    for(int i = 0; i < argVar.GetCount(); i++)
    {
        const AFGUID& identOther = argVar.Object(i);
        const int64_t nGameID = argGameID.Int(i);
        if(self == identOther)
        {
            if(xPrivateMsg.multi_player_record_size() > 0)
            {
                SendMsgToGame(nGameID, AFMsg::EGMI_ACK_OBJECT_RECORD_ENTRY, xPrivateMsg, identOther);
            }
        }
        else
        {
            if(xPublicMsg.multi_player_record_size() > 0)
            {
                SendMsgToGame(nGameID, AFMsg::EGMI_ACK_OBJECT_RECORD_ENTRY, xPublicMsg, identOther);
            }
        }
    }

    return 0;
}

bool AFCWorldNet_ServerModule::OnRecordEnterPack(AFRecord* pRecord, AFMsg::ObjectRecordBase* pObjectRecordBase)
{
    if(!pRecord || !pObjectRecordBase)
    {
        return false;
    }

    for(int i = 0; i < pRecord->GetRowCount(); i ++)
    {
        //不管public还是private都要加上，不然public广播了那不是private就广播不了了
        AFMsg::RecordAddRowStruct* pAddRowStruct = pObjectRecordBase->add_row_struct();
        pAddRowStruct->set_row(i);

        AFCDataList valueList;
        pRecord->QueryRow(i, valueList);

        for(int j = 0; j < valueList.GetCount(); j++)
        {
            AFMsg::RecordPBData* pAddData = pAddRowStruct->add_record_data_list();
            AFINetServerModule::RecordToPBRecord(valueList, i, j, *pAddData);
        }
    }

    return true;
}

int AFCWorldNet_ServerModule::OnPropertyEnter(const AFIDataList& argVar, const AFIDataList& argGameID, const AFGUID& self)
{
    if(argVar.GetCount() <= 0 || self.IsNull())
    {
        return 0;
    }

    if(argVar.GetCount() != argGameID.GetCount())
    {
        return 1;
    }



    //分为自己和外人
    //1.public发送给所有人
    //2.如果自己在列表中，再次发送private数据
    ARK_SHARE_PTR<AFIObject> pObject = m_pKernelModule->GetObject(self);
    if(nullptr == pObject)
    {
        return 0;
    }

    AFMsg::MultiObjectPropertyList xPublicMsg;
    AFMsg::MultiObjectPropertyList xPrivateMsg;
    AFMsg::ObjectPropertyList* pPublicData = xPublicMsg.add_multi_player_property();
    AFMsg::ObjectPropertyList* pPrivateData = xPrivateMsg.add_multi_player_property();

    *(pPublicData->mutable_player_id()) = AFINetServerModule::NFToPB(self);
    *(pPrivateData->mutable_player_id()) = AFINetServerModule::NFToPB(self);

    ARK_SHARE_PTR<AFIPropertyMgr> pPropertyManager = pObject->GetPropertyManager();

    for(int i = 0; i < pPropertyManager->GetPropertyCount(); i++)
    {
        AFProperty* pPropertyInfo = pPropertyManager->GetPropertyByIndex(i);
        if(nullptr == pPropertyInfo)
        {
            continue;
        }
        if(pPropertyInfo->Changed())
        {
            if(pPropertyInfo->IsPublic())
            {
                AFMsg::PropertyPBData* pDataInt = pPublicData->add_property_data_list();
                AFINetServerModule::DataToPBProperty(pPropertyInfo->GetValue(), pPropertyInfo->GetName().c_str(), *pDataInt);
            }

            if(pPropertyInfo->IsPrivate())
            {
                AFMsg::PropertyPBData* pDataInt = pPrivateData->add_property_data_list();
                AFINetServerModule::DataToPBProperty(pPropertyInfo->GetValue(), pPropertyInfo->GetName().c_str(), *pDataInt);
            }
        }

    }

    for(int i = 0; i < argVar.GetCount(); i++)
    {
        const AFGUID& identOther = argVar.Object(i);
        const int64_t nGameID = argGameID.Int(i);
        if(self == identOther)
        {
            //找到他所在网关的FD
            SendMsgToGame(nGameID, AFMsg::EGMI_ACK_OBJECT_PROPERTY_ENTRY, xPrivateMsg, identOther);
        }
        else
        {
            SendMsgToGame(nGameID, AFMsg::EGMI_ACK_OBJECT_PROPERTY_ENTRY, xPublicMsg, identOther);
        }

    }

    return 0;
}

ARK_SHARE_PTR<ServerData> AFCWorldNet_ServerModule::GetSuitProxyForEnter()
{
    return mProxyMap.First();
}

AFINetServerModule* AFCWorldNet_ServerModule::GetNetModule()
{
    return m_pNetModule;
}

int AFCWorldNet_ServerModule::GetPlayerGameID(const AFGUID self)
{
    //to do
    return -1;
}