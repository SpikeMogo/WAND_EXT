// Stripped for open-source release — type definitions only
#pragma once

#include <windows.h>
#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <utility>
#include <array>

// ============================================
//  PhysicsParams
// ============================================
namespace PhysicsParams
{
    struct FootholdParams
    {
        double footholdWalk = 1.0;
        double slopeUvX = 1.0;
        double slopeUvY = 0.0;
        double force = 0.0;
    };

    struct PhysicalSpaceParams
    {
        double dWalkSpeed = 125.0;
        double dWalkForce = 140000.0;
        double dJumpSpeed = 555.0;
        double dFloatDrag1 = 100000.0;
        double dFloatDrag2 = 10000.0;
        double dFloatCoeff = 0.01;
        double dFallSpeed = 670.0;
        double dGravityAcc = 2000.0;
        int layer = 0;
    };

    struct FieldParams
    {
        double fieldDrag = 1.0;
        double gravity = 1.0;
    };

    struct UserParams
    {
        double walkJump = 1.0;
        double shoeWalkSpeed = 1.0;
        double mass = 100.0;
        bool hasFlyStuff = false;
    };

    struct CVecCtrl
    {
        double X;
        double Y;
        double Vx;
        double Vy;
        uint32_t OnRopeAddress = 0;
        uint32_t OnFootholdAddress = 0;
        int32_t InputX;
        bool IsFalling = false;
        bool IsStopped = false;
        int32_t Layer;
    };

} // namespace PhysicsParams

// ============================================
//  MapStructures — forward declarations
// ============================================
namespace MapStructures
{
    struct Foothold;
    struct LadderRope;
    struct Platform;
}

// ============================================
//  Movements
// ============================================
namespace Movements
{
    using namespace MapStructures;

    enum class MoveType : uint8_t
    {
        None = 0,
        SimpleMove,
        DynamicJump,
        Teleport,
        Portal,
    };

    struct Movement
    {
        MoveType type = MoveType::None;

        Platform* pSrcPlatform = nullptr;
        Foothold* pSrcFoothold = nullptr;
        double srcX = 0;
        double srcY = 0;

        Platform* pDstPlatform = nullptr;
        Foothold* pDstFoothold = nullptr;

        LadderRope* pSrcLadderRope = nullptr;
        LadderRope* pDstLadderRope = nullptr;

        double dstX = 0;
        double dstY = 0;

        int durationMs = 100000;
        double cost_Scale = 1.0;

        virtual ~Movement() = default;

        bool IsValid() const { return type != MoveType::None && pDstPlatform != nullptr; }
        virtual std::string GetName() const { return "None"; }
    };

    template<typename T>
    inline T* MovementAs(Movement* move)
    {
        if (!move) return nullptr;
        return static_cast<T*>(move);
    }

    template<typename T>
    inline const T* MovementAs(const Movement* move)
    {
        if (!move) return nullptr;
        return static_cast<const T*>(move);
    }

    struct MoveSimple : Movement
    {
        enum class JumpType { JumpDown, RopeDown, RopeUp, None };

        int direction = 0;
        JumpType jumpType = JumpType::None;

        MoveSimple() { type = MoveType::SimpleMove; }
        std::string GetName() const override { return "MoveSimple"; }
    };

    struct MoveDynamicJump : Movement
    {
        enum class JumpType { Fall, Jump, None };

        int inputX = 0;
        double inputAir = 0;
        double xy1 = 0, xy2 = 0;
        double a1 = 0, a2 = 0;
        double ApeX = 0, ApeY = 0;
        JumpType jumpType = JumpType::None;

        MoveDynamicJump() { type = MoveType::DynamicJump; }
        std::string GetName() const override { return "DynamicJump"; }
    };

    struct MoveTeleport : Movement
    {
        int direction = 0;
        int skillId = 0;
        float teleportRange = 0.0f;

        MoveTeleport() { type = MoveType::Teleport; }
    };

    struct MovePortal : Movement
    {
        int portalId = 0;
        int targetPortalId = 0;

        MovePortal() { type = MoveType::Portal; }
    };

} // namespace Movements

// ============================================
//  TrajectoryComputer
// ============================================
namespace TrajectoryComputer
{
    struct FloatState
    {
        double x = 0, y = 0;
        double vx = 0, vy = 0;
    };

