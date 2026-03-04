#pragma once
#include <cstdint>

enum class MapleVersion
{
    v83,
};

enum class InventoryType
{
    Equip = 1,
    Use   = 2,
    Setup = 3,
    Etc   = 4,
    Cash  = 5
};





struct CMobOffsets
{
    // CMobPool base address
    uintptr_t CMobPool = 0x00BEBFA4;  
    uintptr_t CMobPoolList = 0x28;  


    // Position (raw int version)
    uintptr_t posX_raw     = 0x510;  // this + 324 * 4
    uintptr_t posY_raw     = 0x514;
    uintptr_t prevPosX_raw = 0x518;
    uintptr_t prevPosY_raw = 0x51C;

    uintptr_t HP = 0x520; //percentage


    // Body rects
    uintptr_t rcBody       = 0x40C;  // this + 259*4
    uintptr_t rcBodyFlip   = 0x41C;  // this + 263*4


    //template
    uintptr_t CMobTemplate = 0x188;
    uintptr_t TemplateID    = 0x0C;
    uintptr_t TemplateID_CS = 0x14;

    uintptr_t TemplateMaxHP    = 0x7C;
    uintptr_t TemplateMaxHP_CS = 0x84;


    uintptr_t TemplatesName = 0x30;



    //vec ctrl
    // trace CVecCtrlMob::CreateInstance();
    uintptr_t p_CVecCtrlInterface = 0x11C;  // m_pvc+0x4
    uintptr_t InterfaceToCVecCtrl = 0xC;
    uintptr_t CVecCtrl_pFoothold = 0x110;


    CMobOffsets() = default;
};

struct CLadderRopeOffsets
{
    uintptr_t id = 0x0;
    uintptr_t ladder  = 0x4;
    uintptr_t upperFH = 0x8;
    uintptr_t x       = 0xC;
    uintptr_t y1      = 0x10;
    uintptr_t y2      = 0x14;
    uintptr_t page    = 0x18; 
    uintptr_t stride  = 0x1C;

};

struct CStaticFootholdOffsets
{
    uintptr_t x1 = 0xC;
    uintptr_t y1 = 0x10;
    uintptr_t x2 = 0x14;
    uintptr_t y2 = 0x18;
    uintptr_t layer = 0x20;  //seems like lZmass is the correct name, but layer is also good name
    uintptr_t uvx = 0x30; //slope_x
    uintptr_t uvy = 0x38; //slope_y
    uintptr_t id = 0x48;
    uintptr_t pr = 0x4C;
    uintptr_t ne = 0x50;


    //TSecType<double>::GetData(&pfh->m_pAttrFoothold.p->walk);
    uintptr_t m_pAttrFoothold = 0x28;
    uintptr_t WalkSpeed  = 0xC;
    uintptr_t Force  = 0x24;


    CStaticFootholdOffsets() = default;
};


struct CWvsPhysicalSpace2DOffsets
{
    uintptr_t CWvsPhysicalSpace2D = 0xBEBFA0;
    uintptr_t FootholdList = 0x88;
    uintptr_t LadderRopeArray = 0xA8;

	uintptr_t Left = 0x24;
	uintptr_t Right = 0x2C;
	uintptr_t Top = 0x28;
	uintptr_t Bottom = 0x30;

    uintptr_t BaseLayer = 0x40; //m_nBaseZmass, named layer by me. 

    // physics constants
    uintptr_t m_constants = 0x8;

    uintptr_t WalkForce  = 0x00;
    uintptr_t WalkSpeed  = 0x08;
    uintptr_t FloatDrag1 = 0x28;
    uintptr_t FloatDrag2 = 0x30;
    uintptr_t FloatCoeff = 0x38;
    uintptr_t GravityAcc = 0x60;
    uintptr_t FallSpeed  = 0x68;
    uintptr_t JumpSpeed  = 0x70;


    // CAttrField
    uintptr_t m_pAttrField = 0xAC;
    uintptr_t drag = 0xC;
    uintptr_t gravity = 0x24; //g

    CWvsPhysicalSpace2DOffsets() = default;
};

struct CUserLocalOffsets
{

	uintptr_t CUserLocal = 0xBEBF98; // CUserLocal
	uintptr_t UID = 0x11A8;
	uintptr_t foothold = 0x1F0;
	uintptr_t x = 0x116C;
	uintptr_t y = 0x1170;
	uintptr_t attackCount = 0x2B88;
	uintptr_t breath = 0x56C;
	uintptr_t charAnimation = 0x570;
	uintptr_t comboCount = 0x3220;
	uintptr_t faceDir = 0x1180;


