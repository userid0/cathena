//  _______________________________________________________
// /                                                       \
// |        ########   ######   #######                     |
// |      ###    ### ########  ###   ##                     |
// |     ###    ### ###       ###   ##                      |
// |    ########## #######   #######   %%%   %%%% %   % *   |
// |   ##########  ######## ########  %  %  %    %   %  %%%%|
// |  ###    ###       ### ###   ### %   % %%%  %   %  %%   |
// | ###    ###  ######## ###   ### %  %  %     % %      %% |
// |###    ###  #######  ########  %%%   %%%%   %    %%%%   |
// |      ASB Package                                       |
// |--------------------------------------------------------|
// | Created By : Perkka                Date:  2005/09/30   |
// | Credits    : Scriptor                                  |
// | Revisions  : Ether                                     |
// |--------------------------------------------------------|
// | Notes: This release got a lot of very bad english      |
// | and needs to be unengrished.                           |
// | Some parts are as good as impossible to understand     |
// | UPDATE: De-Enrishment and some indonesian translations |
// | handled by Ether. If you notice a problem with the     |
// | translation, please send an email to                   | 
// | ether@ether.anti-gravity.cc                            |
// \_______________________________________________________/



npc "prt_church" "Marry Happy" 1_F_LIBRARYGIRL 94 100 4 1 1
OnClick:
var baby_w = IsBaby
  if (baby_w == 1)
    dialog "[Marry Happy]"
    dialog "Hello~"
    dialog "I am Marry Happy. If you want to fill out a wedding form, it must go through me."
    dialog "What do you wish to do? ?"
    wait
    choose menu "I want to marry." "Nothing."
      case 1
        dialog "[Marry Happy]"
        dialog "Forgive me..."
        dialog "But children can not marry..."
        dialog "So you cannot register."
        dialog "Doesn't that fit you better ?"
        dialog "You were definently not happy when living with your parents."
        close
      break
      case 2
        dialog "[Marry Happy]"
        dialog "Ok then."
        dialog "See you later."
        close
      break
    endchoose
  return
  endif
	dialog "[Marry Happy]"
	dialog "Marriage refers to"
	dialog "that hope that everybody will find their desired happiness."
	dialog "So, may I ask..."
	dialog "is there someone special you wish to be with forever?"
	wait
	choose menu "Inquire about the Wedding ceremony." "Inquire about the procedure of a Wedding ceremony." "Apply for a Wedding ceremony." "I am the invincible single army!"
		case 1
			dialog "[Marry Happy]"
			dialog "The ceremony will be conduncted at Rune Midgard's Church."
			dialog "His Majesty King Tristan III"
			dialog "will bless his beloved citizens"
			dialog "with his honorable presence..."
			dialog "and be the high priest who conducts the wedding ceremony."
			wait
			dialog "[Marry Happy]"
			dialog "Although his Majesty is busy every day,"
			dialog "he is willing to be present at Prontera's Church"
			dialog "for the newly weds"
			dialog "in a hope that they'll live happily ever after."
			dialog "This is really a blessing from His Majesty."
			wait
			dialog "[Marry Happy]"
			dialog "A proposal for marriage is an honorable thing."
			dialog "Therefore, consider it carefully before you do it."
			dialog "Once two people get married,"
			dialog "they will never be seperated again."
			dialog "The couple will stay together forever,"
			dialog "and, only in death will they part."
			wait
			dialog "[Marry Happy]"
			dialog "Besides that, only a man can marry a woman,"
			dialog "and, only a woman can marry a man."
			dialog "It might be possible that"
			dialog "some will suggest marriage among same sex or marrying a monster."
			dialog "Such suggestions... are not acceptable."
			wait 	
			dialog "[Marry Happy]"
			dialog "If you have someone special,"
			dialog "why not try to make a marriage proposal?"
			dialog "I wish that all the married couples"
			dialog "will find their desired happiness!!"
			close
		break
		case 2
			dialog "[Marry Happy]"
			dialog "First, I'll have to make sure whether the bride and bridegroom"
			dialog "have completed the application."
			dialog "Of course, both of you must not get married before"
			dialog "you have already filled out the application."
			dialog "Please proceed to meet the King after you've formed a party of two."
			wait
			dialog "[Marry Happy]"
			dialog "When you've met the King,"
			dialog "the bridegroom shall speak first"
			dialog "and mention the name of the bride."
			dialog "You have to mention the correct name,"
			dialog "or the ceremony will not be able to continue."
			wait
			dialog "[Marry Happy]"
			dialog "After that..."
			dialog "the bride will have to speak to the King."
			dialog "His Majesty will ask"
			dialog "the name of the bridegroom."
			dialog "So, answer it correctly."
			wait
			dialog "[Marry Happy]"
			dialog "If both names are correct,"
			dialog "they will be able to exchange rings."
			dialog "From that moment onwards,"
			dialog "the couple will stay together forever, isn't this right?"
			dialog "Well, maybe you will be rejected..."
			dialog "but don't be sad."
			wait
			dialog "[Marry Happy]"
			dialog "If there are too many couples"
			dialog "who want to get married,"
			dialog "please queue up and talk to His Majesty in order."
			dialog "For it is impossible for His Majesty"
			dialog "to conduct several ceremonies at the same time."
			wait
			dialog "[Marry Happy]"
			dialog "Last but not least: "
			dialog "be quick when you mention your partner's name."
			dialog "The bride ^ff0000is given 3 minutes after "
			dialog "the bridegroom has finished his speech^000000..."
			dialog "or else"
			dialog "the ceremony will be cancelled."
			wait
			dialog "[Marry Happy]"
			dialog "The easiest way to write the partner's name is"
			dialog "to send a private message to the other side"
			dialog "the left click"
			dialog "the mouse cursor"
			dialog "on the name section at the chat window."
			wait
			dialog "[Marry Happy]"
			dialog "That means you'd be sending a private message."
			dialog "Press the 'Ctrl' button"
			dialog "and 'C' button at the same time to copy the name."
			dialog "When you're asked to key in the name"
			dialog "press the 'shift' button"
			dialog "and 'insert' button at the same time"
			dialog "the name will be inserted there easily."
			wait
			dialog "[Marry Happy]"
			dialog "Let's try it then."
			dialog "Write the name of your partner"
			dialog "according to the method mentioned."
			dialog "Press 'Ctrl' button together with 'C' button at the same time"
			dialog "Then, answer 'Yes'."
			dialog "After that, press the 'shift' button and 'insert'"
			dialog "button at the same time."
			wait
			dlgwritestr
			dialog "[Marry Happy]"
			dialog "Good. Nice job."
			dialog "If you have made up your mind, do come and submit your application here."
			close
		break
		case 3
			dialog "[Marry Happy]"
			dialog "If you want to make an application..."
			dialog "The bridegroom has to pay ^3377FF1,300,000 zeny and"
			dialog "prepare Tuxedo x 1^000000, while the bride has to pay^3377FF1,200,000"
			dialog "zeny and Wedding Dress x 1^000000. "
			dialog "Then, fill out the application form."
			dialog "If both the bride and bridegroom have completed it,"
			dialog "they can get married."
			wait
			dialog "[Marry Happy]"
			dialog "Besides that, you'll have to prepare the wedding ring too."
			dialog "Both the bride and bridegroom have to prepare ^3377FFDiamond Ring x 1^000000"
			dialog "and give it to me"
			dialog "when you submit your application."
			wait
			dialog "[Marry Happy]"
			dialog "After you complete all the procedures,"
			dialog "you can have the wedding ceremony."
			dialog "Do you want to fill out a marriage application?"
			wait
			choose menu "Yes" "No" 
				case 1
					if v[VAR_ISMARRIED] == 1
						dialog "[Marry Happy]"
						dialog "For those who've already got married..."
						dialog "What will your partner think"
						dialog "if you want to marry another person?"
						dialog "Haven't you sworn to stay together forever?"
						dialog "What will that be"
						dialog "if you've broken the vow?"
						wait
						dialog "[Marry Happy]"
						dialog "I think you've heard..."
						dialog "Once you're married, you won't be able to return to previous status."
						dialog "This shall remain even when the partner leaves the world..."
						dialog "Besides that, those who"
						dialog "fool around with others' feelings"
						dialog "will feel the bad karma in future."
						close
					elseif v[wedding_sign] == 1
						dialog "[Marry Happy]"
						dialog "Hmm...? Did you already fill out the form??"
						dialog "Since you've equipped your form,"
						dialog "I'll ask you..."
						dialog "Has your partner also filled out the registration form?"
						dialog "If the process is done,"
						dialog "then please proceed through the room and talk to the King to start the ceremony."
						close
					elseif v[VAR_CLEVEL] < 45
						dialog "[Marry Happy]"
						dialog "Before you propose to your lover,"
						dialog "you should think of your growth."
						dialog "Maybe you can find a better future,"
						dialog "but after you're married"
						dialog "you have the responsibility to take care of your partner too."
						wait
						dialog "[Marry Happy]"
						dialog "If you think that you have come to the stage"
						dialog "where you can protect your loved ones..."
						dialog "Find me then."
						dialog "I'll always be waiting for your presence."
						close
					elseif v[Diamond_Ring] < 1
						dialog "[Marry Happy]"
						dialog "Looks like you've forgotten"
						dialog "to bring along the Diamond Ring that is used as a wedding ring."
						dialog "Have you misplaced it somewhere else?"
						dialog "Look for it carefully and return to me when you find it."
						close
					elseif ((v[VAR_SEX] == 0) & (v[VAR_MONEY] < 1200000))
						dialog "[Marry Happy]"
						dialog "Aiyo aiyo. Like I have said before..."
						dialog "1,200,000 zeny."
						dialog "Have you misplaced it?"
						dialog "Look through your pockets carefully and come to me again."
						close
					elseif ((v[VAR_SEX] == 0) & (v[Wedding_Dress] < 1))
						dialog "[Marry Happy]"
						dialog "Aiyo, have you misplaced"
						dialog "the wedding dress?"
						dialog "Look for it carefully and come again."
						dialog "You'll need to wear it at the ceremony..."
						dialog "So give it to me directly if you're wearing it."
						close
					elseif ((v[VAR_SEX] == 1) & (v[VAR_MONEY] < 1300000))
						dialog "[Marry Happy]"
						dialog "Aiyo aiyo. Like I have said before..."
						dialog "1,300,000 zeny."
						dialog "Have you misplaced it?"
						dialog "Look through your pockets carefully and come to me again."
						close
					elseif ((v[VAR_SEX] == 1) & (v[Tuxedo] < 1))
						dialog "[Marry Happy]"
						dialog "Aiyo, have you misplaced"
						dialog "the tuxedo?"
						dialog "Look for it carefully and come again."
						dialog "You'll need to wear it at the ceremony..."
						dialog "So give it to me directly if you're wearing it."
						close
					else
						dialog "[Marry Happy]"
						dialog "Congratulations!"
						dialog "I'm not sure who your partner is, but you must be very happy."
						wait
						dialog "[Marry Happy]"
						dialog "Let's begin the application process."
						dialog "Now please write down your name here."
						wait 
						while(1)
							var name = PcName
							dlgwritestr
							if inputstr != name
								dialog "[Marry Happy]" 
								dialog "Aiyo, you don't even know how to write a name?"
								dialog "Do you still want to get married?"
								dialog "Learn to write the name before you come back here."
								dialog "It might be better if you write faster."
								dialog "Please write again."
								wait
							else
								exitwhile
							endif
						endwhile
						dialog "[Marry Happy]"
						dialog "Yes, we've received the complete application form."
						dialog "Please use this method too "
						dialog "when you mention the name of your partner to His Majesty."
						dialog "You should know the exact name of your partner."
						dialog "Do you understand?"
						dialog "Get prepared before you go."
						wait
						dialog "[Marry Happy]"
						dialog "Then, Miss " + name + " ....."
						dialog "Let me ask again... "
						dialog "Has your partner completed the application?"
						dialog "If you've completed the application,"
						dialog "proceed to meet the King and start the wedding ceremony."
						dialog "Finally... Be happy, and let's hope that all well shall end well~"
						Emotion "Marry Happy" ET_THROB
						if v[VAR_SEX] == 0
							dropgold 1200000
							dropitem Wedding_Dress 1
						else
							dropgold 1300000
							dropitem Tuxedo 1
						endif
						dropitem Diamond_Ring 1
						setitem wedding_sign 1 
						close
					endif
				break
				case 2
					dialog "[Marry Happy]"
					dialog "I wish you a happy and memorable day...!"
					close
				break
			endchoose
		break 

		case 4
			cmdothernpc "Single Army#Prontera" "on"
			cmdothernpc "Single Army#Geffen" "on"
			cmdothernpc "Single Army#Morocc" "on"
			cmdothernpc "Single Army#Payon" "on"
			cmdothernpc "Single Army#Amatsu" "on"
			cmdothernpc "Single Army#Gonryun" "on"
			Emotion "Wedding Staff" ET_HUK
			dialog "[Single Army]"
			dialog "You have to refine things yourself to produce great items!"
			dialog "It is a waste to form a party in a dungeon!"
			dialog "I can become a heart-shaped NPC!"
			dialog "I train by myself from my creation to my job change!"
			dialog "During Christmas, I'll cut grass outside the house!"
			dialog "...We are the invincible Single Army!"
			close 
			Emotion "Wedding Staff" ET_SWEAT
			cmdothernpc "Single Army#Prontera" "off"
			cmdothernpc "Single Army#Geffen" "off"
			cmdothernpc "Single Army#Morocc" "off"
			cmdothernpc "Single Army#Payon" "off"
			cmdothernpc "Single Army#Amatsu" "off"
			cmdothernpc "Single Army#Gonryun" "off"
		break 
	endchoose 
	close

