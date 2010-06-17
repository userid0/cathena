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

--Returns the variable.
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


-- Global Storage (temp vars)

Server = {
	global_db = {}
}

function Server.SetGlobal(name,value)
	for x = 1,table.getn(Server.global_db) do
		if name == Server.global_db[x] then
			table.insert(Server.global_db,x,name)
			table.insert(Server.global_db,x+1,value)
		end
	end
		table.insert(Server.global_db,name)
		table.insert(Server.global_db,value)
end

function Server.Global(name)
	for x = 1,table.getn(Server.global_db) do
		if name == Server.global_db[x] then
			return Server.global_db[x+1]
		end
	end
end