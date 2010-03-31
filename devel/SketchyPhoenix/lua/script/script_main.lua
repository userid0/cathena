--------------------------------------------------------------
--                eAthena Primary Scripts File              --
--------------------------------------------------------------
--  The idea of this new system is to make scripts more organized
-- since the old system was rather messy with all the NPCs in one
-- file. Now scripts are organized in to files arraged by type.
-- Custom scripts are now in script_custom.lua, all other
-- scripts are deemed as 'official'. You should place your NPCs
-- in to script_custom.lua to follow the trend.
--
-- Thanks,
--  Ancyker and the rest of the eAthena Team

--------------------------------------------------------------
------------------- Core Scripts Functions -------------------
-- Simply do NOT touch this !
require "script/core/core"
--------------------------------------------------------------
------------------------ Script Files ------------------------
-- Sample scripts
--require "script/sample/script_sample"
-- Warps
--require "script/warps/script_warps"
-- Monster Spawn
require "script/spawns/script_spawns"
-- Shops/Merchants
--require "script/merchants/script_merchants"
-- Your NPCs go in this file!
require "script/custom/script_custom"
--------------------------------------------------------------
