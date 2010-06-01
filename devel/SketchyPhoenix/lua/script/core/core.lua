--[[ ===============

   CONSTANTS

================ --]]
JOB_NOVICE = 0
JOB_SWORDMAN = 1
JOB_MAGE = 2
JOB_ARCHER = 3
JOB_ACOLYTE = 4
JOB_MERCHANT = 5
JOB_THIEF = 6
JOB_KNIGHT = 7
JOB_PRIEST = 8
JOB_WIZARD = 9
JOB_BLACKSMITH = 10
JOB_HUNTER = 11
JOB_ASSASSIN = 12
JOB_KNIGHT_PECO = 13
JOB_CRUSADER = 14
JOB_MONK = 15
JOB_SAGE = 16
JOB_ROGUE = 17
JOB_ALCHEMIST = 18
JOB_BARD = 19
JOB_DANCER = 20
JOB_CRUSADER_PECO = 21
JOB_SUPERNOVICE = 23
JOB_NOVICE_HIGH = 4001
JOB_SWORDMAN_HIGH = 4002
JOB_MAGE_HIGH = 4003
JOB_ARCHER_HIGH = 4004
JOB_ACOLYTE_HIGH = 4005
JOB_MERCHANT_HIGH = 4006
JOB_THIEF_HIGH = 4007
JOB_LORD_KNIGHT = 4008
JOB_HIGH_PRIEST = 4009
JOB_HIGH_WIZARD = 4010
JOB_WHITESMITH = 4011
JOB_SNIPER = 4012
JOB_ASSASSIN_CROSS = 4013
JOB_LORD_KNIGHT_PECO = 4014
JOB_PALADIN = 4015
JOB_CHAMPION = 4016
JOB_PROFESSOR = 4017
JOB_STALKER = 4018
JOB_CREATOR = 4019
JOB_CLOWN = 4020
JOB_GYPSY = 4021
JOB_PALADIN_PECO = 4022
JOB_BABY = 4023
JOB_BABY_SWORDMAN = 4024
JOB_BABY_MAGE = 4025
JOB_BABY_ARCHER = 4026
JOB_BABY_ACOLYTE = 4027
JOB_BABY_MERCHANT = 4028
JOB_BABY_THIEF = 4029
JOB_BABY_KNIGHT = 4030
JOB_BABY_PRIEST = 4031
JOB_BABY_WIZARD = 4032
JOB_BABY_BLACKSMITH = 4033
JOB_BABY_HUNTER = 4034
JOB_BABY_ASSASSIN = 4035
JOB_BABY_KNIGHT_PECO = 4036
JOB_BABY_CRUSADER = 4037
JOB_BABY_MONK = 4038
JOB_BABY_SAGE = 4039
JOB_BABY_ROGUE = 4040
JOB_BABY_ALCHEM = 4041
JOB_BABY_BARD = 4042
JOB_BABY_DANCER = 4043
JOB_BABY_CRUSADER_PECO = 4044
JOB_SUPER_BABY = 4045

--[[ ===============

   BUILDIN FUNCTIONS

================ --]]
function menu(...)
	return npcmenu(arg)
end

function shop(...)
	return npcshop(arg)
end

function setmapflag (...)
	return editmapflag(arg)
end

function removemapflag (mapname,flag)
	return editmapflag(mapname,flag,0)
end

function addtoskill(...)
	local va_list = {...}
	return skill(va_list[1],va_list[2], 2)
end

--sleep() command
local clock = os.clock
function sleep(n)  -- seconds
  local t0 = clock()
  while clock() - t0 <= n do end
end

--Return timer tick in milliseconds
function gettick(n)
	local clock = os.clock
	return clock - n/1000
end

function disablenpc(name)
	return npc_enable(name,0)
end

function enablenpc(name)
	return npc_enable(name,1)
end

function setmapflag (...)
	return editmapflag(arg)
end

function removemapflag (...)
	return editmapflag(arg)
end

function addnpc (name,exname,map,x,y,dir,class_,func)
	local id = _addnpc(name,exname,map,x,y,dir,class_,func)
	return id, name, exname
end

