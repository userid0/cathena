addnpc("Socket NPC","socket_test","prontera",155,184,4,851,"socket_hw")

function socket_hw()
	mes(socket._VERSION)
	mes(Server.Global("variable"))
	mes(retrcharid())
	close()
end