npc "prt_church" "Single Army#Prontera" 8W_SOLDIER 97 102 0 1 1
OnInit:
	disablenpc "Single Army#Prontera"
return
OnCommand: "off"
	disablenpc "Single Army#Prontera" 
return
OnCommand: "on"
	enablenpc "Single Army#Prontera"
	Emotion "Single Army#Prontera" ET_GO
return
OnClick:
	dialog "[Single Army]"
	dialog "You have to refine things yourself to produce great items!"
	close
	return


npc "prt_church" "Single Army#Geffen" 4_M_GEF_SOLDIER 98 102 0 1 1
OnInit:
	disablenpc "Single Army#Geffen"
return
OnCommand: "off"
	disablenpc "Single Army#Geffen" 
return
OnCommand: "on"
	enablenpc "Single Army#Geffen"
	Emotion "Single Army#Geffen" ET_GO 
return
OnClick:
	dialog "[Single Army]"
	dialog "It is a waste to form a party in a dungeon!"
	close 
	return

npc "prt_church" "Single Army#Morocc" 4_M_MOC_SOLDIER 99 102 0 1 1
OnInit:
	disablenpc "Single Army#Morocc"
return
OnCommand: "off"
	disablenpc "Single Army#Morocc"
return 
OnCommand: "on"
	enablenpc "Single Army#Morocc"
	Emotion "Single Army#Morocc" ET_GO
