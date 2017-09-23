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
// * @file      AFCMasterNet_ServerModule.cpp                                              *
// * @author    Ark Game Tech                                                *
// * @date      2015-12-15                                                   *
// * @brief     AFCMasterNet_ServerModule                                                  *
*****************************************************************************/
#include "AFCMasterNet_ServerModule.h"
#include "AFMasterNetServerPlugin.h"

bool AFCMasterNet_ServerModule::Init()
{
    m_pNetModule = ARK_NEW AFINetServerModule(pPluginManager);
    return true;
}

bool AFCMasterNet_ServerModule::Shut()
{
    return true;
}

void AFCMasterNet_ServerModule::OnWorldRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        ARK_SHARE_PTR<ServerData> pServerData =  mWorldMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mWorldMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "WorldRegistered");
    }

    SynWorldToLogin();
}

void AFCMasterNet_ServerModule::OnWorldUnRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        mWorldMap.RemoveElement(xData.server_id());


        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "WorldUnRegistered");
    }

    SynWorldToLogin();
}

void AFCMasterNet_ServerModule::OnRefreshWorldInfoProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        ARK_SHARE_PTR<ServerData> pServerData =  mWorldMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mWorldMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "RefreshWorldInfo");

    }

    SynWorldToLogin();
}

void AFCMasterNet_ServerModule::OnLoginRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        ARK_SHARE_PTR<ServerData> pServerData =  mLoginMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mLoginMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "LoginRegistered");
    }

    SynWorldToLogin();
}

void AFCMasterNet_ServerModule::OnLoginUnRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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

        mLoginMap.RemoveElement(xData.server_id());

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "LoginUnRegistered");

    }
}

void AFCMasterNet_ServerModule::OnRefreshLoginInfoProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        ARK_SHARE_PTR<ServerData> pServerData =  mLoginMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<ServerData>(ARK_NEW ServerData());
            mLoginMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xClient = xClientID;
        *(pServerData->pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "RefreshLoginInfo");

    }
}

void AFCMasterNet_ServerModule::OnSelectWorldProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ReqConnectWorld xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ServerData> pServerData =  mWorldMap.GetElement(xMsg.world_id());
    if(nullptr == pServerData)
    {
        return;
    }

    //ת���͵����������
    m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_REQ_CONNECT_WORLD, xMsg, pServerData->xClient, nPlayerID);
}

bool AFCMasterNet_ServerModule::Execute()
{
    LogGameServer();

    m_pNetModule->Execute();
    return true;
}

void AFCMasterNet_ServerModule::OnSelectServerResultProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::AckConnectWorldResult xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ServerData> pServerData =  mLoginMap.GetElement(xMsg.login_id());
    if(nullptr == pServerData)
    {
        return;
    }

    //ת���͵���¼������
    m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_ACK_CONNECT_WORLD, xMsg, pServerData->xClient, nPlayerID);
}

