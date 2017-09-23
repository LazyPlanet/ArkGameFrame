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
// * @file      AFCGameServerNet_ServerModule.cpp                                              *
// * @author    Ark Game Tech                                                *
// * @date      2015-12-15                                                   *
// * @brief     AFCGameServerNet_ServerModule                                                  *
*****************************************************************************/
#include "AFCGameServerNet_ServerModule.h"
#include "SDK/Interface/AFIModule.h"
#include "SDK/Proto/NFProtocolDefine.hpp"

bool AFCGameServerNet_ServerModule::Init()
{
    m_pNetModule = ARK_NEW AFINetServerModule(pPluginManager);
    return true;
}

bool AFCGameServerNet_ServerModule::AfterInit()
{
    m_pKernelModule = pPluginManager->FindModule<AFIKernelModule>();
    m_pClassModule = pPluginManager->FindModule<AFIClassModule>();
    m_pSceneProcessModule = pPluginManager->FindModule<AFISceneProcessModule>();
    m_pElementModule = pPluginManager->FindModule<AFIElementModule>();
    m_pLogModule = pPluginManager->FindModule<AFILogModule>();
    m_pUUIDModule = pPluginManager->FindModule<AFIGUIDModule>();
    m_pGameServerToWorldModule = pPluginManager->FindModule<AFIGameServerToWorldModule>();

    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_PTWG_PROXY_REFRESH, this, &AFCGameServerNet_ServerModule::OnRefreshProxyServerInfoProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_PTWG_PROXY_REGISTERED, this, &AFCGameServerNet_ServerModule::OnProxyServerRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_PTWG_PROXY_UNREGISTERED, this, &AFCGameServerNet_ServerModule::OnProxyServerUnRegisteredProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_ENTER_GAME, this, &AFCGameServerNet_ServerModule::OnClienEnterGameProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_LEAVE_GAME, this, &AFCGameServerNet_ServerModule::OnClienLeaveGameProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_ROLE_LIST, this, &AFCGameServerNet_ServerModule::OnReqiureRoleListProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_CREATE_ROLE, this, &AFCGameServerNet_ServerModule::OnCreateRoleGameProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_DELETE_ROLE, this, &AFCGameServerNet_ServerModule::OnDeleteRoleGameProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_RECOVER_ROLE, this, &AFCGameServerNet_ServerModule::OnClienSwapSceneProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_SWAP_SCENE, this, &AFCGameServerNet_ServerModule::OnClienSwapSceneProcess);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGMI_REQ_SEARCH_GUILD, this, &AFCGameServerNet_ServerModule::OnTransWorld);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGEC_REQ_CREATE_CHATGROUP, this, &AFCGameServerNet_ServerModule::OnTransWorld);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGEC_REQ_JOIN_CHATGROUP, this, &AFCGameServerNet_ServerModule::OnTransWorld);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGEC_REQ_LEAVE_CHATGROUP, this, &AFCGameServerNet_ServerModule::OnTransWorld);
    m_pNetModule->AddReceiveCallBack(AFMsg::EGEC_REQ_SUBSCRIPTION_CHATGROUP, this, &AFCGameServerNet_ServerModule::OnTransWorld);

    m_pNetModule->AddEventCallBack(this, &AFCGameServerNet_ServerModule::OnSocketPSEvent);

    m_pKernelModule->RegisterCommonClassEvent(this, &AFCGameServerNet_ServerModule::OnClassCommonEvent);
    m_pKernelModule->RegisterCommonPropertyEvent(this, &AFCGameServerNet_ServerModule::OnPropertyCommonEvent);
    m_pKernelModule->RegisterCommonRecordEvent(this, &AFCGameServerNet_ServerModule::OnRecordCommonEvent);

    m_pKernelModule->AddClassCallBack(NFrame::Player::ThisName(), this, &AFCGameServerNet_ServerModule::OnObjectClassEvent);

    ARK_SHARE_PTR<AFIClass> xLogicClass = m_pClassModule->GetElement("Server");
    if(nullptr != xLogicClass)
    {
        NFList<std::string>& xNameList = xLogicClass->GetConfigNameList();
        std::string strConfigName;
        for(bool bRet = xNameList.First(strConfigName); bRet; bRet = xNameList.Next(strConfigName))
        {
            const int nServerType = m_pElementModule->GetPropertyInt(strConfigName, "Type");
            const int nServerID = m_pElementModule->GetPropertyInt(strConfigName, "ServerID");
            if(nServerType == NF_SERVER_TYPES::NF_ST_GAME && pPluginManager->AppID() == nServerID)
            {
                const int nPort = m_pElementModule->GetPropertyInt(strConfigName, "Port");
                const int nMaxConnect = m_pElementModule->GetPropertyInt(strConfigName, "MaxOnline");
                const int nCpus = m_pElementModule->GetPropertyInt(strConfigName, "CpuCount");
                const std::string strName(m_pElementModule->GetPropertyString(strConfigName, "Name"));
                const std::string strIP(m_pElementModule->GetPropertyString(strConfigName, "IP"));

                int nRet = m_pNetModule->Initialization(nMaxConnect, strIP, nPort, nServerID, nCpus);
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

bool AFCGameServerNet_ServerModule::Shut()
{

    return true;
}

bool AFCGameServerNet_ServerModule::Execute()
{
    return m_pNetModule->Execute();
}

void AFCGameServerNet_ServerModule::OnSocketPSEvent(const NetEventType eEvent, const AFGUID& xClientID, const int nServerID)
{
    if(eEvent == DISCONNECTED)
    {
        m_pLogModule->LogInfo(xClientID, "NF_NET_EVENT_EOF", "Connection closed", __FUNCTION__, __LINE__);
        OnClientDisconnect(xClientID);
    }
    else  if(eEvent == CONNECTED)
    {
        m_pLogModule->LogInfo(xClientID, "NF_NET_EVENT_CONNECTED", "connected success", __FUNCTION__, __LINE__);
        OnClientConnected(xClientID);
    }
}

void AFCGameServerNet_ServerModule::OnClientDisconnect(const AFGUID& xClientID)
{
    //ֻ���������ض���
    int nServerID = 0;
    ARK_SHARE_PTR<GateServerInfo> pServerData = mProxyMap.First();
    while(nullptr != pServerData)
    {
        if(xClientID == pServerData->xServerData.xClient)
        {
            nServerID = pServerData->xServerData.pData->server_id();
            break;
        }

        pServerData = mProxyMap.Next();
    }

    mProxyMap.RemoveElement(nServerID);
}

void AFCGameServerNet_ServerModule::OnClientConnected(const AFGUID& xClientID)
{

}

void AFCGameServerNet_ServerModule::OnClienEnterGameProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    //�ڽ�����Ϸ֮ǰnPlayerIDΪ�������ص�FD
    AFGUID nGateClientID;
    AFMsg::ReqEnterGameServer xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nGateClientID))
    {
        return;
    }

    AFGUID nRoleID = AFINetServerModule::PBToNF(xMsg.id());

    if(m_pKernelModule->GetObject(nRoleID))
    {
        m_pKernelModule->DestroyObject(nRoleID);
    }

    //////////////////////////////////////////////////////////////////////////

    ARK_SHARE_PTR<AFCGameServerNet_ServerModule::GateBaseInfo>  pGateInfo = GetPlayerGateInfo(nRoleID);
    if(nullptr != pGateInfo)
    {
        RemovePlayerGateInfo(nRoleID);
    }

    ARK_SHARE_PTR<AFCGameServerNet_ServerModule::GateServerInfo> pGateServereinfo = GetGateServerInfoByClientID(xClientID);
    if(nullptr == pGateServereinfo)
    {
        return;
    }

    int nGateID = -1;
    if(pGateServereinfo->xServerData.pData)
    {
        nGateID = pGateServereinfo->xServerData.pData->server_id();
    }

    if(nGateID < 0)
    {
        return;
    }

    if(!AddPlayerGateInfo(nRoleID, nGateClientID, nGateID))
    {
        return;
    }

    //Ĭ��1�ų���
    int nSceneID = 1;
    AFCDataList var;
    var.AddString("Name");
    var.AddString(xMsg.name().c_str());

    var.AddString("GateID");
    var.AddInt(nGateID);

    var.AddString("ClientID");
    var.AddObject(nGateClientID);

    ARK_SHARE_PTR<AFIObject> pObject = m_pKernelModule->CreateObject(nRoleID, nSceneID, 0, NFrame::Player::ThisName(), "", var);
    if(nullptr == pObject)
    {
        //�ڴ�й©
        //mRoleBaseData
        //mRoleFDData
        return;
    }

    pObject->SetPropertyInt("LoadPropertyFinish", 1);
    pObject->SetPropertyInt("GateID", nGateID);
    pObject->SetPropertyInt("GameID", pPluginManager->AppID());

    m_pKernelModule->DoEvent(pObject->Self(), NFrame::Player::ThisName(), CLASS_OBJECT_EVENT::COE_CREATE_FINISH, AFCDataList());

    AFCDataList varEntry;
    varEntry << pObject->Self();
    varEntry << int32_t(0);
    varEntry << (int32_t)nSceneID;
    varEntry << int32_t (-1);
    m_pKernelModule->DoEvent(pObject->Self(), AFED_ON_CLIENT_ENTER_SCENE, varEntry);
}