function getiteminfo (itemid,itemtype)
	return iteminfo(itemid,itemtype,0)
end

function setiteminfo (itemid,itemtype,value)
	return iteminfo(itemid,itemtype,1,value)
end

function getinventorylist (inventorylist,charid)
	if charid == "self" then
		charid = nil
	end
	inventorylist.id = _getinventorylist(1,charid)
	inventorylist.amount = _getinventorylist(2,charid)
	inventorylist.equip = _getinventorylist(3,charid)
	inventorylist.refine = _getinventorylist(4,charid)
	inventorylist.identify = _getinventorylist(5,charid)
	inventorylist.attribute = _getinventorylist(6,charid)
	inventorylist.card1 = _getinventorylist(7,charid)
	inventorylist.card2 = _getinventorylist(8,charid)
	inventorylist.card3 = _getinventorylist(9,charid)
	inventorylist.card4 = _getinventorylist(10,charid)
	inventorylist.expire = _getinventorylist(11,charid)
	inventorylist.count = _getinventorylist(12,charid)
	return inventorylist
end

function getskilllist (skilllist,charid)
	if charid == "self" then
		charid = nil
	end
	skilllist.id = _getskilllist(1,charid)
	skilllist.lv = _getskilllist(2,charid)
	skilllist.flag = _getskilllist(3,charid)
	skilllist.count = _getskilllist(4,charid)
	return skilllist
end

function disguise (id,charid)
	if charid == "self" then
		charid = nil
	end
	
	return _disguise(id,1,charid)
end

function undisguise (id,charid)
	if charid == "self" then
		charid = nil
	end
	
	return _disguise(id,0,charid)
end

--[[ ==============

	MISC FUNCTIONS

============== --]]
TYPE_NIL = 0
TYPE_NUM = 1
TYPE_STR = 2
TYPE_FUNC = 3
TYPE_TBL = 4
function checktype (var)
	if type(var) == "number" then
		return TYPE_NUM
	elseif type(var) == "string" then
		return TYPE_STR
	elseif type(var) == "function" then
		return TYPE_FUNC
	elseif type(var) == "table" then
		return TYPE_TBL
	else
		return nil
	end
end


--[[ ====================

	NPC VARIABLE STORAGE

====================  --]]

NPC = {}
NPC.__index = NPC

--Constructor
function NPC.create()
	local list = {
	vars = {},
	ids = {},
	names = {},
	exnames = {},
	}
	setmetatable(list,NPC)
	for k, v in pairs (list) do setmetatable(v,NPC) end
	return list
end

--Adds a NPC to its shared index
function NPC:AddObject(oid,name,exname)
	for i=1,table.getn(self.ids) do
		if self.ids[i] == nil and self.names[i] == nil and self.exnames[i] == nil then
			table.insert(self.ids,i,oid)
			table.insert(self.names,i,name)
			table.insert(self.exnames,i,exname)
		end
	end
end

--Sets a variable
function NPC:Set(name,value)
	for x = 1,table.getn(self.vars) do
		if name == self.vars[x] then
			table.insert(self.vars,x,name)
			table.insert(self.vars,x+1,value)
		end
	end
		table.insert(self.vars,name)
		table.insert(self.vars,value)
end

--References the variable.
function NPC:Variable(name)
	for x = 1,table.getn(self.vars) do
		if name == self.vars[x] then
			return self.vars[x+1]
		end
	end
end

--Returns random ID in the index
function NPC:getanyID()
	return math.random(table.getn(self.ids))
end

function NPC:getidbyname(name)
	for i=1,table.getn(self.names) do
		if self.names[i] == name then
			return self.ids[i]
		end
	end
end

function NPC:getidbyexname(exname)
	for i=1,table.getn(self.exnames) do
		if self.exnames[i] == exname then
			return self.ids[i]
		end
	end
end

function NPC:getnamebyid(id)
	for i=1,table.getn(self.ids) do
		if self.ids[i] == id then
			return self.names[i]
		end
	end
end

function NPC:getexnamebyid(id)
	for i=1,table.getn(self.ids) do
		if self.ids[i] == id then
			return self.exnames[i]
		end
	end
end
