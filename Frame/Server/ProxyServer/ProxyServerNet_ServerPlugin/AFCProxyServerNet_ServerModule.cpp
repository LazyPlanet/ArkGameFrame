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
// * @file      AFCProxyServerNet_ServerModule.cpp                                              *
// * @author    Ark Game Tech                                                *
// * @date      2015-12-15                                                   *
// * @brief     AFCProxyServerNet_ServerModule                                                  *
*****************************************************************************/
#include "AFCProxyServerNet_ServerModule.h"
#include "AFProxyNetServerPlugin.h"
#include "SDK/Interface/AFIKernelModule.h"

bool AFCProxyServerNet_ServerModule::Init()
{
    m_pNetModule = ARK_NEW AFINetServerModule(pPluginManager);
    return true;
}

bool AFCProxyServerNet_ServerModule::AfterInit()
{
    m_pKernelModule = pPluginManager->FindModule<AFIKernelModule>();
    m_pClassModule = pPluginManager->FindModule<AFIClassModule>();
    m_pProxyToWorldModule = pPluginManager->FindModule<AFIProxyServerToWorldModule>();
    m_pLogModule = pPluginManager->FindModule<AFILogModule>();
    m_pElementModule = pPluginManager->FindModule<AFIElementModule>();
    m_pUUIDModule = pPluginManager->FindModule<AFIGUIDModule>();
    m_pProxyServerToGameModule = pPluginManager->FindModule<AFIProxyServerToGameModule>();

    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_CONNECT_KEY, this, &AFCProxyServerNet_ServerModule::OnConnectKeyProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_WORLD_LIST, this, &AFCProxyServerNet_ServerModule::OnReqServerListProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_SELECT_SERVER, this, &AFCProxyServerNet_ServerModule::OnSelectServerProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_ROLE_LIST, this, &AFCProxyServerNet_ServerModule::OnReqRoleListProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_CREATE_ROLE, this, &AFCProxyServerNet_ServerModule::OnReqCreateRoleProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_DELETE_ROLE, this, &AFCProxyServerNet_ServerModule::OnReqDelRoleProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_ENTER_GAME, this, &AFCProxyServerNet_ServerModule::OnReqEnterGameServer);
    m_pNetModule->AddReceiveCallBack(this, &AFCProxyServerNet_ServerModule::OnOtherMessage);

    m_pNetModule->AddEventCallBack(this, &AFCProxyServerNet_ServerModule::OnSocketClientEvent);

    ARK_SHARE_PTR<AFIClass> xLogicClass = m_pClassModule->GetElement("Server");
    if(nullptr != xLogicClass)
    {
        NFList<std::string>& xNameList = xLogicClass->GetConfigNameList();
        std::string strConfigName;
        for(bool bRet = xNameList.First(strConfigName); bRet; bRet = xNameList.Next(strConfigName))
        {
            const int nServerType = m_pElementModule->GetPropertyInt(strConfigName, "Type");
            const int nServerID = m_pElementModule->GetPropertyInt(strConfigName, "ServerID");
            if(nServerType == NF_SERVER_TYPES::NF_ST_PROXY && pPluginManager->AppID() == nServerID)
            {
                const int nPort = m_pElementModule->GetPropertyInt(strConfigName, "Port");
                const int nMaxConnect = m_pElementModule->GetPropertyInt(strConfigName, "MaxOnline");
                const int nCpus = m_pElementModule->GetPropertyInt(strConfigName, "CpuCount");
                const std::string strName(m_pElementModule->GetPropertyString(strConfigName, "Name"));
                const std::string strIP(m_pElementModule->GetPropertyString(strConfigName, "IP"));

                m_pUUIDModule->SetWorkerAndDatacenter(nServerID, nServerID);

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

bool AFCProxyServerNet_ServerModule::Shut()
{

    return true;
}

bool AFCProxyServerNet_ServerModule::Execute()
{
    return m_pNetModule->Execute();
}

int AFCProxyServerNet_ServerModule::HB_OnConnectCheckTime(const AFGUID& self, const std::string& strHeartBeat, const float fTime, const int nCount, const AFIDataList& var)
{
    m_pKernelModule->DestroyObject(self);

    return 0;
}

void AFCProxyServerNet_ServerModule::OnOtherMessage(const AFIMsgHead& xHead, const int nMsgID, const char * msg, const uint32_t nLen, const AFGUID& xClientID)
{
    ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
    if(!pSessionData || pSessionData->mnLogicState <= 0 || pSessionData->mnGameID <= 0)
    {
        //state error
        return;
    }

    if(pSessionData->mnUserID != xHead.GetPlayerID())
    {
        //when after entergame xHead.GetPlayerID() is really palyerID
        m_pLogModule->LogError(xHead.GetPlayerID(), "xHead.GetPlayerID() is not really palyerID", "", __FUNCTION__, __LINE__);
        return;
    }

    m_pProxyServerToGameModule->GetClusterModule()->SendByServerID(pSessionData->mnGameID, nMsgID, msg, nLen, xHead.GetPlayerID());
}

void AFCProxyServerNet_ServerModule::OnConnectKeyProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ReqAccountLogin xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    bool bRet = m_pProxyToWorldModule->VerifyConnectData(xMsg.account(), xMsg.security_code());
    if(bRet)
    {
        //���Խ���,���ñ�־��ѡ����,�����ӳ�,����gs������ɫ��ɾ����ɫ,����ֻ��ת��
        ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
        if(pSessionData)
        {
            pSessionData->mnLogicState = 1;
            pSessionData->mstrAccout  = xMsg.account();

            AFMsg::AckEventResult xSendMsg;
            xSendMsg.set_event_code(AFMsg::EGEC_VERIFY_KEY_SUCCESS);
            *xSendMsg.mutable_event_client() = AFINetServerModule::NFToPB(pSessionData->mnClientID);//��ǰ�˼ǵ��Լ���fd��������һЩ��֤
            *xSendMsg.mutable_event_object() = AFINetServerModule::NFToPB(nPlayerID);

            m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_ACK_CONNECT_KEY, xSendMsg, xClientID, nPlayerID);
        }
    }
    else
    {
        m_pNetModule->GetNet()->CloseNetObject(xClientID);
    }
}

void AFCProxyServerNet_ServerModule::OnSocketClientEvent(const NetEventType eEvent, const AFGUID& xClientID, const int nSeverID)
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

void AFCProxyServerNet_ServerModule::OnClientDisconnect(const AFGUID& xClientID)
{
    ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
    if(pSessionData)
    {
        if(pSessionData->mnGameID > 0)
        {
            if(!pSessionData->mnUserID.IsNull())
            {
                AFMsg::ReqLeaveGameServer xData;
                std::string strMsg;
                if(!xData.SerializeToString(&strMsg))
                {
                    return;
                }

                m_pProxyServerToGameModule->GetClusterModule()->SendByServerID(pSessionData->mnGameID, AFMsg::EGameMsgID::EGMI_REQ_LEAVE_GAME, strMsg, pSessionData->mnUserID);
            }
        }

        mmSessionData.RemoveElement(xClientID);
    }
}

void AFCProxyServerNet_ServerModule::OnSelectServerProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ReqSelectServer xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ConnectData> pServerData = m_pProxyServerToGameModule->GetClusterModule()->GetServerNetInfo(xMsg.world_id());
    if(pServerData && ConnectDataState::NORMAL == pServerData->eState)
    {
        ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
        if(pSessionData)
        {
            //now this client bind a game server, after this time, all message will be sent to this game server who bind with client
            pSessionData->mnGameID = xMsg.world_id();

            AFMsg::AckEventResult xMsg;
            xMsg.set_event_code(AFMsg::EGameEventCode::EGEC_SELECTSERVER_SUCCESS);
            m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_ACK_SELECT_SERVER, xMsg, xClientID, nPlayerID);
            return;
        }
    }

    AFMsg::AckEventResult xSendMsg;
    xSendMsg.set_event_code(AFMsg::EGameEventCode::EGEC_SELECTSERVER_FAIL);
    m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_ACK_SELECT_SERVER, xMsg, xClientID, nPlayerID);
}