    uintptr_t m_pAttrShoe = 0x2B64;   //m_pAttrShoe     ZRef<CAttrShoe>.p = (2B60+4) 
    uintptr_t AttrShoe_WalkSpeed = 0x24;
    uintptr_t AttrShoe_WalkJump = 0x48;
    uintptr_t AttrShoe_Mass = 0xC;

// 00000000 CAttrShoe       struc ; (sizeof=0x90, align=0x4, copyof_2255)
// 00000000 baseclass_0     ZRefCounted ?
// 0000000C mass            TSecType<double> ?
// 00000018 walkAcc         TSecType<double> ?
// 00000024 walkSpeed       TSecType<double> ?
// 00000030 walkDrag        TSecType<double> ?
// 0000003C walkSlant       TSecType<double> ?
// 00000048 walkJump        TSecType<double> ?
// 00000054 swimAcc         TSecType<double> ?
// 00000060 swimSpeedH      TSecType<double> ?
// 0000006C swimSpeedV      TSecType<double> ?
// 00000078 flyAcc          TSecType<double> ?
// 00000084 flySpeed        TSecType<double> ?
// 00000090 CAttrShoe       ends
// 00000090


    //CVecCtrl
    // m_pInterface = this->m_pvc.m_pInterface;
    // v4 = (CVecCtrl *)&m_pInterface[-3]; 0xC
    uintptr_t p_CVecCtrlInterface = 0x11A4;  // m_pvc
    uintptr_t InterfaceToCVecCtrl = 0xC;

    uintptr_t CVecCtrl_X = 0x20;  
    uintptr_t CVecCtrl_X_CS = 0x30; 

    uintptr_t CVecCtrl_Y = 0x38;   
    uintptr_t CVecCtrl_Y_CS = 0x48; 

    uintptr_t CVecCtrl_Vx = 0x50;   //abs_pos->Vx
    uintptr_t CVecCtrl_Vx_CS = 0x60; 

    uintptr_t CVecCtrl_Vy = 0x68;   //abs_pos->Vy
    uintptr_t CVecCtrl_Vy_CS = 0x78; 
    
    uintptr_t CVecCtrl_pLadderOrRope = 0x124;
    uintptr_t CVecCtrl_pLadderOrRope_CS = 0x12C;

    //direct read
    uintptr_t CVecCtrl_pFoothold = 0x110;
    uintptr_t CVecCtrl_layer = 0x134;


    //Fuse long 
    uintptr_t CVecCtrl_InputX = 0x150;
    uintptr_t CVecCtrl_InputX_CS = 0x158;




    CUserLocalOffsets() = default;
};

struct CWvsContextOffsets
{
    uintptr_t CWvsContext = 0xBE7918;
	uintptr_t Channel = 0x2058;
	uintptr_t ZArray_Channel = 0x36D4;

    
    //
    uintptr_t MaxHP = 0x211C;
    uintptr_t MaxHP_CS = 0x2124;
    uintptr_t MaxMP = 0x2128;
    uintptr_t MaxMP_CS = 0x2130;


    // CharacterData
    uintptr_t CharacterData = 0x20B8;
    uintptr_t ZArray_ZRef_GW_ItemSlotBase = 0x44B;

    //stats
    uintptr_t BasicStat     = 0x20BC; 
    uintptr_t STR = ( 9)*0x4;
    uintptr_t DEX = (12)*0x4;
    uintptr_t INT = (15)*0x4;
    uintptr_t LUK = (18)*0x4;

    //
    uintptr_t SecondaryStat = 0x2134; 
    uintptr_t attack    = 0x4*0*12;
    uintptr_t defense   = 0x4*1*12;
    uintptr_t magic     = 0x4*2*12;
    uintptr_t magic_def = 0x4*3*12;
    uintptr_t accuracy  = 0x4*4*12;
    uintptr_t avoid     = 0x4*5*12;
    uintptr_t hands     = 0x4*6*12;
    uintptr_t speed     = 0x4*7*12;
    uintptr_t jump      = 0x4*8*12;
    // Disease rReason offsets (relative to SecondaryStat)
    uintptr_t SEAL     = 0x0330;
    uintptr_t DARKNESS = 0x0354;
    uintptr_t STUN     = 0x02E8;
    uintptr_t POISON   = 0x030C;
    uintptr_t WEAKNESS = 0x04C8;
    uintptr_t CURSE    = 0x04EC;
    uintptr_t SLOW     = 0x0510;
    uintptr_t SEDUCE   = 0x063C;
    uintptr_t ZOMBIFY  = 0x0744;
    uintptr_t CONFUSE  = 0x07E0;