void AFCGameServerNet_ServerModule::OnClienLeaveGameProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ReqLeaveGameServer xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }

    if(nPlayerID.IsNull())
    {
        return;
    }

    if(m_pKernelModule->GetObject(nPlayerID))
    {
        m_pKernelModule->DestroyObject(nPlayerID);
    }

    RemovePlayerGateInfo(nPlayerID);
}

int AFCGameServerNet_ServerModule::OnPropertyEnter(const AFIDataList& argVar, const AFGUID& self)
{
    if(argVar.GetCount() <= 0 || self.IsNull())
    {
        return 0;
    }

    AFMsg::MultiObjectPropertyList xPublicMsg;
    AFMsg::MultiObjectPropertyList xPrivateMsg;

    //��Ϊ�Լ�������
    //1.public���͸�������
    //2.����Լ����б��У��ٴη���private����
    ARK_SHARE_PTR<AFIObject> pObject = m_pKernelModule->GetObject(self);
    if(nullptr != pObject)
    {
        AFMsg::ObjectPropertyList* pPublicData = xPublicMsg.add_multi_player_property();
        AFMsg::ObjectPropertyList* pPrivateData = xPrivateMsg.add_multi_player_property();

        *(pPublicData->mutable_player_id()) = AFINetServerModule::NFToPB(self);
        *(pPrivateData->mutable_player_id()) = AFINetServerModule::NFToPB(self);

        ARK_SHARE_PTR<AFIPropertyMgr> pPropertyManager = pObject->GetPropertyManager();

        for(int i = 0; i < pPropertyManager->GetPropertyCount(); i++)
        {
            AFProperty* pPropertyInfo = pPropertyManager->GetPropertyByIndex(i);
            if(NULL == pPropertyInfo)
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
            AFGUID identOther = argVar.Object(i);
            if(self == identOther)
            {
                //�ҵ����������ص�FD
                SendMsgPBToGate(AFMsg::EGMI_ACK_OBJECT_PROPERTY_ENTRY, xPrivateMsg, identOther);
            }
            else
            {
                SendMsgPBToGate(AFMsg::EGMI_ACK_OBJECT_PROPERTY_ENTRY, xPublicMsg, identOther);
            }
        }
    }

    return 0;
}

bool OnRecordEnterPack(AFRecord* pRecord, AFMsg::ObjectRecordBase* pObjectRecordBase)
{
    if(!pRecord || !pObjectRecordBase)
    {
        return false;
    }

    for(int i = 0; i < pRecord->GetRowCount(); i++)
    {
        AFMsg::RecordAddRowStruct* pAddRowStruct = pObjectRecordBase->add_row_struct();
        pAddRowStruct->set_row(i);
        for(int j = 0; j < pRecord->GetColCount(); j++)
        {
            AFMsg::RecordPBData* pAddData = pAddRowStruct->add_record_data_list();

            AFCData xRowColData;
            if(!pRecord->GetValue(i, j, xRowColData))
            {
                ARK_ASSERT(0, "Get record value failed, please check", __FILE__, __FUNCTION__);
                continue;
            }

            AFINetServerModule::RecordToPBRecord(xRowColData, i, j, *pAddData);
        }
    }

    return true;
}