void AFCProxyServerNet_ServerModule::OnReqServerListProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ReqServerList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    if(xMsg.type() != AFMsg::RSLT_GAMES_ERVER)
    {
        return;
    }

    ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
    if(pSessionData && pSessionData->mnLogicState > 0)
    {
        //ack all gameserver data
        AFMsg::AckServerList xData;
        xData.set_type(AFMsg::RSLT_GAMES_ERVER);

        AFMapEx<int, ConnectData>& xServerList = m_pProxyServerToGameModule->GetClusterModule()->GetServerList();
        ConnectData* pGameData = xServerList.FirstNude();
        while(NULL != pGameData)
        {
            if(ConnectDataState::NORMAL == pGameData->eState)
            {
                AFMsg::ServerInfo* pServerInfo = xData.add_info();

                pServerInfo->set_name(pGameData->strName);
                pServerInfo->set_status(AFMsg::EServerState::EST_NARMAL);
                pServerInfo->set_server_id(pGameData->nGameID);
                pServerInfo->set_wait_count(0);
            }

            pGameData = xServerList.NextNude();
        }

        m_pNetModule->SendMsgPB(AFMsg::EGameMsgID::EGMI_ACK_WORLD_LIST, xData, xClientID, nPlayerID);
    }
}

int AFCProxyServerNet_ServerModule::Transpond(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen)
{
    ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xHead.GetPlayerID());
    if(pSessionData)
    {
        m_pNetModule->GetNet()->SendMsgWithOutHead(nMsgID, msg, nLen, pSessionData->mnClientID, xHead.GetPlayerID());
    }

    return true;
}