    //party
    uintptr_t m_party = 0x2ED8;


    struct UIOffsets
    {
        uint32_t m_pUIItem       = 0x35B8;  // this[3438]
        uint32_t m_pUIEquip      = 0x35C0;  // this[3440]
        uint32_t m_pUIStat       = 0x35C8;  // this[3442]
        uint32_t m_pUISkill      = 0x35D0;  // this[3444]
        uint32_t m_pUIKeyConfig  = 0x35E0;  // this[3448]
        uint32_t m_pUIQuestInfo  = 0x35F0;  // this[3452]
        uint32_t m_pUIUserInfo   = 0x3600;  // this[3456]
        uint32_t m_pUIGuildBBS   = 0x3610;  // this[3460]
        uint32_t m_pUIEnergyBar  = 0x3628;  // this[3466]
        uint32_t m_pUIItemMaker  = 0x3648;  // this[3474]
        uint32_t m_pUIFamily     = 0x3658;  // this[3478]
        uint32_t m_pUIFamilyChart = 0x3660; // this[3480]
    } UI;


    //Equip Stats
};


struct GW_ItemSlotOffsets {

    //GW_ItemSlot
    uintptr_t GW_ItemSlot_ID = 0xC;
    uintptr_t GW_ItemSlot_Qty = 0x28;
    uintptr_t GW_ItemSlot_Qty_CS = 0x2C;

    uintptr_t  Equip_STR          = 0x4*13; 
    uintptr_t  Equip_DEX          = 0x4*15; 
    uintptr_t  Equip_INT          = 0x4*17; 
    uintptr_t  Equip_LUK          = 0x4*19; 
    uintptr_t  Equip_MaxHP        = 0x4*21; 
    uintptr_t  Equip_MaxMP        = 0x4*23; 
    uintptr_t  Equip_WeaponAttack = 0x4*25; 
    uintptr_t  Equip_MagicAttack  = 0x4*27;
    uintptr_t  Equip_WeaponDef    = 0x4*29;
    uintptr_t  Equip_MagicDef     = 0x4*31;
    uintptr_t  Equip_Accuracy     = 0x4*33;
    uintptr_t  Equip_Avoid        = 0x4*35;
    uintptr_t  Equip_Hands        = 0x4*37;
    uintptr_t  Equip_Speed        = 0x4*39;
    uintptr_t  Equip_Jump         = 0x4*41;
    //checksum offsets 
    uintptr_t  Equip_Stat_CS_Offsets = 0x4;

    //Upgrade Avail: uchar
    uintptr_t  Equip_UpgradeAvail = 0x28; 
    uintptr_t  Equip_Upgrade_CS_Offsets = 0x2;
    

    // Might use checksum wrapper for some fields
};


struct CharacterStatOffsets
{

    uintptr_t CharacterStat = 0xBF3CD8; 

    uintptr_t Level = 0x33;
    uintptr_t Level_CS = 0x35;

    uintptr_t Job = 0x39;
    uintptr_t Job_CS = 0x3D;

    uintptr_t HP = 0x61;
    uintptr_t HP_CS = 0x65;

    uintptr_t MP = 0x71;
    uintptr_t MP_CS = 0x75;

    uintptr_t EXP = 0x91;
    uintptr_t EXP_CS = 0x99;
    

    uintptr_t Mesos = 0xA5;        // ZtlSecureFuse<long> — meso count
    uintptr_t Mesos_CS = 0xAD;     // Mesos + 0x8

    uintptr_t EXP_Table = 0xBEF230; //static;
};

struct CDropOffsets
{
    uintptr_t CDropPool = 0x00BED6AC;  
    uintptr_t CDropPoolList = 0x2C;  

    //UID
    uintptr_t UID     = 0x20;  
    uintptr_t OwnerID = 0x24;    // who owns/can pick
    uintptr_t SourceID = 0x28;   //from what
    uintptr_t OwnType = 0x2C;    
    uintptr_t IsMeso  = 0x30;  
    uintptr_t ID      = 0x34;  //Item ID or mesos