return
OnClick:
	dialog "[Single Army]"
	dialog "I can become a heart-shaped NPC!"
	close
	return

npc "prt_church" "Single Army#Payon" 4_M_PAY_SOLDIER 100 102 0 1 1
OnInit:
	disablenpc "Single Army#Payon" 
return
OnCommand: "off"
	disablenpc "Single Army#Payon"
return
OnCommand: "on"
	enablenpc "Single Army#Payon" 
	Emotion "Single Army#Payon" ET_GO
return
OnClick:
	dialog "[Single Army]" 
	dialog "I train by myself from my creation to my job change!"
	close
	return

npc "prt_church" "Single Army#Amatsu" 8_M_JPNSOLDIER 101 102 0 1 1
OnInit: 
	disablenpc "Single Army#Amatsu"
return 
OnCommand: "off"
	disablenpc "Single Army#Amatsu"
return 
OnCommand: "on"
	enablenpc "Single Army#Amatsu"
	Emotion "Single Army#Amatsu" ET_GO
return
OnClick:
	dialog "[Single Army]"
	dialog "During Christmas, I'll cut grass outside the house!"
	close
	return 

npc "prt_church" "Single Army#Gonryun" 8_M_TWSOLDIER 102 102 0 1 1
OnInit:
	disablenpc "Single Army#Gonryun"
