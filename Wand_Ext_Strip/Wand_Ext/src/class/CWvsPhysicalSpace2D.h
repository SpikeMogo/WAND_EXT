
#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>
#include "MemoryUtil.h"
#include "MapleOffsets.h"
#include <class/CStaticFoothold.h>
#include <class/CLadderRope.h>
#include <class/CPortal.h>
#include <class/ZRef.h>
#include <string>
#include <class/CItemInfo.h>
#include <navigator/Map.h>




class CWvsPhysicalSpace2D
{
public:


    static size_t ForEachFoothold(
        HANDLE hProcess,
        const std::function<bool(const CStaticFoothold&)>& callback)
        {
        if (!hProcess)
            return 0;

        // Get CWvsPhysicalSpace2D* from global
        uintptr_t spaceThis = 0;
        if (!MemUtil::ReadPtr32(
                hProcess,
                g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D, // <- adjust name if needed
                spaceThis) || spaceThis == 0)
        {
            return 0;
        }

        // Get ZList<ZRef<CStaticFoothold>> head at offset 0x7C
        uintptr_t pos = 0;
        if (!MemUtil::ReadPtr32(
                hProcess,
                spaceThis + g_Maple.CWvsPhysicalSpace2D.FootholdList, // should be 0x7C
                pos) || pos == 0)
        {
            return 0;
        }

        size_t count = 0;

        while (pos != 0)
        {
            uintptr_t fhAddr = 0;
            if (!MemUtil::ZListGetNext32(hProcess, pos, fhAddr))
                break;

            if (fhAddr != 0)
            {
                CStaticFoothold fh(fhAddr);
                ++count;
                if (!callback(fh))
                    break;
            }
        }

        return count;
    }