    uintptr_t Gr2DLayer = 0x38; //IWzGr2DLayer
    uintptr_t x = 0x5C;
    uintptr_t y = 0x60;




    CDropOffsets() = default;
};



struct CPortalOffsets
{

    uintptr_t CPortalList = 0xBED768;  //static 
    uintptr_t PortalArray = 0x4; //ZArray<ZRef<PORTAL> >

    //CPortal
	uintptr_t ID = 0x0;
	uintptr_t Name = 0x4;
	uintptr_t Type = 0x8;
	uintptr_t x = 0xC;
	uintptr_t y = 0x10;
	uintptr_t toMapID = 0x1C;
	uintptr_t toName = 0x20;


    CPortalOffsets() = default;
};


struct CItemInfoOffsets
{
    uintptr_t CItemInfo =0xBE78D8;
    uintptr_t m_mItemString = 0x54;
    uintptr_t m_mMapString = 0x6C;

};


struct CUIMiniMapOffsets
{
    uintptr_t CUIMiniMap = 0xBED788;
    uintptr_t Width = 0x5C0;
    uintptr_t Height = 0x5C4;
    uintptr_t CenterX = 0x5C8;
    uintptr_t CenterY = 0x5CC;
    uintptr_t MapID = 0x668;


};


struct CUserOffsets
{
    uintptr_t CUserPool = 0xBEBFA8;
    uintptr_t CUserPoolList = 0x30;
    //p->pUserRemote.p
    uintptr_t CUserRemoteEntryOffset = 0x10; //USERREMOTE_ENTRY->0x10 = p_CuserRemote
    uintptr_t Job = 0x37DC;
    uintptr_t x = 0x1174; //inside CUser::GetPos(); +0x4 from the vec object
    uintptr_t y = 0x1178; //
    uintptr_t UID = 0x11A8;
    uintptr_t name = 0x11AC;


};

struct CNpcOffsets
{
    // CNpcPool base address
    uintptr_t CNpcPool     = 0x00BED780;
    uintptr_t CNpcPoolList = 0x28;

    // Position (raw int)
    uintptr_t posX     = 0x150;
    uintptr_t posY     = 0x154;
    uintptr_t prevPosX = 0x158;
    uintptr_t prevPosY = 0x15C;

    // Template
    uintptr_t CNpcTemplate = 0xA8;
    uintptr_t TemplateID   = 0x00;
    uintptr_t TemplateName = 0x04;

    CNpcOffsets() = default;
};


struct CInputSystemOffsets
{
    uintptr_t CInputSystem       = 0xBEC33C; // global ptr to CInputSystem*
    uintptr_t m_bAcquireKeyboard = 0x20;     // [ecx+20h]
    uintptr_t m_aKeyState        = 0x824;    // [ecx+824h + vk] (BYTE[256])


    uintptr_t p_MouseLocation = 0x978;
    uintptr_t MouseX = 0x8C;
    uintptr_t MouseY = 0x90;

};


struct CTemporaryStatViewOffsets
{

    uintptr_t m_tempStatView = 0x2EA8;   //offset on CWvsContext, static 
    uintptr_t m_temporaryStatList = 0x4+0xC; //m_tempStatView->m_lTemporaryStatList.head;  ZList<ZRef<CTemporaryStatView::TEMPORARY_STAT>>

    uintptr_t nType    = 0x1C ;
    uintptr_t nID      = 0x20 ;
    uintptr_t nSubID   = 0x24 ;
    uintptr_t sToolTip = 0x28 ;
    uintptr_t tLeft = 0x38;
};


struct CFuncKeyMappedOffsets
{
    uintptr_t CFuncKeyMappedMan = 0xBED5A0;
    uintptr_t KeyUIPosX = 0x00BE27E0;  // dword_BE27E0
    uintptr_t KeyUIPosY = 0x00BE27E4;  // dword_BE27E4
};


struct CShopDlgOffsets
{
    //  check CShopDlg Memory Layout & Hierarchy in CShopDlg
    // --- Singleton & VTable ---
    uintptr_t Instance              = 0x00BE7910;   // TSingleton<CUniqueModeless>::ms_pInstance
    uintptr_t VTable                = 0x00AFE110;   // CShopDlg vtable address
    