return
OnCommand: "off"
	disablenpc "Single Army#Gonryun"
return 
OnCommand: "on"
	enablenpc "Single Army#Gonryun"
	Emotion "Single Army#Gonryun" ET_GO
return
OnClick:
	dialog "[Single Army]" 
	dialog "...We are the invincible Single Army!"
	close
	return

npc "prt_church" "The king of Midgard" 1_M_PRON_KING 99 125 4 1 1
OnInit:
	strlocalvar 0 "wedding"
	SetLocalVar "wedding" 0 
	return
OnClick:
  var baby_w = IsBaby
  if (baby_w == 1)
    dialog "[Vomars]"
    dialog "You cannot marry yet... you are too young."
    close
    return
  endif
	if v[VAR_ISMARRIED] == 0
		if lv["wedding"] == 0
			if v[wedding_sign] == 1
				if v[VAR_CPARTY] == 2
					if v[VAR_SEX] == 1
						SetLocalVar "wedding" 1
						InitTimer
						dialog "[Tristan III]"
						dialog "The young lovers who seek happiness in future..."
						dialog "Please remember this moment forever."
						dialog "May your future be blessed with goodness and serenity forever."
						dialog "And may you two"
						dialog "stay together forever."
						wait
						var name = PcName
						broadcastinmap "This statement was from the groom, " + name + ",..."
						dialog "[Tristan III]"
						dialog "Till the end of the world..."
						dialog "I shall stay with you forever."
						dialog "Let me be the one to shine up the path of your life in future."
						dialog "May I know what is the name of the lady"
						dialog "who wants to get married?"
						wait
						dlgwritestr // 신부 이름 받음
						dialog "[Tristan III]"
						dialog "Bridegroom Mr " + name + ","
						dialog "Will you swear with your life that you will protect"
						dialog "your bride Miss " + inputstr + ","
						dialog "to care and value her forever?"
						wait
						choose menu "Yes"
					case 1
					break
						endchoose
						strlocalvar 2 inputstr // 2에 신부 이름 입력
						strlocalvar 1 PcName //1에 신랑 이름 입력
						dialog "[Tristan III]"
						dialog "Now, I would like to hear"
						dialog "the bride's opinion."
						close
						broadcastinmap "The bridegroom Mr "+ name +" swore to marry the bride, Miss "+ inputstr +"" "
					else
						dialog "[Tristan III]"
						dialog "It is the woman's turn..."
						close
					endif 
				else
					dialog "[Tristan III]"
					dialog "If you wanted to marry..."
					dialog "Then please form a party before coming here."
					close 
				endif
			else
				dialog "[Tristan III]"
				dialog "If you want to marry, you need to hand over the registration form beforehand."
				close
			endif 
		elseif lv["wedding"] == 1
			var name_1
			var name_2
			name_1 = GetLocalVarName 1
			name_2 = GetLocalVarName 2
			if v[wedding_sign] == 1
				if v[VAR_CPARTY] == 2
					if v[VAR_SEX] == 0
						if PcName == name_2
							dialog "[Tristan III]"
							dialog "The young lovers who seek happiness in future..."
							dialog "Please remember this moment forever."
							dialog "May your future be blessed with goodness and serenity forever."
							dialog "And may you two"
							dialog "stay together forever."
							wait
							broadcastinmap "Let us listen to what the woman, " + name_2 + ", wishes to say..."
							dialog "[Tristan III]"
							dialog "Bride, Miss " + name_2 + ","
							dialog "Will you swear with your life that you will protect"
							dialog "your groom, Mr " + name_1 + ","
							dialog "to care and value him forever?" 
							wait
							choose menu "Yes" "No"
								case 1
								var result = GetMarried
									if result == 1
										broadcastinmap "I hereby pronounce " + name_1 + " and Miss " + name_2 + " husband and wife."
										dialog "[Tristan III]"
										dialog "I, The leader of the Rune Midgard kingdom,"
										dialog "In the name of Tristan the III,"
										dialog "bless your future with happiness forever," 
										dialog "and hope for a good continuing and ending." 
										wait
										dialog "[Tristan III]"
										dialog "And... finally..."
										dialog "live happily ever after with  " + name_2 + "..."
										strlocalvar 1 0
										strlocalvar 2 0
										SetLocalVar "wedding" 0
										close 
										stoptimer
									else
									endif
								break
								case 2
									broadcastinmap "The bride, Miss " + name_2 + " has refused the invitation to marry the groom, Mr " + name_1 + "."
									dialog "[Tristan III]"
									dialog "It seems that you did not yet sever him."
									dialog "Possibly you needed to consider him as a husband"
									dialog "before you thought about marrying." 
									strlocalvar 1 0
									strlocalvar 2 0
									SetLocalVar "wedding" 0
									close
									stoptimer
								break
							endchoose
						else 
							if ((name_1 != 0) & (name_2 != 0))
								dialog "[Tristan III]"
								dialog "This is a celebration of the marriage of  "
								dialog "the bride, Miss "+ name_2 +";"
								dialog "do not interrupt."
								close
							else
								dialog "[Tristan III]"
								dialog "The celebration of another couple is currently in progress."
								dialog "Please have patience and wait for your turn."
								close 
							endif
						endif
					else
						if ((name_1 != 0) & (name_2 != 0))
							dialog "[Tristan III]"
							dialog "This is a celebration of the marriage of  "
							dialog "the bride, Miss "+ name_2 +","
							dialog "and the groom Mr. "+ name_1 + ";"
							dialog "do not interrupt."
							close
						else
							dialog "[Tristan III]"
							dialog "The celebration of another couple is currently in progress."
							dialog "Please have patience and wait for your turn."
							close
						endif
					endif
				else 
					if ((name_1 != 0) & (name_2 != 0))
						dialog "[Tristan III]"
						dialog "This is a celebration of the marriage of  "
						dialog "the bride, Miss "+ name_2 +","
						dialog "and the groom Mr. "+ name_1 + ";"
						dialog "do not interrupt."
						close
					else
						dialog "[Tristan III]"
						dialog "The celebration of another couple is currently in progress."
						dialog "Please have patience and wait for your turn."
						close
					endif
				endif
			else 
				if PcName == name_2
					dialog "[Tristan III]"
					dialog "Ahh.. You have still not filled out the registration form."
					dialog "Please do so if you wish to marry."
					close
					return
				endif
				if ((name_1 != 0) & (name_2 != 0)) 
					dialog "[Tristan III]"
					dialog "This is a celebration of the marriage of  "
					dialog "the bride, Miss "+ name_2 +","
					dialog "and the groom Mr. "+ name_1 + ";"
					dialog "do not interrupt.""
					close
				else
					dialog "[Tristan III]"
					dialog "Please hand over the registration form if you wish to marry."
					close
				endif 
			endif
		endif
	else 
		dialog "[Tristan III]"
		dialog "I hope you'll live happy together forever."
		close
	endif
	return