int AFCGameServerNet_ServerModule::OnRecordEnter(const AFIDataList& argVar, const AFGUID& self)
{
    if(argVar.GetCount() <= 0 || self.IsNull())
    {
        return 0;
    }

    AFMsg::MultiObjectRecordList xPublicMsg;
    AFMsg::MultiObjectRecordList xPrivateMsg;

    ARK_SHARE_PTR<AFIObject> pObject = m_pKernelModule->GetObject(self);
    if(nullptr == pObject)
    {
        return 0;
    }

    AFMsg::ObjectRecordList* pPublicData = NULL;
    AFMsg::ObjectRecordList* pPrivateData = NULL;

    ARK_SHARE_PTR<AFIRecordMgr> pRecordManager = pObject->GetRecordManager();

    size_t nRecordCount = pRecordManager->GetCount();
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
        AFGUID identOther = argVar.Object(i);
        if(self == identOther)
        {
            if(xPrivateMsg.multi_player_record_size() > 0)
            {
                SendMsgPBToGate(AFMsg::EGMI_ACK_OBJECT_RECORD_ENTRY, xPrivateMsg, identOther);
            }
        }
        else
        {
            if(xPublicMsg.multi_player_record_size() > 0)
            {
                SendMsgPBToGate(AFMsg::EGMI_ACK_OBJECT_RECORD_ENTRY, xPublicMsg, identOther);
            }
        }
    }

    return 0;
}

int AFCGameServerNet_ServerModule::OnObjectListEnter(const AFIDataList& self, const AFIDataList& argVar)
{
    if(self.GetCount() <= 0 || argVar.GetCount() <= 0)
    {
        return 0;
    }

    AFMsg::AckPlayerEntryList xPlayerEntryInfoList;
    for(int i = 0; i < argVar.GetCount(); i++)
    {
        AFGUID identOld = argVar.Object(i);
        //�ų��ն���
        if(identOld.IsNull())
        {
            continue;
        }

        AFMsg::PlayerEntryInfo* pEntryInfo = xPlayerEntryInfoList.add_object_list();
        *(pEntryInfo->mutable_object_guid()) = AFINetServerModule::NFToPB(identOld);
        Point3D xPoint;
        xPoint.x = m_pKernelModule->GetPropertyFloat(identOld, "x");
        xPoint.y = m_pKernelModule->GetPropertyFloat(identOld, "y");
        xPoint.z = m_pKernelModule->GetPropertyFloat(identOld, "z");

        *pEntryInfo->mutable_pos() = AFINetServerModule::NFToPB(xPoint);
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

        //�����ڲ�ͬ��������,�õ��������ڵ�����FD
        SendMsgPBToGate(AFMsg::EGMI_ACK_OBJECT_ENTRY, xPlayerEntryInfoList, ident);
    }

    return 1;
}

int AFCGameServerNet_ServerModule::OnObjectListLeave(const AFIDataList& self, const AFIDataList& argVar)
{
    if(self.GetCount() <= 0 || argVar.GetCount() <= 0)
    {
        return 0;
    }

    AFMsg::AckPlayerLeaveList xPlayerLeaveInfoList;
    for(int i = 0; i < argVar.GetCount(); i++)
    {
        AFGUID identOld = argVar.Object(i);
        //�ų��ն���
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
        //�����ڲ�ͬ��������,�õ��������ڵ�����FD
        SendMsgPBToGate(AFMsg::EGMI_ACK_OBJECT_LEAVE, xPlayerLeaveInfoList, ident);
    }

    return 1;
}


int AFCGameServerNet_ServerModule::OnPropertyCommonEvent(const AFGUID& self, const std::string& strPropertyName, const AFIData& oldVar, const AFIData& newVar)
{
    //if ( NFrame::Player::ThisName() == m_pKernelModule->GetPropertyString( self, "ClassName" ) )
    {
        if("GroupID" == strPropertyName)
        {
            //�Լ�����Ҫ֪���Լ���������Ա仯��,���Ǳ��˾Ͳ���Ҫ֪����
            OnGroupEvent(self, strPropertyName, oldVar, newVar);
        }

        if("SceneID" == strPropertyName)
        {
            //�Լ�����Ҫ֪���Լ���������Ա仯��,���Ǳ��˾Ͳ���Ҫ֪����
            OnContainerEvent(self, strPropertyName, oldVar, newVar);
        }

        if(NFrame::Player::ThisName() == std::string(m_pKernelModule->GetPropertyString(self, "ClassName")))
        {
            if(m_pKernelModule->GetPropertyInt(self, "LoadPropertyFinish") <= 0)
            {
                return 0;
            }
        }
    }

    AFCDataList valueBroadCaseList;
    int nCount = 0;//argVar.GetCount() ;
    if(nCount <= 0)
    {
        nCount = GetBroadCastObject(self, strPropertyName, false, valueBroadCaseList);
    }
    else
    {
        //����Ĳ�����Ҫ�㲥�Ķ����б�
        //valueBroadCaseList = argVar;
    }

    if(valueBroadCaseList.GetCount() <= 0)
    {
        return 0;
    }

    AFMsg::ObjectPropertyPBData xPropertyData;
    AFMsg::Ident* pIdent = xPropertyData.mutable_player_id();
    *pIdent = AFINetServerModule::NFToPB(self);
    AFMsg::PropertyPBData* pData = xPropertyData.add_property_list();
    AFINetServerModule::DataToPBProperty(oldVar, strPropertyName.c_str(), *pData);

    for(int i = 0; i < valueBroadCaseList.GetCount(); i++)
    {
        AFGUID identOld = valueBroadCaseList.Object(i);

        SendMsgPBToGate(AFMsg::EGMI_ACK_PROPERTY_DATA, xPropertyData, identOld);
    }

    return 0;
}