    // --- CWndMan (for origin calculation) ---
    uintptr_t CWndMan_Instance      = 0x00BEC20C;   // TSingleton<CWndMan>::ms_pInstance
    uint32_t  CWndMan_pOrgWindow    = 0xDC;         // IWzVector2D* origin window
    
    // --- CWnd member offsets (base class of CShopDlg) ---
    uint32_t CWnd_pLayer            = 0x18;         // IWzGr2DLayer* m_pLayer
    uint32_t CWnd_width             = 0x24;         // m_width
    uint32_t CWnd_height            = 0x28;         // m_height
    
    // --- CShopDlg member offsets ---
    uint32_t pBtnExit               = 0x84;         // CCtrlButton* m_pBtnExit
    uint32_t pBtnBuy                = 0x8C;         // CCtrlButton* m_pBtnBuy    
    uint32_t pBtnSell               = 0x94;         // CCtrlButton* m_pBtnSell
    uint32_t pTabBuy                = 0x9C;         // CCtrlTab* m_pTabBuy
    uint32_t pTabSell               = 0xA4;         // CCtrlTab* m_pTabSell
    uint32_t npcTemplateId          = 0xBC;         // int32 m_npcTemplateId
    uint32_t aShopItem              = 0xC4;         // ZArray<ITEM> m_aBuyItem

    // --- CCtrlTab
    uint32_t m_nCurTab              = 0x3C;
    
    //--- CShopDlg selection
    // check  CShopDlg::SendBuyRequest
    uint32_t nBuySelected           = 0xF0;
    uint32_t nSellSelected          = 0xF4;


    // --- SHOPITEM structure offsets ---
    uint32_t ShopItem_nItemId       = 0x00;         // TSecType<long> (12 bytes)
    uint32_t ShopItem_nPos          = 0x0C;         // int32
    uint32_t ShopItem_sItemName     = 0x10;         // Ztl_bstr_t*
    uint32_t ShopItem_nPrice        = 0x18;         // int32
    uint32_t ShopItem_Size          = 0x48;         // 72 bytes per item
    
    // --- CCtrlWnd structure offsets ---
    uint32_t CCtrlWnd_nCtrlId       = 0x14;         // int32 m_nCtrlId
    uint32_t CCtrlWnd_pLTCtrl       = 0x18;         // IWzVector2D* m_pLTCtrl
    uint32_t CCtrlWnd_width         = 0x1C;         // int32 m_width
    uint32_t CCtrlWnd_height        = 0x20;         // int32 m_height
    
    // --- IWzVector2D structure offsets (SHAPE2D.DLL) ---
    uint32_t IWzVector2D_x_simple   = 0x1C;         // x when flag=0
    uint32_t IWzVector2D_y_simple   = 0x20;         // y when flag=0
    uint32_t IWzVector2D_flag       = 0x24;         // parent pointer (0 = simple mode)
    uint32_t IWzVector2D_x_rel      = 0x68;         // x when flag!=0 (read from flag+offset)
    uint32_t IWzVector2D_y_rel      = 0x6C;         // y when flag!=0 (read from flag+offset)
    
    // --- IWzGr2DLayer structure offsets (found by memory scanning) ---
    uint32_t IWzGr2DLayer_x         = 0x5C;         // x coordinate
    uint32_t IWzGr2DLayer_y         = 0x60;         // y coordinate




    // m_aSellItem (ZArray<SELLITEM>)
    // CShopDlg offsets
    uintptr_t aSellItem       = 0xD4;   // this[53] = 53 * 4 = 0xD4
    uintptr_t bShopRequestSent = 0xFC;  // this[63] = 63 * 4 = 0xFC

    // SELLITEM structure (0x48 = 72 bytes)
    uintptr_t SellItem_nItemID  = 0x00;  // TSecType<long> (8 bytes)
    uintptr_t SellItem_nPOS     = 0x0C;  // int32 - inventory slot
    uintptr_t SellItem_strName  = 0x10;  // _bstr_t (guess, between nPOS and nNumber)
    uintptr_t SellItem_nPrice   = 0x18;  // int32 (guess)
    uintptr_t SellItem_nNumber  = 0x34;  // int32 - quantity (confirmed: v5 + 52)
    uintptr_t SellItem_pItem    = 0x40;  // ZRef<GW_ItemSlotBase> (confirmed: v5 + 64)
    uintptr_t SellItem_Size     = 0x48;  // 72 bytes (confirmed)
};