OnCommand: "stop"
	stoptimer
return
OnCommand: "reset"
	strlocalvar 1 0

	strlocalvar 2 0
	SetLocalVar "wedding" 0 
return
OnTimer: 180000
	broadcastinmap "You've waited too long for the answer... Please let the other couple come forward."
	strlocalvar 1 0
	strlocalvar 2 0
	SetLocalVar "wedding" 0
	stoptimer
return

npc "prt_church" "The king of Midgard#Switch" 1_M_PRON_KING 28 178 4 1 1
OnClick:
		dialog "[Tristan III]"
		dialog "Who are you...?"
		wait 
		dlgwrite 0 4000
		if error
			dialog "[Tristan III]"
			dialog "Don't play here."
			close
			moveto "prt_church" 101 102
			return
		elseif input == 0
			dialog "[Tristan III]"
			dialog "Don't play here."
			close
			moveto "prt_church" 101 102
			return 
		elseif input == 1854
			dialog "[Wedding Switch]"
			dialog "If youre having a problem with the marriage celebration,"
			dialog "then please try again."
			wait	
			choose menu "No, thanks" "Try again"
				case 1
					dialog "[Wedding Switch]"
					dialog "The main security,"
					dialog "The marriage celebration ought to go smooth."
					close
				break 
				case 2
					cmdothernpc "The king of Midgard" "stop"
					cmdothernpc "The king of Midgard" "reset"
					broadcastinmap "You have waited too long... Let the other couple come through."
					dialog "[Wedding Switch]"
					dialog "Tristan III... Reactivate!!"
					dialog "Let us begin this marriage ceremony."
					close
				break
			endchoose 
		else
			dialog "[Tristan III]"
			dialog "Don't play here."
			close 
			moveto "prt_church" 101 102
			return
		endif
	return 