int AFCGameServerNet_ServerModule::OnRecordCommonEvent(const AFGUID& self, const RECORD_EVENT_DATA& xEventData, const AFIData& oldVar, const AFIData& newVar)
{
    const std::string& strRecordName = xEventData.strRecordName;
    const int nOpType = xEventData.nOpType;
    const int nRow = xEventData.nRow;
    const int nCol = xEventData.nCol;

    int nObjectContainerID = m_pKernelModule->GetPropertyInt(self, "SceneID");
    int nObjectGroupID = m_pKernelModule->GetPropertyInt(self, "GroupID");

    if(nObjectGroupID < 0)
    {
        //����
        return 0;
    }

    if(NFrame::Player::ThisName() == std::string(m_pKernelModule->GetPropertyString(self, "ClassName")))
    {
        if(m_pKernelModule->GetPropertyInt(self, "LoadPropertyFinish") <= 0)
        {
            return 0;
        }
    }

    AFCDataList valueBroadCaseList;
    GetBroadCastObject(self, strRecordName, true, valueBroadCaseList);

    switch(nOpType)
    {
    case AFRecord::RecordOptype::Add:
        {
            AFMsg::ObjectRecordAddRow xAddRecordRow;
            AFMsg::Ident* pIdent = xAddRecordRow.mutable_player_id();
            *pIdent = AFINetServerModule::NFToPB(self);

            xAddRecordRow.set_record_name(strRecordName);

            AFMsg::RecordAddRowStruct* pAddRowData = xAddRecordRow.add_row_data();
            pAddRowData->set_row(nRow);

            //add row ��Ҫ������row
            AFRecord* xRecord = m_pKernelModule->FindRecord(self, strRecordName);
            if(xRecord)
            {
                AFCDataList xRowDataList;
                if(xRecord->QueryRow(nRow, xRowDataList))
                {
                    for(int i = 0; i < xRowDataList.GetCount(); i++)
                    {
                        AFMsg::RecordPBData* pAddData = pAddRowData->add_record_data_list();
                        AFINetServerModule::RecordToPBRecord(xRowDataList, nRow, nCol, *pAddData);
                    }

                    for(int i = 0; i < valueBroadCaseList.GetCount(); i++)
                    {
                        AFGUID identOther = valueBroadCaseList.Object(i);

                        SendMsgPBToGate(AFMsg::EGMI_ACK_ADD_ROW, xAddRecordRow, identOther);
                    }
                }
            }
        }
        break;
    case AFRecord::RecordOptype::Del:
        {
            AFMsg::ObjectRecordRemove xReoveRecordRow;

            AFMsg::Ident* pIdent = xReoveRecordRow.mutable_player_id();
            *pIdent = AFINetServerModule::NFToPB(self);

            xReoveRecordRow.set_record_name(strRecordName);
            xReoveRecordRow.add_remove_row(nRow);

            for(int i = 0; i < valueBroadCaseList.GetCount(); i++)
            {
                AFGUID identOther = valueBroadCaseList.Object(i);

                SendMsgPBToGate(AFMsg::EGMI_ACK_REMOVE_ROW, xReoveRecordRow, identOther);
            }
        }
        break;
    case AFRecord::RecordOptype::Swap:
        {
            //��ʵ��2��row����
            AFMsg::ObjectRecordSwap xSwapRecord;
            *xSwapRecord.mutable_player_id() = AFINetServerModule::NFToPB(self);

            xSwapRecord.set_origin_record_name(strRecordName);
            xSwapRecord.set_target_record_name(strRecordName);   // ��ʱû��
            xSwapRecord.set_row_origin(nRow);
            xSwapRecord.set_row_target(nCol);

            for(int i = 0; i < valueBroadCaseList.GetCount(); i++)
            {
                AFGUID identOther = valueBroadCaseList.Object(i);

                SendMsgPBToGate(AFMsg::EGMI_ACK_SWAP_ROW, xSwapRecord, identOther);
            }
        }
        break;
    case AFRecord::RecordOptype::Update:
        {
            AFMsg::ObjectRecordPBData xRecordChanged;
            *xRecordChanged.mutable_player_id() = AFINetServerModule::NFToPB(self);
            xRecordChanged.set_record_name(strRecordName);
            AFMsg::RecordPBData* recordProperty = xRecordChanged.add_record_list();
            AFINetServerModule::RecordToPBRecord(newVar, nRow, nCol, *recordProperty);

            for(int i = 0; i < valueBroadCaseList.GetCount(); i++)
            {
                AFGUID identOther = valueBroadCaseList.Object(i);

                SendMsgPBToGate(AFMsg::EGMI_ACK_RECORD_DATA, xRecordChanged, identOther);
            }
        }
        break;
    case AFRecord::RecordOptype::Create:
        break;
    case AFRecord::RecordOptype::Cleared:
        break;
    default:
        break;
    }

    return 0;
}