struct CWndManOffsets
{
    // --- Singleton ---
    uintptr_t Instance              = 0x00BEC20C;   // TSingleton<CWndMan>::ms_pInstance
    
    // --- CWndMan member offsets (old version) ---
    uint32_t m_pActiveWnd           = 0x80;         // CWnd* - active window/dialog
    uint32_t m_pFocus               = 0x88;         // IUIMsgHandler* - focused handler
    
    // --- Dialog vtables for identification ---
    uintptr_t CUtilDlg_VTable       = 0x00B3BB24;   // CUtilDlg::vftable
    uintptr_t CUtilDlgEx_VTable     = 0x00B3E100;   // CUtilDlgEx::vftable

    // -- commom Vtables
    uintptr_t CWnd_VTable = 0x00B3F13C;

    uintptr_t CUIEquip_VTable = 0x00B389E8;
    uintptr_t CUIItem_VTable  = 0x00B39108;
    uintptr_t CUIStat_VTable  = 0x00B3B908;
    uintptr_t CUISkill_VTable = 0x00B3B5CC;
    uintptr_t CUIQuest_VTable = 0x00B3A9EC;
    uintptr_t CUIParty_VTable = 0x00B3C35C;
    uintptr_t CUIFamily_VTable = 0x00B38CA8;
    uintptr_t CUIGameMenu_VTable = 0x00B39B60;
    uintptr_t CUIChannelShift_VTable = 0x00B3D9E0;

    struct CUtilDlgExOffsets
    {
        uint32_t m_aCT_a = 0x6C4;      // this + 433*4 = pointer to CT_INFO array
        uint32_t CT_INFO_size = 68;    // sizeof(CT_INFO)
        uint32_t CT_INFO_nType = 0x00; // int - 0=text, 4=list item
        uint32_t CT_INFO_sText = 0x10; // ZXString<char> - text string ptr
        uint32_t CT_INFO_nSelect = 0x28;

        uint32_t m_nSelect = 0xF4;      // current selection
        uint32_t m_nSelectPrev = 0xF8;
    } CUtilDlgEx;

    struct CUtilDlgOffsets
    {
        uint32_t m_asText_a = 0xD0;  // ZArray<ZXString<char>>
    } CUtilDlg;


    struct CUIStatOffsets
    {
        // Button pointer arrays (ZRef = 8 bytes each, pointer at +4)
        uint32_t m_pBtApUp = 0x5B0;       // array[6] - STR, DEX, INT, LUK, HP, MP
        uint32_t m_pBtAutoApUp = 0x5E0;   // array[3] - Auto buttons
    } CUIStat;

    struct CUISkillOffsets
    {
        uint32_t m_pTab             = 0x5B8;   // ZRef<CCtrlTab> - tab control
        uint32_t m_pSBSkill         = 0x5C0;   // ZRef<CCtrlScrollBar> - scroll bar
        uint32_t m_spBtUp           = 0x5C4;   // ZRef<CCtrlButton>[4] - skill up buttons
        uint32_t m_SkillRootVisible = 0x5EC;   // embedded SKILLROOT (cached visible skills)
    } CUISkill;

    struct CCtrlScrollBarOffsets
    {
        uint32_t m_nCurPos = 0x38;   // current scroll position
    } CCtrlScrollBar;

    struct CCtrlTabOffsets
    {
        uint32_t m_nCurTab = 0x3C;   // current tab index
    } CCtrlTab;

    struct SKILLROOTOffsets
    {
        uint32_t nSkillRootID = 0x00;
        uint32_t sBookName    = 0x04;   // ZXString<char>
        uint32_t aSkill       = 0x08;   // ZArray<ZRef<SKILLENTRY>>
    } SKILLROOT;

    // check CUIRevive::Update
    // CWvsContext::UI_CloseRevive
    struct CUIReviveOffsets
    {
        uintptr_t Instance = 0x00BF1028;
        uintptr_t m_tWaitRevive = 0x88;
    } CUIRevive;

};



struct CUIStatusBarOffsets
{
    // Singleton instance
    uintptr_t Instance       = 0x00BEC208;
    
    // Static ZArray<ZRef<CChatLog>> - contains pointer to array data
    uintptr_t m_aChatLog     = 0x00BF1100;