npc "prt_church" "Divorce Staff" 1_F_LIBRARYGIRL 20 179 4 1 1
OnClick:
		dialog "[Bad Ending]" 
		dialog "Aiyo, what happened?"
		wait
		dlgwrite 0 4000
		if error
			dialog "[Bad Ending]"
			dialog "A mistake~"
			close
			return
		elseif input == 0 
			dialog "[Bad Ending]"
			dialog "A mistake~"
			close 
			return
		elseif input == 1854
			dialog "[Bad Ending]"
			dialog "Nobody can get rid of the wedding ring."
			dialog "Do you want me to help you do that?"
			wait	
			choose menu "Get rid of it." "Keep it."
				case 1
					if v[VAR_SEX] == 0 // 여성
						if v[Bride_Ring] > 0
							dropitem Bride_Ring 1
							dialog "[Bad Endding]"
							dialog "It is done~"
							dialog "Please take it off if you're wearing it."
							close
						else
							dialog "[Bad Endding]"
							dialog "The wedding ring is nowhere to be found-"
							dialog "Please take it off if you're wearing it."
							close
						endif
					else
						if v[Bridegroom_Ring] > 0
							dropitem Bridegroom_Ring 1
							dialog "[Bad Endding]"
							dialog "It is done~"
							dialog "Please take it off if you're wearing it."
							close
						else
							dialog "[Bad Endding]"
							dialog "The wedding ring is nowhere to be found-"
							dialog "Please take it off if you're wearing it."
							close
						endif
					endif
				break
				case 2
					dialog "[Bad Endding]"
					dialog "Marriage is the end, the graveyard."
					dialog "I shall get rid of it. Trust me."
					close
				break
			endchoose
		else
			dialog "[Bad Endding]"
			dialog "A mistake~"
			close
		endif
	return

