--[[
    Distance Functions Module
    
    Collection of distance calculation functions for target selection in hunting scripts.
    Each function takes two points (x1,y1) and (x2,y2) plus an optional parameter table,
    returning a weighted "distance" value used to prioritize targets.
    
    Lower return values = higher priority targets.
    
    Usage:
        local dist = require("DistanceFunction")
        local d = dist.euclidean(px, py, mx, my)
        local d = dist.facing_weighted(px, py, mx, my, {1.5, 1.0})
]]

local DistanceFunction = {}

--------------------------------------------------------------------------------
-- Constants
--------------------------------------------------------------------------------

local MIN_WEIGHT = 0.1      -- Minimum allowed weight (prevents division issues)
local MAX_WEIGHT = 10.0     -- Maximum allowed weight (prevents extreme bias)
local DEFAULT_WEIGHT = 1.5  -- Default directional weight

--------------------------------------------------------------------------------
-- Internal Helpers
--------------------------------------------------------------------------------

--[[
    Clamps a value between min and max bounds.
]]
local function clamp(value, min_val, max_val)
    if value < min_val then return min_val end
    if value > max_val then return max_val end
    return value
end

--[[
    Validates and clamps a weight parameter.
    Ensures weight is always >= 1.0 so that:
      - Preferred direction gets weight 1/w (reduced distance)
      - Non-preferred direction gets weight w (increased distance)
    
    @param weight   Raw weight value from params
    @param default  Default value if weight is nil
    @return         Clamped weight guaranteed to be in [1.0, MAX_WEIGHT]
]]
local function validate_weight(weight, default)
    local w = weight or default or DEFAULT_WEIGHT
    return clamp(math.abs(w), 1.0, MAX_WEIGHT)
end

--[[
    Validates a scale parameter (can be < 1.0, just needs to be positive).
    
    @param scale    Raw scale value from params
    @param default  Default value if scale is nil
    @return         Clamped scale guaranteed to be in [MIN_WEIGHT, MAX_WEIGHT]
]]
local function validate_scale(scale, default)
    local s = scale or default or 1.0
    return clamp(math.abs(s), MIN_WEIGHT, MAX_WEIGHT)
end

--------------------------------------------------------------------------------
-- Basic Distance Functions
--------------------------------------------------------------------------------

--[[
    Standard Euclidean distance.
    
    @param x1, y1   First point (typically player position)
    @param x2, y2   Second point (typically mob position)
    @return         Straight-line distance between points
]]
function DistanceFunction.euclidean(x1, y1, x2, y2, params)
    local dx = x1 - x2
    local dy = y1 - y2
    return math.sqrt(dx * dx + dy * dy)
end

--------------------------------------------------------------------------------
-- Weighted Distance Functions
--------------------------------------------------------------------------------

--[[
    Y-axis scaled distance.
    Useful for maps where vertical movement is more costly than horizontal.
    
    @param x1, y1   First point
    @param x2, y2   Second point
    @param params   {y_scale} - multiplier for vertical distance
                    y_scale > 1: penalize vertical distance (prefer same-platform targets)
                    y_scale < 1: reduce vertical importance
                    Clamped to [0.1, 10.0]
    @return         Scaled distance value
    
    Example: params = {2.0} makes vertical distance count double
]]
function DistanceFunction.y_scaled(x1, y1, x2, y2, params)
    params = params or {}
    local y_scale = validate_scale(params[1], 1.0)
    
    local dx = x1 - x2
    local dy = (y1 - y2) * y_scale
    return math.sqrt(dx * dx + dy * dy)
end

--[[
    X and Y axis independently scaled distance.
    
    @param x1, y1   First point
    @param x2, y2   Second point
    @param params   {x_scale, y_scale} - multipliers for each axis
                    Both clamped to [0.1, 10.0]
    @return         Scaled distance value
]]
function DistanceFunction.xy_scaled(x1, y1, x2, y2, params)
    params = params or {}
    local x_scale = validate_scale(params[1], 1.0)
    local y_scale = validate_scale(params[2], 1.0)
    
    local dx = (x1 - x2) * x_scale
    local dy = (y1 - y2) * y_scale
    return math.sqrt(dx * dx + dy * dy)
end

--[[
    Minkowski distance with separate exponents for X and Y.
    Generalization of Euclidean (p=2) and Manhattan (p=1) distances.
    
    @param x1, y1   First point
    @param x2, y2   Second point
    @param params   {x_power, y_power} - exponents for each axis
                    Clamped to [1.0, 4.0] to prevent extreme behavior
    @return         Minkowski-style distance
]]
function DistanceFunction.minkowski(x1, y1, x2, y2, params)
    params = params or {}
    local x_power = clamp(params[1] or 2, 1.0, 4.0)
    local y_power = clamp(params[2] or 2, 1.0, 4.0)
    
    local dx = math.abs(x1 - x2)
    local dy = math.abs(y1 - y2)
    return math.pow(dx, x_power) + math.pow(dy, y_power)
end