    struct Trajectory
    {
        bool target = false;
        double t_target = 0.0;
        std::vector<std::pair<MapStructures::LadderRope*, double>> ropeReach;

        double TimeToY(double y) const { return -1.0; }
        FloatState StateAt(double t) const { return {}; }
    };

} // namespace TrajectoryComputer

// ============================================
//  ReachabilityComputer
// ============================================
namespace ReachabilityComputer
{
    static constexpr int kNumCases = 5;

    struct Reach
    {
        double x0 = 0, y0 = 0;
        double vx0 = 0, vy0 = 0;

        const TrajectoryComputer::Trajectory& Get(int index) const
        {
            static TrajectoryComputer::Trajectory empty;
            return empty;
        }
    };

} // namespace ReachabilityComputer

// ============================================
//  MapStructures
// ============================================
namespace MapStructures
{
    enum class PathReturn { Error, Float, NoTarget, NotFound, Found };

    struct MapBounds
    {
        long left = 0;
        long right = 0;
        long top = 0;
        long bottom = 0;
    };

    struct Foothold
    {
        int id = 0;
        int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        int prev = 0, next = 0, size = 0;
        int layer = 0;

        PhysicsParams::FootholdParams Params;

        Foothold* p_Prev = nullptr;
        Foothold* p_Next = nullptr;
        Platform* p_Platform = nullptr;
        uintptr_t remoteAddress = 0;

        bool isWall = false;
        bool isAirWall = false;
        bool hasRopeConnection = false;
        bool checked = false;
        bool isLeftEdge = false;
        bool isRightEdge = false;

        std::vector<int> RopeFallX;
        std::vector<int> ropeHeads;

        double GetYAtX(int x) const { return 0.0; }
        double GetYAtX(double x) const { return 0.0; }
        bool onFoothold(double x, double y) const { return false; }
    };

    struct LadderRope
    {
        int id = 0;
        int x = 0, y1 = 0, y2 = 0, size = 0;
        bool isLadder = false;
        bool fromUpper = false;

        Foothold* p_TopFoothold = nullptr;
        Platform* p_Platform = nullptr;
        uintptr_t remoteAddress = 0;
    };

    struct Platform
    {
        int id = 0;
        int numSegments = 0;
        int leftX = 0, rightX = 0, centerX = 0, size = 0;

        Foothold* p_Begin = nullptr;
        Foothold* p_End = nullptr;
        LadderRope* p_LadderRope = nullptr;

        std::vector<std::unique_ptr<Movements::Movement>> Connections;
        std::unordered_map<int, std::vector<Movements::Movement*>> ConnectionsByDst;

        Platform() = default;
        Platform(Platform&&) = default;
        Platform& operator=(Platform&&) = default;
        Platform(const Platform&) = delete;
        Platform& operator=(const Platform&) = delete;

        bool IsLadderRope() const { return p_LadderRope != nullptr; }
    };

} // namespace MapStructures

// ============================================
//  PathFinding
// ============================================
namespace PathFinding
{
    using namespace MapStructures;
    using namespace Movements;

    enum class Destination : uint8_t { Location, Life, Portal, Drop };

    struct PathStep
    {
        enum class Action : uint8_t { Walk, Movement };

        Action action = Action::Walk;
        Platform* platform = nullptr;
        double startX = 0, startY = 0;
        double endX = 0, endY = 0;
        double walkCostMs = 0;
        Movement* movement = nullptr;
    };

    struct PathResult
    {
        PathReturn status = PathReturn::Error;

        Platform* srcPlatform = nullptr;
        Foothold* srcFoothold = nullptr;
        LadderRope* srcLadderRope = nullptr;
        double srcX = 0, srcY = 0;

        Platform* dstPlatform = nullptr;
        Foothold* dstFoothold = nullptr;
        LadderRope* dstLadderRope = nullptr;
        double dstX = 0, dstY = 0;

        std::vector<PathStep> steps;
        double totalCostMs = 0;
        long long computeTimeUs = 0;
        int currentStep = 0;

        bool IsValid() const { return status == PathReturn::Found && !steps.empty(); }
        bool IsComplete() const { return currentStep >= static_cast<int>(steps.size()); }

        PathStep* GetCurrentStep()
        {
            if (currentStep < static_cast<int>(steps.size()))
                return &steps[currentStep];
            return nullptr;
        }

        void AdvanceStep() { currentStep++; }