bool AFCMasterNet_ServerModule::AfterInit()
{
    m_pKernelModule = pPluginManager->FindModule<AFIKernelModule>();
    m_pLogModule = pPluginManager->FindModule<AFILogModule>();
    m_pClassModule = pPluginManager->FindModule<AFIClassModule>();
    m_pElementModule = pPluginManager->FindModule<AFIElementModule>();

    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_STS_HEART_BEAT, this, &AFCMasterNet_ServerModule::OnHeartBeat);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_MTL_WORLD_REGISTERED, this, &AFCMasterNet_ServerModule::OnWorldRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_MTL_WORLD_UNREGISTERED, this, &AFCMasterNet_ServerModule::OnWorldUnRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_MTL_WORLD_REFRESH, this, &AFCMasterNet_ServerModule::OnRefreshWorldInfoProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_LTM_LOGIN_REGISTERED, this, &AFCMasterNet_ServerModule::OnLoginRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_LTM_LOGIN_UNREGISTERED, this, &AFCMasterNet_ServerModule::OnLoginUnRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_LTM_LOGIN_REFRESH, this, &AFCMasterNet_ServerModule::OnRefreshLoginInfoProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_CONNECT_WORLD, this, &AFCMasterNet_ServerModule::OnSelectWorldProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_ACK_CONNECT_WORLD, this, &AFCMasterNet_ServerModule::OnSelectServerResultProcess);
    m_pNetModule->AddReceiveCallBack(this, &AFCMasterNet_ServerModule::InvalidMessage);

    m_pNetModule->AddEventCallBack(this, &AFCMasterNet_ServerModule::OnSocketEvent);

    ARK_SHARE_PTR<AFIClass> xLogicClass = m_pClassModule->GetElement("Server");
    if(nullptr != xLogicClass)
    {
        NFList<std::string>& xNameList = xLogicClass->GetConfigNameList();
        std::string strConfigName;
        for(bool bRet = xNameList.First(strConfigName); bRet; bRet = xNameList.Next(strConfigName))
        {
            const int nServerType = m_pElementModule->GetPropertyInt(strConfigName, "Type");
            const int nServerID = m_pElementModule->GetPropertyInt(strConfigName, "ServerID");
            if(nServerType == NF_SERVER_TYPES::NF_ST_MASTER && pPluginManager->AppID() == nServerID)
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

void AFCMasterNet_ServerModule::OnSocketEvent(const NetEventType eEvent, const AFGUID& xClientID, const int nServerID)
{
    //std::cout << "OnSocketEvent::thread id=" << GetCurrentThreadId() << std::endl;

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

void AFCMasterNet_ServerModule::OnClientDisconnect(const AFGUID& xClientID)
{
    //������login����world��Ҫ�ҳ���,������ע��
    ARK_SHARE_PTR<ServerData> pServerData =  mWorldMap.First();
    while(nullptr != pServerData)
    {
        if(xClientID == pServerData->xClient)
        {
            pServerData->pData->set_server_state(AFMsg::EST_CRASH);
            pServerData->xClient = AFGUID(0);

            SynWorldToLogin();
            return;
        }

        pServerData = mWorldMap.Next();
    }

    //////////////////////////////////////////////////////////////////////////

    int nServerID = 0;
    pServerData =  mLoginMap.First();
    while(nullptr != pServerData)
    {
        if(xClientID == pServerData->xClient)
        {
            nServerID = pServerData->pData->server_id();
            break;
        }

        pServerData = mLoginMap.Next();
    }

    mLoginMap.RemoveElement(nServerID);

}

void AFCMasterNet_ServerModule::OnClientConnected(const AFGUID& xClientID)
{
    //��������ɶ������
}

void AFCMasterNet_ServerModule::SynWorldToLogin()
{
    AFMsg::ServerInfoReportList xData;

    ARK_SHARE_PTR<ServerData> pServerData =  mWorldMap.First();
    while(nullptr != pServerData)
    {
        AFMsg::ServerInfoReport* pData = xData.add_server_list();
        *pData = *(pServerData->pData);

        pServerData = mWorldMap.Next();
    }

    //�㲥������loginserver
    pServerData =  mLoginMap.First();
    while(nullptr != pServerData)
    {
        m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_STS_NET_INFO, xData, pServerData->xClient, AFGUID(0));

        pServerData = mLoginMap.Next();
    }
}

void AFCMasterNet_ServerModule::LogGameServer()
{
    if(mnLastLogTime + 10 > GetPluginManager()->GetNowTime())
    {
        return;
    }

    mnLastLogTime = GetPluginManager()->GetNowTime();

    //////////////////////////////////////////////////////////////////////////

    m_pLogModule->LogInfo(AFGUID(), "Begin Log WorldServer Info", "");

    ARK_SHARE_PTR<ServerData> pGameData = mWorldMap.First();
    while(pGameData)
    {
        std::ostringstream stream;
        stream << "Type: " << pGameData->pData->server_type() << " ID: " << pGameData->pData->server_id() << " State: " <<  AFMsg::EServerState_Name(pGameData->pData->server_state()) << " IP: " << pGameData->pData->server_ip() << " xClient: " << pGameData->xClient.n64Value;
        m_pLogModule->LogInfo(AFGUID(), stream);

        pGameData = mWorldMap.Next();
    }

    m_pLogModule->LogInfo(AFGUID(), "End Log WorldServer Info", "");

    m_pLogModule->LogInfo(AFGUID(), "Begin Log LoginServer Info", "");

    //////////////////////////////////////////////////////////////////////////
    pGameData = mLoginMap.First();
    while(pGameData)
    {
        std::ostringstream stream;
        stream << "Type: " << pGameData->pData->server_type() << " ID: " << pGameData->pData->server_id() << " State: " <<  AFMsg::EServerState_Name(pGameData->pData->server_state()) << " IP: " << pGameData->pData->server_ip() << " xClient: " << pGameData->xClient.n64Value;
        m_pLogModule->LogInfo(AFGUID(), stream);

        pGameData = mLoginMap.Next();
    }

    m_pLogModule->LogInfo(AFGUID(), "End Log LoginServer Info", "");

}

void AFCMasterNet_ServerModule::OnHeartBeat(const AFIMsgHead& xHead, const int nMsgID, const char * msg, const uint32_t nLen, const AFGUID& xClientID)
{
}

void AFCMasterNet_ServerModule::InvalidMessage(const AFIMsgHead& xHead, const int nMsgID, const char * msg, const uint32_t nLen, const AFGUID& xClientID)
{
    printf("NFNet || �Ƿ���Ϣ:unMsgID=%d\n", nMsgID);
}