    uintptr_t m_dEXPNo =  0xBC8;  //double precision, exp precentage.

    uintptr_t m_nChatType = 0xBAC;
    // 0 To a buddy  // 1 To Group  // 2 To the party // 3 To the Guild // 4 To Alliance // 5 To Spouse // 6 To Whisper// 7 To All 


    uintptr_t m_pEditChatInput = 0xB18; //messagehandler at +4
    
    // CChatLog structure offsets
    struct CChatLogOffsets
    {
        uintptr_t m_sChat           = 0x0C;  // ZXString<wchar_t> (pointer to string)
        uintptr_t m_nType           = 0x10;  // ZtlSecureTear encrypted value (8 bytes)
        uintptr_t m_nType_CS        = 0x18;  // ZtlSecureTear checksum
        uintptr_t m_nBack           = 0x1C;  // Background color
        uintptr_t m_nChannelID      = 0x20;  // Channel ID (-1 = none)
        uintptr_t m_bWhisperIcon    = 0x24;  // Whisper icon flag
        uintptr_t m_bFirstLine      = 0x28;  // First line flag
        uintptr_t m_pItem           = 0x2C;  // ZRef<GW_ItemSlotBase>
    } CChatLog;
};


struct CPetOffsets
{
    // CUser::m_apPet - ZArray<ZRef<CPet>>, max 3 pets
    // From IDA: mov eax, [edi+1EDCh] in SetActivePet
    // ZRef<CPet> stride = 8 bytes (refCount + p)
    uintptr_t m_apPet = 0x1EDC;          // offset from CUser/CUserLocal to m_apPet.a  — used in CUserLocal::GetActivePets

    // CPet member offsets (from IDA: *(this + N) where N*4 = byte offset)
    uintptr_t m_pOwner       = 0x94;      // CUser* owner                               — NOT used in any method yet
    uintptr_t m_sName        = 0x98;      // ZXString<char> pet name                     — used in CPet::GetName
    uintptr_t m_nPetIndex    = 0x9C;      // slot index (0/1/2), mov [ecx+9Ch], eax      — used in CPet::GetPetIndex
    uintptr_t m_nTameness    = 0xA8;      // *(this+42) = closeness (decoded)            — used in CPet::GetTameness
    uintptr_t m_nRepleteness = 0xAC;      // *(this+43) = fullness (decoded)             — used in CPet::GetRepleteness
    uintptr_t m_nPetAttribute= 0xB0;      // *(this+44) = pet attribute flags (decoded)  — used in CPet::GetPetAttribute
};

struct CMapleTimeOffsets
{
    uintptr_t Base      = 0xBE7B38;   // MapleTimeBase global pointer
    uintptr_t TimeStamp = 0x18;       // offset to 4-byte tick timestamp
};

struct CNetworkOffsets
{
    uintptr_t CClientSocket  = 0x00BE7914;   // TSingleton<CClientSocket>::ms_pInstance
    uintptr_t SendPacket     = 0x0049637B;   // CClientSocket::SendPacket
    uintptr_t ProcessPacket  = 0x004965F1;   // CClientSocket::ProcessPacket (recv handler)
};


struct MapleOffsets
{
    MapleVersion Version;
    CMobOffsets  CMob;
    CStaticFootholdOffsets Foothold;
    CLadderRopeOffsets LadderRope;
    CWvsPhysicalSpace2DOffsets CWvsPhysicalSpace2D;
    CUserLocalOffsets CUserLocal;
    GW_ItemSlotOffsets GW_ItemSlot;
    CharacterStatOffsets CharacterStat;
    CWvsContextOffsets CWvsContext;
    CDropOffsets CDrop;
    CPortalOffsets CPortal;
    CItemInfoOffsets CItemInfo;
    CUIMiniMapOffsets CUIMiniMap;
    CUserOffsets CUser;
    CInputSystemOffsets CInputSystem;
    CTemporaryStatViewOffsets CTemporaryStatView;
    CShopDlgOffsets CShopDlg;
    CFuncKeyMappedOffsets CFuncKeyMapped;
    CWndManOffsets CWndMan;
    CUIStatusBarOffsets CUIStatusBar;
    CNpcOffsets CNpc;
    CPetOffsets CPet;
    CMapleTimeOffsets MapleTime;
    CNetworkOffsets CNetwork;

};








extern MapleOffsets g_Maple;