--------------------------------------------------------------------------------
-- Player-State Weighted Functions
--------------------------------------------------------------------------------

--[[
    Facing direction weighted distance.
    Prioritizes targets in the direction the player is facing.
    
    Weight behavior (guaranteed by validation):
      - Targets in front of player: distance multiplied by 1/weight (closer)
      - Targets behind player: distance multiplied by weight (further)
    
    @param x1, y1   Player position
    @param x2, y2   Target position
    @param params   {facing_weight, y_scale}
                    facing_weight: bias factor, clamped to [1.0, 10.0]
                                   Higher = stronger preference for facing direction
                    y_scale: vertical distance multiplier, clamped to [0.1, 10.0]
    @return         Direction-weighted distance
    
    Example: params = {2.0, 1.5}
             - Mobs in front appear 2x closer, mobs behind 2x further
             - Vertical distance scaled by 1.5x
]]
function DistanceFunction.facing_weighted(x1, y1, x2, y2, params)
    params = params or {}
    local facing_weight = validate_weight(params[1], 1.5)
    local y_scale = validate_scale(params[2], 1.0)
    
    local player = get_player()
    
    -- Convert faceDir (0=left, 1=right) to sign (-1=left, 1=right)
    local facing_sign = player.faceDir == 0 and -1 or 1
    
    -- Target direction relative to player (positive = target is to the right)
    local target_dir = x2 - x1
    
    -- Apply weight: favor targets in facing direction
    local x_weight
    if target_dir * facing_sign > 0 then
        -- Target is in front of player - reduce effective distance
        x_weight = 1.0 / facing_weight
    else
        -- Target is behind player - increase effective distance
        x_weight = facing_weight
    end
    
    local dx = (x1 - x2) * x_weight
    local dy = (y1 - y2) * y_scale
    return math.sqrt(dx * dx + dy * dy)
end

--[[
    Input direction weighted distance.
    Prioritizes targets in the direction of current arrow key input.
    Useful for responsive target switching while actively moving.
    
    Weight behavior (guaranteed by validation):
      - Horizontal: targets in facing direction get reduced distance
      - Vertical: targets in input direction (up/down) get reduced distance
    
    @param x1, y1   Player position
    @param x2, y2   Target position
    @param params   {horizontal_weight, vertical_weight}
                    Both clamped to [1.0, 10.0]
    @return         Input-weighted distance
    
    Behavior:
      - Horizontal always uses facing direction (not left/right keys)
      - Vertical uses up/down key input, neutral if no vertical input
]]
function DistanceFunction.input_weighted(x1, y1, x2, y2, params)
    params = params or {}
    local h_weight = validate_weight(params[1], 1.5)
    local v_weight = validate_weight(params[2], 1.5)
    
    local player = get_player()
    
    -- Get current facing direction
    local facing_sign = player.faceDir == 0 and -1 or 1
    
    -- Get arrow key states
    local left, up, right, down = get_arrow_key_states()
    
    -- Determine vertical input direction (up = -1, down = +1, none = 0)
    local v_input = 0
    if up == 1 then v_input = -1 end
    if down == 1 then v_input = 1 end
    
    -- Calculate horizontal weight based on facing vs target
    local target_h_dir = x2 - x1
    local x_weight
    if target_h_dir * facing_sign > 0 then
        x_weight = 1.0 / h_weight  -- Target in front - reduce distance
    else
        x_weight = h_weight  -- Target behind - increase distance
    end
    
    -- Calculate vertical weight based on input vs target
    local target_v_dir = y2 - y1
    local y_weight
    if v_input == 0 then
        -- No vertical input - neutral weighting
        y_weight = 1.0
    elseif target_v_dir * v_input > 0 then
        -- Target in input direction - reduce distance
        y_weight = 1.0 / v_weight
    else
        -- Target opposite to input - increase distance
        y_weight = v_weight
    end
    
    local dx = (x1 - x2) * x_weight
    local dy = (y1 - y2) * y_weight
    return math.sqrt(dx * dx + dy * dy)
end

--------------------------------------------------------------------------------
-- Utility Functions
--------------------------------------------------------------------------------

--[[
    Manhattan distance (L1 norm).
    Sum of absolute differences - useful for grid-based calculations.
]]
function DistanceFunction.manhattan(x1, y1, x2, y2, params)
    return math.abs(x1 - x2) + math.abs(y1 - y2)
end

--[[
    Chebyshev distance (L-infinity norm).
    Maximum of absolute differences - "king's move" distance.
]]
function DistanceFunction.chebyshev(x1, y1, x2, y2, params)
    return math.max(math.abs(x1 - x2), math.abs(y1 - y2))
end

--------------------------------------------------------------------------------
-- Configuration Helpers
--------------------------------------------------------------------------------

--[[
    Returns the weight bounds for reference.
]]
function DistanceFunction.get_weight_bounds()
    return {
        min_weight = MIN_WEIGHT,
        max_weight = MAX_WEIGHT,
        default_weight = DEFAULT_WEIGHT
    }
end

return DistanceFunction