int AFCGameServerNet_ServerModule::OnClassCommonEvent(const AFGUID& self, const std::string& strClassName, const CLASS_OBJECT_EVENT eClassEvent, const AFIDataList& var)
{
    ////////////1:�㲥���Ѿ����ڵ���//////////////////////////////////////////////////////////////
    if(CLASS_OBJECT_EVENT::COE_DESTROY == eClassEvent)
    {
        //ɾ�����߱�־

        //////////////////////////////////////////////////////////////////////////

        int nObjectContainerID = m_pKernelModule->GetPropertyInt(self, "SceneID");
        int nObjectGroupID = m_pKernelModule->GetPropertyInt(self, "GroupID");

        if(nObjectGroupID < 0)
        {
            //����
            return 0;
        }

        AFCDataList valueAllObjectList;
        AFCDataList valueBroadCaseList;
        AFCDataList valueBroadListNoSelf;
        m_pKernelModule->GetGroupObjectList(nObjectContainerID, nObjectGroupID, valueAllObjectList);

        for(int i = 0; i < valueAllObjectList.GetCount(); i++)
        {
            AFGUID identBC = valueAllObjectList.Object(i);
            const std::string strClassName(m_pKernelModule->GetPropertyString(identBC, "ClassName"));
            if(NFrame::Player::ThisName() == strClassName)
            {
                valueBroadCaseList << identBC;
                if(identBC != self)
                {
                    valueBroadListNoSelf << identBC;
                }
            }
        }

        //����Ǹ����Ĺ֣�����Ҫ���ͣ���Ϊ�����뿪������ʱ��һ����һ����Ϣ����
        OnObjectListLeave(valueBroadListNoSelf, AFCDataList() << self);
    }

    else if(CLASS_OBJECT_EVENT::COE_CREATE_NODATA == eClassEvent)
    {
        //id��fd,gateid��
        ARK_SHARE_PTR<GateBaseInfo> pDataBase = mRoleBaseData.GetElement(self);
        if(nullptr != pDataBase)
        {
            //�ظ��ͻ��˽�ɫ������Ϸ����ɹ���
            AFMsg::AckEventResult xMsg;
            xMsg.set_event_code(AFMsg::EGEC_ENTER_GAME_SUCCESS);

            *xMsg.mutable_event_client() = AFINetServerModule::NFToPB(pDataBase->xClientID);
            *xMsg.mutable_event_object() = AFINetServerModule::NFToPB(self);

            SendMsgPBToGate(AFMsg::EGMI_ACK_ENTER_GAME, xMsg, self);
        }
    }
    else if(CLASS_OBJECT_EVENT::COE_CREATE_LOADDATA == eClassEvent)
    {
    }
    else if(CLASS_OBJECT_EVENT::COE_CREATE_HASDATA == eClassEvent)
    {
        //�Լ��㲥���Լ��͹���
        if(strClassName == NFrame::Player::ThisName())
        {
            OnObjectListEnter(AFCDataList() << self, AFCDataList() << self);

            OnPropertyEnter(AFCDataList() << self, self);
            OnRecordEnter(AFCDataList() << self, self);
        }
    }
    else if(CLASS_OBJECT_EVENT::COE_CREATE_FINISH == eClassEvent)
    {

    }
    return 0;
}

int AFCGameServerNet_ServerModule::OnGroupEvent(const AFGUID& self, const std::string& strPropertyName, const AFIData& oldVar, const AFIData& newVar)
{
    //���������仯��ֻ���ܴ�A������0���л���B������0��
    //��Ҫע�����------------�κβ�ı��ʱ�򣬴������ʵ��δ����㣬��ˣ���ı��ʱ���ȡ������б�Ŀ����ǲ������Լ���
    int nSceneID = m_pKernelModule->GetPropertyInt(self, "SceneID");

    //�㲥�������Լ���ȥ(�㽵����Ծ��)
    int nOldGroupID = oldVar.GetInt();
    if(nOldGroupID > 0)
    {
        AFCDataList valueAllOldObjectList;
        AFCDataList valueAllOldPlayerList;
        m_pKernelModule->GetGroupObjectList(nSceneID, nOldGroupID, valueAllOldObjectList);
        if(valueAllOldObjectList.GetCount() > 0)
        {
            //�Լ�ֻ��Ҫ�㲥�������
            for(int i = 0; i < valueAllOldObjectList.GetCount(); i++)
            {
                AFGUID identBC = valueAllOldObjectList.Object(i);

                if(valueAllOldObjectList.Object(i) == self)
                {
                    valueAllOldObjectList.SetObject(i, AFGUID());
                }

                const std::string strClassName(m_pKernelModule->GetPropertyString(identBC, "ClassName"));
                if(NFrame::Player::ThisName() == strClassName)
                {
                    valueAllOldPlayerList << identBC;
                }
            }

            OnObjectListLeave(valueAllOldPlayerList, AFCDataList() << self);

            //�ϵ�ȫ��Ҫ�㲥ɾ��
            OnObjectListLeave(AFCDataList() << self, valueAllOldObjectList);
        }

        m_pKernelModule->DoEvent(self, AFED_ON_CLIENT_LEAVE_SCENE, AFCDataList() << nOldGroupID);
    }

    //�ٹ㲥�������Լ�����(��������Ծ��)
    int nNewGroupID = newVar.GetInt();
    if(nNewGroupID > 0)
    {
        //������Ҫ���Լ��ӹ㲥���ų�
        //////////////////////////////////////////////////////////////////////////
        AFCDataList valueAllObjectList;
        AFCDataList valueAllObjectListNoSelf;
        AFCDataList valuePlayerList;
        AFCDataList valuePlayerListNoSelf;
        m_pKernelModule->GetGroupObjectList(nSceneID, nNewGroupID, valueAllObjectList);
        for(int i = 0; i < valueAllObjectList.GetCount(); i++)
        {
            AFGUID identBC = valueAllObjectList.Object(i);
            const std::string strClassName(m_pKernelModule->GetPropertyString(identBC, "ClassName"));
            if(NFrame::Player::ThisName() == strClassName)
            {
                valuePlayerList << identBC;
                if(identBC != self)
                {
                    valuePlayerListNoSelf << identBC;
                }
            }

            if(identBC != self)
            {
                valueAllObjectListNoSelf << identBC;
            }
        }

        //�㲥������,�Լ�����(���ﱾ��Ӧ�ù㲥���Լ�)
        if(valuePlayerListNoSelf.GetCount() > 0)
        {
            OnObjectListEnter(valuePlayerListNoSelf, AFCDataList() << self);
        }

        const std::string strSelfClassName(m_pKernelModule->GetPropertyString(self, "ClassName"));

        //�㲥���Լ�,���еı��˳���
        if(valueAllObjectListNoSelf.GetCount() > 0)
        {
            if(strSelfClassName == NFrame::Player::ThisName())
            {
                OnObjectListEnter(AFCDataList() << self, valueAllObjectListNoSelf);
            }
        }

        if(strSelfClassName == NFrame::Player::ThisName())
        {
            for(int i = 0; i < valueAllObjectListNoSelf.GetCount(); i++)
            {
                //��ʱ�����ٹ㲥�Լ������Ը��Լ�
                //���Ѿ����ڵ��˵����Թ㲥����������
                AFGUID identOld = valueAllObjectListNoSelf.Object(i);

                OnPropertyEnter(AFCDataList() << self, identOld);
                //���Ѿ����ڵ��˵ı�㲥����������
                OnRecordEnter(AFCDataList() << self, identOld);
            }
        }

        //���������˵����Թ㲥���ܱߵ���
        if(valuePlayerListNoSelf.GetCount() > 0)
        {
            OnPropertyEnter(valuePlayerListNoSelf, self);
            OnRecordEnter(valuePlayerListNoSelf, self);
        }
    }

    return 0;
}