    static size_t ForEachLadderRope(
    HANDLE hProcess,
    const std::function<bool(const CLadderRope&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t spaceBase = 0;
        if (!MemUtil::ReadPtr32(hProcess, g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D, spaceBase) || !spaceBase)
            return 0;

        uintptr_t lrArrayPtr = 0;
        if (!MemUtil::ReadPtr32(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.LadderRopeArray, lrArrayPtr) || !lrArrayPtr)
            return 0;

        int count = 0;
        if (!MemUtil::RPM<int>(hProcess, lrArrayPtr - 4, count) || count <= 0)
            return 0;

        size_t numVisited = 0;
        for (int i = 1; i <= count; ++i)
        {
            uintptr_t elemAddr = lrArrayPtr + i * g_Maple.LadderRope.stride;
            CLadderRope lr(elemAddr);
            ++numVisited;

            long id = 0;
            // If ID doesn’t match index, stop — memory no longer valid or switched map.
            if (!lr.GetId(hProcess, id) || id != i)
                break;

            if (!callback(lr))
                break;
        }

        return numVisited;
    }




    //                    arrayPtr
    //                       │
    //                       ▼
    // +--------+--------+--------+--------+--------+--------+
    // | Count  |  Ref   | Portal |  Ref   | Portal |  Ref   |
    // |  (int) | Count  | Ptr    | Count  | Ptr    | Count  |
    // |  -4    | +0     | +4     | +8     | +12    | +16    |
    // +--------+--------+--------+--------+--------+--------+


    static size_t ForEachPortal(
    HANDLE hProcess,
    const std::function<bool(const CPortal&)>& callback)
    {
        if (!hProcess)
            return 0;

        uintptr_t portalListBase = 0;
        if (!MemUtil::RPM(hProcess, g_Maple.CPortal.CPortalList, portalListBase) || !portalListBase)
            return 0;

        uintptr_t arrayPtr = 0;
        if (!MemUtil::RPM(hProcess, portalListBase + g_Maple.CPortal.PortalArray, arrayPtr) || !arrayPtr)
            return 0;

        int count = 0;
        if (!MemUtil::RPM<int>(hProcess, arrayPtr - 4, count) || count <= 0)
            return 0;

        size_t numVisited = 0;

        for (int i = 0; i < count; ++i)
        {
            ZRef zref{};
            if (!MemUtil::RPM(hProcess,
                            arrayPtr + i * sizeof(ZRef),
                            zref))
                continue;

            if (!zref.p)
                continue;

            CPortal portal(zref.p);
            ++numVisited;

            long nType = 0;
            uint32_t toMap = 999999999;

            // printf("portal %i\n",i);

            if (!portal.GetType(hProcess, nType) || nType == 0)
                continue;

            if (!portal.GetTopMap(hProcess, toMap))
                continue;

            if (!callback(portal))
                break;

            // printf("portal %i %i\n",nType, toMap);
            
        }

        return numVisited;
    }





    static bool GetMapID(
        HANDLE hProcess,
        uint32_t& mapID)
    {
        
        mapID = 0;

        if (!hProcess)
            return false;

        // 1) Get CWvsPhysicalSpace2D* from global
        uintptr_t spaceBase = 0;
        if (!MemUtil::RPM<uintptr_t>(
                hProcess,
                g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D,
                spaceBase) || !spaceBase)
        {
            return false;
        }

        uint32_t id = 0;
        uintptr_t miniMapBase = 0;
        if (!MemUtil::RPM<uintptr_t>(hProcess,g_Maple.CUIMiniMap.CUIMiniMap,miniMapBase) || !miniMapBase) return false;
        if (!MemUtil::RPM<uint32_t>(hProcess, miniMapBase  + g_Maple.CUIMiniMap.MapID,  id)) return false;
        mapID = id;
        return true;
    }


    static bool GetXBounds(HANDLE hProcess, int& l, int& r)
    {

        if (!hProcess)
            return false;

        // 1) Get CWvsPhysicalSpace2D* from global
        uintptr_t spaceBase = 0;
        if (!MemUtil::RPM<uintptr_t>(
                hProcess,
                g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D,
                spaceBase) || !spaceBase)
        {
            return false;
        }

        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.Left,   l)) return false;
        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.Right,  r)) return false;

        return true;

    }



    static bool GetMap(
        HANDLE hProcess,
        long& left,
        long& right,
        long& top,
        long& bottom,
        uint32_t& mapID,
        std::string &streetName,
        std::string &mapName)
    {
        left = right = top = bottom = 0;
        mapID = 0;
        streetName = "unKnown";
        mapName = "unKnown";


        if (!hProcess)
            return false;

        // 1) Get CWvsPhysicalSpace2D* from global
        uintptr_t spaceBase = 0;
        if (!MemUtil::RPM<uintptr_t>(
                hProcess,
                g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D,
                spaceBase) || !spaceBase)
        {
            return false;
        }

        // 2) Read the four wall values
        int l = 0, r = 0, t = 0, b = 0;
        uint32_t id = 0;

        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.Left,   l)) return false;
        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.Right,  r)) return false;
        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.Top,    t)) return false;
        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.Bottom, b)) return false;


        uintptr_t miniMapBase = 0;
        if (!MemUtil::RPM<uintptr_t>(hProcess,g_Maple.CUIMiniMap.CUIMiniMap,miniMapBase) || !miniMapBase) return false;
        if (!MemUtil::RPM<uint32_t>(hProcess, miniMapBase  + g_Maple.CUIMiniMap.MapID,  id)) return false;


        left   = l;
        right  = r;
        top    = t;
        bottom = b;
        mapID = id;

        int w = 0, h = 0;

        if (MemUtil::RPM<int>(hProcess, miniMapBase + g_Maple.CUIMiniMap.Width,   w) &&
            MemUtil::RPM<int>(hProcess, miniMapBase + g_Maple.CUIMiniMap.Height,  h)) 
        {
            if ((bottom-top)>h*2) top = bottom-h;
        }



        CItemInfo::GetMapStringByIdAndKey(hProcess,id,"streetName",streetName);
        CItemInfo::GetMapStringByIdAndKey(hProcess,id,"mapName",mapName);


        return true;
    }


    static bool GetMiniMap(
        HANDLE hProcess,
        long& Width,
        long& Height)
    {
        Width = Height = 0;

        if (!hProcess)
            return false;

        // 1) Get CWvsPhysicalSpace2D* from global
        uintptr_t miniMapBase = 0;
        if (!MemUtil::RPM<uintptr_t>(
                hProcess,
                g_Maple.CUIMiniMap.CUIMiniMap,
                miniMapBase) || !miniMapBase)
        {
            return false;
        }

        // 2) Read the four wall values
        int w = 0, h = 0;
        // uint32_t id = 0;

        if (!MemUtil::RPM<int>(hProcess, miniMapBase + g_Maple.CUIMiniMap.Width,   w)) return false;
        if (!MemUtil::RPM<int>(hProcess, miniMapBase + g_Maple.CUIMiniMap.Height,  h)) return false;
        // if (!MemUtil::RPM<int>(hProcess, miniMapBase + g_Maple.CUIMiniMap.CenterX, x)) return false;
        // if (!MemUtil::RPM<int>(hProcess, miniMapBase + g_Maple.CUIMiniMap.CenterY, y)) return false;
        // if (!MemUtil::RPM<uint32_t>(hProcess, miniMapBase  + g_Maple.CUIMiniMap.MapID,  id)) return false;


        Width   = w;
        Height  = h;


        return true;
    }


    static bool GetConstants(
        HANDLE hProcess,
        PhysicsParams::PhysicalSpaceParams& ParamsP,
        PhysicsParams::FieldParams& ParamsF) 
    {

        ParamsP.dWalkForce  = 140000.0;
        ParamsP.dWalkSpeed  = 125.0;
        ParamsP.dFloatDrag1 = 100000.0;
        ParamsP.dFloatDrag2 = 10000.0;
        ParamsP.dFloatCoeff = 0.01;
        ParamsP.dGravityAcc = 2000.0;
        ParamsP.dFallSpeed  = 670.0;
        ParamsP.dJumpSpeed  = 555.0;
        ParamsP.layer = 0;
        ParamsF.fieldDrag = 1.00;
        ParamsF.gravity   = 1.00;

        if (!hProcess) return false;
        
        uintptr_t spaceBase = 0;
        if (!MemUtil::RPM<uintptr_t>( hProcess, g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D, spaceBase) || !spaceBase) return false;

        uintptr_t constants = 0;
        if (!MemUtil::RPM(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.m_constants, constants) || !constants) return false;

        uintptr_t AttrField = 0;
        if (!MemUtil::RPM(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.m_pAttrField, AttrField) || !AttrField) return false;


        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.WalkForce , ParamsP.dWalkForce )) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.WalkSpeed , ParamsP.dWalkSpeed )) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.FloatDrag1, ParamsP.dFloatDrag1)) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.FloatDrag2, ParamsP.dFloatDrag2)) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.FloatCoeff, ParamsP.dFloatCoeff)) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.GravityAcc, ParamsP.dGravityAcc)) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.FallSpeed , ParamsP.dFallSpeed )) return false;
        if (!MemUtil::RPM<double>(hProcess, constants + g_Maple.CWvsPhysicalSpace2D.JumpSpeed , ParamsP.dJumpSpeed )) return false;
        if (!MemUtil::RPM<int>   (hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.BaseLayer,  ParamsP.layer      )) return false;


        if (!MemUtil::ReadTSecTypeDouble(hProcess, AttrField + g_Maple.CWvsPhysicalSpace2D.drag,    ParamsF.fieldDrag)) return false;
        if (!MemUtil::ReadTSecTypeDouble(hProcess, AttrField + g_Maple.CWvsPhysicalSpace2D.gravity, ParamsF.gravity))   return false;




        return true;
    }


    static bool GetBaseLayer(
        HANDLE hProcess,
        PhysicsParams::PhysicalSpaceParams& ParamsP,
        int& layer) 
    {

        if (!hProcess) return false;
        
        uintptr_t spaceBase = 0;
        if (!MemUtil::RPM<uintptr_t>( hProcess, g_Maple.CWvsPhysicalSpace2D.CWvsPhysicalSpace2D, spaceBase) || !spaceBase) return false;

        if (!MemUtil::RPM<int>(hProcess, spaceBase + g_Maple.CWvsPhysicalSpace2D.BaseLayer, layer)) return false;


        return true;
    }


};