int AFCProxyServerNet_ServerModule::SendToPlayerClient(const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID&  nClientID, const AFGUID&  nPlayer)
{
    m_pNetModule->GetNet()->SendMsgWithOutHead(nMsgID, msg, nLen, nClientID, nPlayer);

    return true;
}

void AFCProxyServerNet_ServerModule::OnClientConnected(const AFGUID& xClientID)
{
    ARK_SHARE_PTR<SessionData> pSessionData(ARK_NEW SessionData());

    pSessionData->mnClientID = xClientID;
    mmSessionData.AddElement(xClientID, pSessionData);
}

void AFCProxyServerNet_ServerModule::OnReqRoleListProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    //��û����ʽ������Ϸ֮ǰ��nPlayerID����FD
    AFGUID nPlayerID;
    AFMsg::ReqRoleList xData;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xData, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ConnectData> pServerData = m_pProxyServerToGameModule->GetClusterModule()->GetServerNetInfo(xData.game_id());
    if(pServerData && ConnectDataState::NORMAL == pServerData->eState)
    {
        //����ƥ��
        ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
        if(pSessionData
                && pSessionData->mnLogicState > 0
                && pSessionData->mnGameID == xData.game_id()
                && pSessionData->mstrAccout == xData.account())
        {
            std::string strMsg;
            if(!xData.SerializeToString(&strMsg))
            {
                return;
            }

            m_pProxyServerToGameModule->GetClusterModule()->SendByServerID(pSessionData->mnGameID, AFMsg::EGameMsgID::EGMI_REQ_ROLE_LIST, strMsg, xClientID);
        }
    }
}

void AFCProxyServerNet_ServerModule::OnReqCreateRoleProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    //��û����ʽ������Ϸ֮ǰ��nPlayerID����FD
    AFGUID nPlayerID;
    AFMsg::ReqCreateRole xData;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xData, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ConnectData> pServerData = m_pProxyServerToGameModule->GetClusterModule()->GetServerNetInfo(xData.game_id());
    if(pServerData && ConnectDataState::NORMAL == pServerData->eState)
    {
        //����ƥ��
        ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
        if(pSessionData
                && pSessionData->mnLogicState > 0
                && pSessionData->mnGameID == xData.game_id()
                && pSessionData->mstrAccout == xData.account())
        {
            std::string strMsg;
            if(!xData.SerializeToString(&strMsg))
            {
                return;
            }

            m_pProxyServerToGameModule->GetClusterModule()->SendByServerID(pSessionData->mnGameID, nMsgID, strMsg, pSessionData->mnClientID);
        }
    }
}

void AFCProxyServerNet_ServerModule::OnReqDelRoleProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    //��û����ʽ������Ϸ֮ǰ��nPlayerID����FD
    AFGUID nPlayerID;
    AFMsg::ReqDeleteRole xData;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xData, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ConnectData> pServerData = m_pProxyServerToGameModule->GetClusterModule()->GetServerNetInfo(xData.game_id());
    if(pServerData && ConnectDataState::NORMAL == pServerData->eState)
    {
        //����ƥ��
        ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
        if(pSessionData
                && pSessionData->mnLogicState > 0
                && pSessionData->mnGameID == xData.game_id()
                && pSessionData->mstrAccout == xData.account())
        {
            m_pProxyServerToGameModule->GetClusterModule()->SendByServerID(pSessionData->mnGameID, nMsgID, std::string(msg, nLen), xClientID);
        }
    }
}

void AFCProxyServerNet_ServerModule::OnReqEnterGameServer(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    //��û����ʽ������Ϸ֮ǰ��nPlayerID����FD
    AFGUID nPlayerID;
    AFMsg::ReqEnterGameServer xData;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xData, nPlayerID))
    {
        return;
    }

    ARK_SHARE_PTR<ConnectData> pServerData = m_pProxyServerToGameModule->GetClusterModule()->GetServerNetInfo(xData.game_id());
    if(pServerData && ConnectDataState::NORMAL == pServerData->eState)
    {
        //����ƥ��
        ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
        if(pSessionData
                && pSessionData->mnLogicState > 0
                && pSessionData->mnGameID == xData.game_id()
                && pSessionData->mstrAccout == xData.account()
                && !xData.name().empty()
                && !xData.account().empty())
        {
            std::string strMsg;
            if(!xData.SerializeToString(&strMsg))
            {
                return;
            }

            //playerid�ڽ�����Ϸ֮ǰ����FD������ʱ������ʵ��ID
            m_pProxyServerToGameModule->GetClusterModule()->SendByServerID(pSessionData->mnGameID, AFMsg::EGameMsgID::EGMI_REQ_ENTER_GAME, strMsg, pSessionData->mnClientID);
        }
    }
}

int AFCProxyServerNet_ServerModule::EnterGameSuccessEvent(const AFGUID xClientID, const AFGUID xPlayerID)
{
    ARK_SHARE_PTR<SessionData> pSessionData = mmSessionData.GetElement(xClientID);
    if(pSessionData)
    {
        pSessionData->mnUserID = xPlayerID;
    }

    return 0;
}