int AFCGameServerNet_ServerModule::OnContainerEvent(const AFGUID& self, const std::string& strPropertyName, const AFIData& oldVar, const AFIData& newVar)
{
    //���������仯��ֻ���ܴ�A������0���л���B������0��
    //��Ҫע�����------------�κ������ı��ʱ����ұ�����0��
    int nOldSceneID = oldVar.GetInt();
    int nNowSceneID = newVar.GetInt();

    m_pLogModule->LogInfo(self, "Enter Scene:", nNowSceneID);

    //�Լ���ʧ,��Ҳ��ù㲥����Ϊ����ʧ֮ǰ����ص�0�㣬���ѹ㲥�����
    AFCDataList valueOldAllObjectList;
    AFCDataList valueNewAllObjectList;
    AFCDataList valueAllObjectListNoSelf;
    AFCDataList valuePlayerList;
    AFCDataList valuePlayerNoSelf;

    m_pKernelModule->GetGroupObjectList(nOldSceneID, 0, valueOldAllObjectList);
    m_pKernelModule->GetGroupObjectList(nNowSceneID, 0, valueNewAllObjectList);

    for(int i = 0; i < valueOldAllObjectList.GetCount(); i++)
    {
        AFGUID identBC = valueOldAllObjectList.Object(i);
        if(identBC == self)
        {
            valueOldAllObjectList.SetObject(i, AFGUID());
        }
    }

    for(int i = 0; i < valueNewAllObjectList.GetCount(); i++)
    {
        AFGUID identBC = valueNewAllObjectList.Object(i);
        const std::string strClassName(m_pKernelModule->GetPropertyString(identBC, "ClassName"));
        if(NFrame::Player::ThisName() == strClassName)
        {
            valuePlayerList << identBC;
            if(identBC != self)
            {
                valuePlayerNoSelf << identBC;
            }
        }

        if(identBC != self)
        {
            valueAllObjectListNoSelf << identBC;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    //���Ǿɳ���0���NPC��Ҫ�㲥
    OnObjectListLeave(AFCDataList() << self, valueOldAllObjectList);

    //�㲥�������˳��ֶ���(�������ң�������㲥���Լ�)
    //����㲥�Ķ���0���
    if(valuePlayerList.GetCount() > 0)
    {
        //��self�㲥��argVar��Щ��
        OnObjectListEnter(valuePlayerNoSelf, AFCDataList() << self);
    }

    //�²��Ȼ��0����0��NPC�㲥���Լ�------------�Լ��㲥���Լ���������㲥����Ϊ����ID�ڿ糡��ʱ�ᾭ���仯

    //��valueAllObjectList�㲥��self
    OnObjectListEnter(AFCDataList() << self, valueAllObjectListNoSelf);

    ////////////////////���Ѿ����ڵ��˵����Թ㲥����������//////////////////////////////////////////////////////
    for(int i = 0; i < valueAllObjectListNoSelf.GetCount(); i++)
    {
        AFGUID identOld = valueAllObjectListNoSelf.Object(i);
        OnPropertyEnter(AFCDataList() << self, identOld);
        ////////////////////���Ѿ����ڵ��˵ı�㲥����������//////////////////////////////////////////////////////
        OnRecordEnter(AFCDataList() << self, identOld);
    }

    //���������˵����Թ㲥���ܱߵ���()
    if(valuePlayerNoSelf.GetCount() > 0)
    {
        OnPropertyEnter(valuePlayerNoSelf, self);
        OnRecordEnter(valuePlayerNoSelf, self);
    }

    return 0;
}

int AFCGameServerNet_ServerModule::GetBroadCastObject(const AFGUID& self, const std::string& strPropertyName, const bool bTable, AFIDataList& valueObject)
{
    int nObjectContainerID = m_pKernelModule->GetPropertyInt(self, "SceneID");
    int nObjectGroupID = m_pKernelModule->GetPropertyInt(self, "GroupID");

    //��ͨ�����������жϹ㲥����
    std::string strClassName = m_pKernelModule->GetPropertyString(self, "ClassName");

    ARK_SHARE_PTR<AFIPropertyMgr> pClassPropertyManager = m_pClassModule->GetClassPropertyManager(strClassName);
    ARK_SHARE_PTR<AFIRecordMgr> pClassRecordManager = m_pClassModule->GetClassRecordManager(strClassName);

    AFRecord* pRecord = NULL;
    AFProperty* pProperty(nullptr);
    if(bTable)
    {
        if(nullptr == pClassRecordManager)
        {
            return -1;
        }

        pRecord = pClassRecordManager->GetRecord(strPropertyName.c_str());
        if(nullptr == pRecord)
        {
            return -1;
        }
    }
    else
    {
        if(nullptr == pClassPropertyManager)
        {
            return -1;
        }

        pProperty = pClassPropertyManager->GetProperty(strPropertyName.c_str());
        if(nullptr == pProperty)
        {
            return -1;
        }
    }

    if(NFrame::Player::ThisName() == strClassName)
    {
        if(bTable)
        {
            if(pRecord->IsPublic())
            {
                GetBroadCastObject(nObjectContainerID, nObjectGroupID, valueObject);
            }
            else if(pRecord->IsPrivate())
            {
                valueObject.AddObject(self);
            }
        }
        else
        {
            if(pProperty->IsPublic())
            {
                GetBroadCastObject(nObjectContainerID, nObjectGroupID, valueObject);
            }
            else if(pProperty->IsPrivate())
            {
                valueObject.AddObject(self);
            }
        }
        //һ����Ҷ����㲥
        return valueObject.GetCount();
    }

    //�������,NPC�͹������
    if(bTable)
    {
        if(pRecord->IsPublic())
        {
            //�㲥���ͻ����Լ����ܱ���
            GetBroadCastObject(nObjectContainerID, nObjectGroupID, valueObject);
        }
    }
    else
    {
        if(pProperty->IsPublic())
        {
            //�㲥���ͻ����Լ����ܱ���
            GetBroadCastObject(nObjectContainerID, nObjectGroupID, valueObject);
        }
    }

    return valueObject.GetCount();
}

int AFCGameServerNet_ServerModule::GetBroadCastObject(const int nObjectContainerID, const int nGroupID, AFIDataList& valueObject)
{
    AFCDataList valContainerObjectList;
    m_pKernelModule->GetGroupObjectList(nObjectContainerID, nGroupID, valContainerObjectList);
    for(int i = 0; i < valContainerObjectList.GetCount(); i++)
    {
        const std::string& strObjClassName = m_pKernelModule->GetPropertyString(valContainerObjectList.Object(i), "ClassName");
        if(NFrame::Player::ThisName() == strObjClassName)
        {
            valueObject.AddObject(valContainerObjectList.Object(i));
        }
    }

    return valueObject.GetCount();
}

int AFCGameServerNet_ServerModule::OnObjectClassEvent(const AFGUID& self, const std::string& strClassName, const CLASS_OBJECT_EVENT eClassEvent, const AFIDataList& var)
{
    if(CLASS_OBJECT_EVENT::COE_DESTROY == eClassEvent)
    {
        //SaveDataToNoSql( self, true );
        m_pLogModule->LogInfo(self, "Player Offline", "");
    }
    else if(CLASS_OBJECT_EVENT::COE_CREATE_LOADDATA == eClassEvent)
    {
        //LoadDataFormNoSql( self );
    }
    else if(CLASS_OBJECT_EVENT::COE_CREATE_FINISH == eClassEvent)
    {
        m_pKernelModule->AddEventCallBack(self, AFED_ON_OBJECT_ENTER_SCENE_BEFORE, this, &AFCGameServerNet_ServerModule::OnSwapSceneResultEvent);
    }

    return 0;
}

int AFCGameServerNet_ServerModule::OnSwapSceneResultEvent(const AFGUID& self, const int nEventID, const AFIDataList& var)
{
    if(var.GetCount() != 7 ||
            !var.TypeEx(AF_DATA_TYPE::DT_OBJECT, AF_DATA_TYPE::DT_INT, AF_DATA_TYPE::DT_INT,
                        AF_DATA_TYPE::DT_INT, AF_DATA_TYPE::DT_FLOAT, AF_DATA_TYPE::DT_FLOAT, AF_DATA_TYPE::DT_FLOAT, AF_DATA_TYPE::DT_UNKNOWN)
      )
    {
        return 1;
    }

    AFGUID ident = var.Object(0);
    int nType = var.Int(1);
    int nTargetScene = var.Int(2);
    int nTargetGroupID = var.Int(3);
    Point3D xPos;
    xPos.x = var.Float(4);
    xPos.y = var.Float(5);
    xPos.z = var.Float(6);

    AFMsg::ReqAckSwapScene xSwapScene;
    xSwapScene.set_transfer_type(AFMsg::ReqAckSwapScene::EGameSwapType::ReqAckSwapScene_EGameSwapType_EGST_NARMAL);
    xSwapScene.set_scene_id(nTargetScene);
    xSwapScene.set_line_id(nTargetGroupID);
    xSwapScene.set_x(xPos.x);
    xSwapScene.set_y(xPos.y);
    xSwapScene.set_z(xPos.z);

    SendMsgPBToGate(AFMsg::EGMI_ACK_SWAP_SCENE, xSwapScene, self);

    return 0;
}

void AFCGameServerNet_ServerModule::OnReqiureRoleListProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    //fd
    AFGUID nGateClientID;
    AFMsg::ReqRoleList xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nGateClientID))
    {
        return;
    }

    AFMsg::AckRoleLiteInfoList xAckRoleLiteInfoList;
    m_pNetModule->SendMsgPB(AFMsg::EGMI_ACK_ROLE_LIST, xAckRoleLiteInfoList, xClientID, nGateClientID);
}

