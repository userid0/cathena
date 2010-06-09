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

function getmapxy(...)
	return getlocation(arg)
end

function isday()
	return (getnightflag==0) and 1 or nil
end

function isnight()
	return (getnightflag==1) and 1 or nil
end

function day()
	return togglenight()
end

function night()
	return togglenight()
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

