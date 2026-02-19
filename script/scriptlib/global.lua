--------------------------------------------------------------------------------
-- Global State Module
-- Shared state for coordinating between modules
--------------------------------------------------------------------------------

local module = {
  
  -- Module enable flags
  enabled = {
      hunt = true,
      loot = true,
      shop = false,
  },
}

return module