void AFCGameServerNet_ServerModule::OnCreateRoleGameProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nGateClientID;
    AFMsg::ReqCreateRole xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nGateClientID))
    {
        return;
    }

    AFMsg::AckRoleLiteInfoList xAckRoleLiteInfoList;
    AFMsg::RoleLiteInfo* pData = xAckRoleLiteInfoList.add_char_data();
    pData->mutable_id()->CopyFrom(AFINetServerModule::NFToPB(m_pUUIDModule->CreateGUID()));
    pData->set_career(xMsg.career());
    pData->set_sex(xMsg.sex());
    pData->set_race(xMsg.race());
    pData->set_noob_name(xMsg.noob_name());
    pData->set_game_id(xMsg.game_id());
    pData->set_role_level(1);
    pData->set_delete_time(0);
    pData->set_reg_time(0);
    pData->set_last_offline_time(0);
    pData->set_last_offline_ip(0);
    pData->set_view_record("");

    m_pNetModule->SendMsgPB(AFMsg::EGMI_ACK_ROLE_LIST, xAckRoleLiteInfoList, xClientID, nGateClientID);
}

void AFCGameServerNet_ServerModule::OnDeleteRoleGameProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    AFGUID nPlayerID;
    AFMsg::ReqDeleteRole xMsg;
    if(!m_pNetModule->ReceivePB(xHead, nMsgID, msg, nLen, xMsg, nPlayerID))
    {
        return;
    }


    AFMsg::AckRoleLiteInfoList xAckRoleLiteInfoList;
    m_pNetModule->SendMsgPB(AFMsg::EGMI_ACK_ROLE_LIST, xAckRoleLiteInfoList, xClientID, nPlayerID);
}

void AFCGameServerNet_ServerModule::OnClienSwapSceneProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    CLIENT_MSG_PROCESS(nMsgID, msg, nLen, AFMsg::ReqAckSwapScene);

    AFCDataList varEntry;
    varEntry << pObject->Self();
    varEntry << int32_t(0);
    varEntry << xMsg.scene_id();
    varEntry << int32_t(-1) ;
    m_pKernelModule->DoEvent(pObject->Self(), AFED_ON_CLIENT_ENTER_SCENE, varEntry);
}