        void Clear()
        {
            status = PathReturn::Error;
            srcPlatform = dstPlatform = nullptr;
            srcFoothold = dstFoothold = nullptr;
            srcLadderRope = dstLadderRope = nullptr;
            srcX = srcY = dstX = dstY = 0;
            steps.clear();
            totalCostMs = 0;
            computeTimeUs = 0;
            currentStep = 0;
        }

        std::string PathHeadInfo() const { return {}; }
        void DebugPrint() const {}
    };

    struct PathSnapshot
    {
        struct Step
        {
            bool isWalk;
            double startX, startY;
            double endX, endY;
            int ApeX, ApeY = 10000;
        };

        std::vector<Step> steps;
        bool valid = false;
    };

    class PathRenderBuffer
    {
    public:
        void Update(const PathResult& result) {}
        const PathSnapshot* GetSnapshot() { return &m_readBuffer; }
        void Clear() {}

    private:
        PathSnapshot m_readBuffer;
        PathSnapshot m_writeBuffer;
        std::mutex m_mutex;
        bool m_dirty = false;
    };

    inline PathRenderBuffer g_PathRenderBuffer;

} // namespace PathFinding

// ============================================
//  MapleMap
// ============================================
class MapleMap
{
public:
    MapleMap() = delete;

    static inline uint32_t MapID = 0;
    static inline uint32_t ChannelID = 0;

    static inline MapStructures::MapBounds Bounds{};

    static inline PhysicsParams::PhysicalSpaceParams ParamsP{};
    static inline PhysicsParams::FieldParams         ParamsF{};
    static inline PhysicsParams::UserParams          ParamsU{};

    static inline std::vector<MapStructures::Foothold>   Footholds;
    static inline std::vector<MapStructures::LadderRope> LadderRopes;
    static inline std::vector<MapStructures::Platform>   Platforms;

    static inline std::unordered_map<uint32_t, MapStructures::Foothold*>   LoopUpFoothold;
    static inline std::unordered_map<uint32_t, MapStructures::LadderRope*> LoopUpLadderRope;

    struct KeySet { UINT JumpKey; UINT TeleKey; };
    static inline KeySet Keys{};

    static inline int NumFootholds = 0;
    static inline int NumLadderRopes = 0;
    static inline int NumPlatforms = 0;

    static inline PathFinding::PathResult CurrentPath{};

    static bool LoadMap(HANDLE hProcess) { return false; }
    static bool IfFieldReady(HANDLE hProcess) { return false; }

    static bool ComputeInstantReach(ReachabilityComputer::Reach& Reach) { return false; }
    static int ComputeInstantReach(Movements::MoveDynamicJump* Dynamic, ReachabilityComputer::Reach& Reach, double& xy1, double& xy2) { return 0; }
    static bool ComputeInstantJumpReach(ReachabilityComputer::Reach& Reach) { return false; }
    static int ComputeInstantJumpReach(Movements::MoveDynamicJump* Dynamic, ReachabilityComputer::Reach& Reach, int& airInputX, double& xy1, double& xy2, int move = 1) { return 0; }
    static bool TestReach(ReachabilityComputer::Reach& Reach) { return false; }

    static MapStructures::PathReturn Findpath(double x, double y) { return MapStructures::PathReturn::Error; }
    static MapStructures::PathReturn Findpath(MapStructures::Foothold* DstFh, MapStructures::LadderRope* DstLr, double x, double y) { return MapStructures::PathReturn::Error; }
    static MapStructures::LadderRope* FindLrByPoint(double x, double y, PathFinding::Destination Type = PathFinding::Destination::Location) { return nullptr; }
    static MapStructures::Foothold* FindFhBelowPoint(double x, double y, PathFinding::Destination Type = PathFinding::Destination::Location) { return nullptr; }

    static bool MoveTo(MapStructures::Foothold* DstFh, MapStructures::LadderRope* DstLr, double x, double y, PathFinding::Destination Type = PathFinding::Destination::Location) { return false; }

    static int GetMobPlatformID(double x, double y) { return 0; }

    static inline Movements::Movement* AeroDynamicsMovement = nullptr;

    static MapStructures::Foothold* FindFhByAddress(uint32_t address) { return nullptr; }
    static MapStructures::LadderRope* FindLrByAddress(uint32_t address) { return nullptr; }
};