npc "prt_church" "Remarry Staff" 1_F_LIBRARYGIRL 22 179 4 1 1
OnClick:
		dialog "[Wedding Again]"
		dialog "Hey, what's up?"
		wait
		dlgwrite 0 4000
		if error
			dialog "[Wedding Again]"
			dialog "A mistake~"
			close
			return
		elseif input == 0
			dialog "[Wedding Again]"
			dialog "A mistake~"
			close 
			return
		elseif input == 1854
			if v[VAR_ISMARRIED] == 0
				dialog "[Wedding Again]"
				dialog "How can you have the wedding ring when you haven't married yet?"
				dialog "Please come again after you have a groom!"
				close
				return
			endif
			var ring
			dialog "[Wedding Again]"
			dialog "If you've lost the wedding ring, you'll necessarily have to tell me that."
			dialog "I'll return it once more."
			wait
			choose menu "Give me one once more." "I am still keeping it."
				case 1
					if v[VAR_SEX] == 0
						var ring = GetEquipCount Bride_Ring
						if ((v[Bride_Ring] > 0) | (ring == 1))
							dialog "[Wedding Again]"
							dialog "Did you wear a ring?"
							dialog "You do not need more than one ring."
							close
						else
							getitem Bride_Ring 1
							dialog "[Wedding Again]"
							dialog "Here you have a new ring-"
							dialog "Please don't lose it again."
							close
						endif
					else
						var ring = GetEquipCount Bridegroom_Ring
						if ((v[Bridegroom_Ring] > 0) | (ring == 1))
							dialog "[Wedding Again]]"
							dialog "Did you wear a ring?"
							dialog "You do not need more than one ring."
							close
						else
							getitem Bridegroom_Ring 1
							dialog "[Wedding Again]]"
							dialog "Here you have a new ring-"
							dialog "Please don't lose it again."
							close 
						endif 
					endif
				break
				case 2
					dialog "[Wedding Again]]"
					dialog "That you're in fact still married, keep it well."
					dialog "If you have any problems than come visit me."
					close
				break
			endchoose
		else
			dialog "[Wedding Again]]"
			dialog "A mistake~"
			close
		endif
	return