void AFCGameServerNet_ServerModule::OnProxyServerRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        ARK_SHARE_PTR<GateServerInfo> pServerData = mProxyMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<GateServerInfo>(ARK_NEW GateServerInfo());
            mProxyMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xServerData.xClient = xClientID;
        *(pServerData->xServerData.pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "Proxy Registered");
    }

    return;
}

void AFCGameServerNet_ServerModule::OnProxyServerUnRegisteredProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        mProxyMap.RemoveElement(xData.server_id());


        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "Proxy UnRegistered");
    }

    return;
}

void AFCGameServerNet_ServerModule::OnRefreshProxyServerInfoProcess(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
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
        ARK_SHARE_PTR<GateServerInfo> pServerData = mProxyMap.GetElement(xData.server_id());
        if(nullptr == pServerData)
        {
            pServerData = ARK_SHARE_PTR<GateServerInfo>(ARK_NEW GateServerInfo());
            mProxyMap.AddElement(xData.server_id(), pServerData);
        }

        pServerData->xServerData.xClient = xClientID;
        *(pServerData->xServerData.pData) = xData;

        m_pLogModule->LogInfo(AFGUID(0, xData.server_id()), xData.server_name(), "Proxy Registered");
    }

    return;
}

void AFCGameServerNet_ServerModule::SendMsgPBToGate(const uint16_t nMsgID, google::protobuf::Message& xMsg, const AFGUID& self)
{
    ARK_SHARE_PTR<GateBaseInfo> pData = mRoleBaseData.GetElement(self);
    if(nullptr != pData)
    {
        ARK_SHARE_PTR<GateServerInfo> pProxyData = mProxyMap.GetElement(pData->nGateID);
        if(nullptr != pProxyData)
        {
            m_pNetModule->SendMsgPB(nMsgID, xMsg, pProxyData->xServerData.xClient, pData->xClientID);
        }
    }
}

void AFCGameServerNet_ServerModule::SendMsgPBToGate(const uint16_t nMsgID, const std::string& strMsg, const AFGUID& self)
{
    ARK_SHARE_PTR<GateBaseInfo> pData = mRoleBaseData.GetElement(self);
    if(nullptr != pData)
    {
        ARK_SHARE_PTR<GateServerInfo> pProxyData = mProxyMap.GetElement(pData->nGateID);
        if(nullptr != pProxyData)
        {
            m_pNetModule->SendMsgPB(nMsgID, strMsg, pProxyData->xServerData.xClient, pData->xClientID);
        }
    }
}

AFINetServerModule* AFCGameServerNet_ServerModule::GetNetModule()
{
    return m_pNetModule;
}

bool AFCGameServerNet_ServerModule::AddPlayerGateInfo(const AFGUID& nRoleID, const AFGUID& nClientID, const int nGateID)
{
    if(nGateID <= 0)
    {
        //�Ƿ�gate
        return false;
    }

    if(nClientID.IsNull())
    {
        return false;
    }

    ARK_SHARE_PTR<AFCGameServerNet_ServerModule::GateBaseInfo> pBaseData = mRoleBaseData.GetElement(nRoleID);
    if(nullptr != pBaseData)
    {
        // �Ѿ�����
        m_pLogModule->LogError(nClientID, "player is exist, cannot enter game", "", __FUNCTION__, __LINE__);
        return false;
    }

    ARK_SHARE_PTR<GateServerInfo> pServerData = mProxyMap.GetElement(nGateID);
    if(nullptr == pServerData)
    {
        return false;
    }

    if(!pServerData->xRoleInfo.insert(std::make_pair(nRoleID, pServerData->xServerData.xClient)).second)
    {
        return false;
    }

    if(!mRoleBaseData.AddElement(nRoleID, ARK_SHARE_PTR<GateBaseInfo>(ARK_NEW GateBaseInfo(nGateID, nClientID))))
    {
        pServerData->xRoleInfo.erase(nRoleID);
        return false;
    }

    return true;
}

bool AFCGameServerNet_ServerModule::RemovePlayerGateInfo(const AFGUID& nRoleID)
{
    ARK_SHARE_PTR<GateBaseInfo> pBaseData = mRoleBaseData.GetElement(nRoleID);
    if(nullptr == pBaseData)
    {
        return false;
    }

    mRoleBaseData.RemoveElement(nRoleID);

    ARK_SHARE_PTR<GateServerInfo> pServerData = mProxyMap.GetElement(pBaseData->nGateID);
    if(nullptr == pServerData)
    {
        return false;
    }

    pServerData->xRoleInfo.erase(nRoleID);
    return true;
}

ARK_SHARE_PTR<AFIGameServerNet_ServerModule::GateBaseInfo> AFCGameServerNet_ServerModule::GetPlayerGateInfo(const AFGUID& nRoleID)
{
    return mRoleBaseData.GetElement(nRoleID);
}

ARK_SHARE_PTR<AFIGameServerNet_ServerModule::GateServerInfo> AFCGameServerNet_ServerModule::GetGateServerInfo(const int nGateID)
{
    return mProxyMap.GetElement(nGateID);
}

ARK_SHARE_PTR<AFIGameServerNet_ServerModule::GateServerInfo> AFCGameServerNet_ServerModule::GetGateServerInfoByClientID(const AFGUID& nClientID)
{
    int nGateID = -1;
    ARK_SHARE_PTR<GateServerInfo> pServerData = mProxyMap.First();
    while(nullptr != pServerData)
    {
        if(nClientID == pServerData->xServerData.xClient)
        {
            nGateID = pServerData->xServerData.pData->server_id();
            break;
        }

        pServerData = mProxyMap.Next();
    }

    if(nGateID == -1)
    {
        return nullptr;
    }

    return pServerData;
}

void AFCGameServerNet_ServerModule::OnTransWorld(const AFIMsgHead& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
{
    std::string strMsg;
    AFGUID nPlayer;
    int nHasKey = 0;
    if(AFINetServerModule::ReceivePB(xHead, nMsgID, msg, nLen, strMsg, nPlayer))
    {
        nHasKey = nPlayer.nIdent;
    }

    m_pGameServerToWorldModule->SendBySuit(nHasKey, nMsgID, msg, nLen);
}

