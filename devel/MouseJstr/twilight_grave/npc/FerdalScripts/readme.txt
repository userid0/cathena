Houses are a new directory under the NPC directory. These are the scripts we'll need for 
player houses. Also, in the mapreg in the save directory, add the line: 

$daysleft	30

That make it count the days down until bills come. Thing about the houses is I dunno if it
will use the bank that tRO already uses, so if you'd like, you can look over the scripts
I got. Also, just so we know I didn't make this, this is from a poster on the eAthena
forums named ImAwesome. Also, I've had to add duplicates of the prt_in maps to get it to
work, also the bmp map for it, which goes into your data>texture>유저인터페이스>map
directoryDuplicates were made so the houses already there and used by NPCs aren't effected 
by this. The prt_in2.gat, ".gnd, ".rsw go into the data folder. Emuz has said there's an
easier way of doing all this but I'm not sure how, 'said all we need is a duplicate of the
prt_in2.rsw I'll try it out soon.

Anyways, check that over.

Also, I found a "Trading Agency" but I just found out there's been a bug to duplicate items
so I won't send that yet, though I'll send the Bank of Alberta script. This bank allows
players to use it like an item storage, 'cept instead of items it's zeny. They can put in
say 15 z on one char, and when they get on another char in their account they can access
that same 15 z. Also, each player is given an ID number for their bank accounts. Players can
transfer zeny to another player's account by his/her ID number. Whether you guys want to
have this it's up to you, but I think it's a pretty neat idea. But if you did add it it'd
have to replace the bank used in tRO and the bank used in the Player Houes, which I don't
know how to do. In any case, it's up to you. BankofAlberta-eng shouldgo into the other 
directory in the npc directory. This was a script posted on the eAthena forums by
Shadow_Arch with a few things tweeked, just the NPC sprite and where it's at. Feel free
to edit it. :D

I added in the ayothaya NPC script I used in HeRO. Not a lot but it has an armor/weapon
shops, a smith, a refiner, repairman, a Pharacon and an emvertacon seller, a warper,
a kafra, some guards. I'll update it as I go along. The ayothaya.txt should go into the
cities directory in the npc directory. Also, guides_ayo is the ayothaya guide I made, well,
editted from the prontera guide so it'd be more Ayothaya like. :) If you want to change
the description on each spot go ahead, I just thought Ayothaya could be a good
historical/resort city.

The GefeAyo.txt is the mobs for Geffenia and Ayothaya field/dungeons. Take a look at them
I think we need to tweek 'em some more. These mobs were made by our GM, Rude, also known as
healplz, so don't give me the credit. :)

Edit:
Ok I took out the prt_in duplicate stuff, I'm sure you know how to do all that, it was
taking too long to upload.

Added:
I added a new dye.txt This one allows players to "browse" colors and the like, and it 
tells you what number you're on. It also lets you change your hairstyle I'm pretty sure. 
:P

Edit:
I editted the dye.txt and am gonna replace the old one with it. It allows players to browse
their palette options if they aren't sure what they want or know a number by heart. It also
allows the player to see what hairstyle they want.

Edit:
Ok, I've updated the dye.txt with the newer palettes and restrictions. Should work now.

Added:
rudescript.txt is the scripts Rude-GM has sent me recently, check them out. :)

Added:
newarena.txt should replace the arena script we have already. It's got a better selection
I think and I transferred Time Attack over.

Added:
rudescript2.txt Rude looked over the server's payon and found some NPCs missing,
he also figured we could use an NPC that players can sell to, basicly.

Transferred:
Rude's scripts (rudescript.txt, rudescript2.txt, and GefeAyo.txt) to the RudeScripts
subdirectory in scripts/twilight.

Added:
Editted prt_are_in with some new shiot that should make it a lot more useful. It's got
PvP, GvG, and MvP warps. Check it out and see how you like it, I think it's pretty
neat. :)