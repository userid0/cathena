-	script	dayNight	-1,{
	end;

OnInit:
	if(gettime(3) > 7 && gettime(3) < 19) {
		goto L_Day;
	} else {
		goto L_Night;
	}

L_Day:
	day;
	callfunc "RcloudFlag";
	end;

L_Night:
	night;
	callfunc "cloudFlag";
	end;

OnClock1900:
	goto L_Night;

OnClock0700:
	goto L_Day;
}

function	script	cloudFlag	{
setmapflag "alb2trea.gat",mf_clouds;
setmapflag "alberta.gat",mf_clouds;
setmapflag "aldebaran.gat",mf_clouds;
setmapflag "gef_fild00.gat",mf_clouds;
setmapflag "gef_fild01.gat",mf_clouds;
setmapflag "gef_fild02.gat",mf_clouds;
setmapflag "gef_fild03.gat",mf_clouds;
setmapflag "gef_fild04.gat",mf_clouds;
setmapflag "gef_fild05.gat",mf_clouds;
setmapflag "gef_fild06.gat",mf_clouds;
setmapflag "gef_fild07.gat",mf_clouds;
setmapflag "gef_fild08.gat",mf_clouds;
setmapflag "gef_fild09.gat",mf_clouds;
setmapflag "gef_fild10.gat",mf_clouds;
setmapflag "gef_fild11.gat",mf_clouds;
setmapflag "geffen.gat",mf_clouds;
setmapflag "gl_church.gat",mf_clouds;
setmapflag "gl_chyard.gat",mf_clouds;
setmapflag "gl_knt01.gat",mf_clouds;
setmapflag "gl_knt02.gat",mf_clouds;
setmapflag "gl_step.gat",mf_clouds;
setmapflag "glast_01.gat",mf_clouds;
setmapflag "hunter_1-1.gat",mf_clouds;
setmapflag "hunter_2-1.gat",mf_clouds;
setmapflag "hunter_3-1.gat",mf_clouds;
setmapflag "izlude.gat",mf_clouds;
setmapflag "job_thief1.gat",mf_clouds;
setmapflag "knight_1-1.gat",mf_clouds;
setmapflag "knight_2-1.gat",mf_clouds;
setmapflag "knight_3-1.gat",mf_clouds;
setmapflag "mjolnir_01.gat",mf_clouds;
setmapflag "mjolnir_02.gat",mf_clouds;
setmapflag "mjolnir_03.gat",mf_clouds;
setmapflag "mjolnir_04.gat",mf_clouds;
setmapflag "mjolnir_05.gat",mf_clouds;
setmapflag "mjolnir_06.gat",mf_clouds;
setmapflag "mjolnir_07.gat",mf_clouds;
setmapflag "mjolnir_08.gat",mf_clouds;
setmapflag "mjolnir_09.gat",mf_clouds;
setmapflag "mjolnir_10.gat",mf_clouds;
setmapflag "mjolnir_11.gat",mf_clouds;
setmapflag "mjolnir_12.gat",mf_clouds;
setmapflag "moc_fild01.gat",mf_clouds;
setmapflag "moc_fild02.gat",mf_clouds;
setmapflag "moc_fild03.gat",mf_clouds;
setmapflag "moc_fild04.gat",mf_clouds;
setmapflag "moc_fild05.gat",mf_clouds;
setmapflag "moc_fild06.gat",mf_clouds;
setmapflag "moc_fild07.gat",mf_clouds;
setmapflag "moc_fild08.gat",mf_clouds;
setmapflag "moc_fild09.gat",mf_clouds;
setmapflag "moc_fild10.gat",mf_clouds;
setmapflag "moc_fild11.gat",mf_clouds;
setmapflag "moc_fild12.gat",mf_clouds;
setmapflag "moc_fild13.gat",mf_clouds;
setmapflag "moc_fild14.gat",mf_clouds;
setmapflag "moc_fild15.gat",mf_clouds;
setmapflag "moc_fild16.gat",mf_clouds;
setmapflag "moc_fild17.gat",mf_clouds;
setmapflag "moc_fild18.gat",mf_clouds;
setmapflag "moc_fild19.gat",mf_clouds;
setmapflag "moc_pryd01.gat",mf_clouds;
setmapflag "moc_pryd02.gat",mf_clouds;
setmapflag "moc_pryd03.gat",mf_clouds;
setmapflag "moc_pryd04.gat",mf_clouds;
setmapflag "moc_pryd05.gat",mf_clouds;
setmapflag "moc_pryd06.gat",mf_clouds;
setmapflag "moc_prydb1.gat",mf_clouds;
setmapflag "moc_ruins.gat",mf_clouds;
setmapflag "morocc.gat",mf_clouds;
setmapflag "new_1-1.gat",mf_clouds;
setmapflag "new_1-2.gat",mf_clouds;
setmapflag "new_1-3.gat",mf_clouds;
setmapflag "new_1-4.gat",mf_clouds;
setmapflag "pay_arche.gat",mf_clouds;
setmapflag "pay_fild01.gat",mf_clouds;
setmapflag "pay_fild02.gat",mf_clouds;
setmapflag "pay_fild03.gat",mf_clouds;
setmapflag "pay_fild04.gat",mf_clouds;
setmapflag "pay_fild05.gat",mf_clouds;
setmapflag "pay_fild06.gat",mf_clouds;
setmapflag "pay_fild07.gat",mf_clouds;
setmapflag "pay_fild08.gat",mf_clouds;
setmapflag "pay_fild09.gat",mf_clouds;
setmapflag "pay_fild10.gat",mf_clouds;
setmapflag "pay_fild11.gat",mf_clouds;
setmapflag "priest_1-1.gat",mf_clouds;
setmapflag "priest_2-1.gat",mf_clouds;
setmapflag "priest_3-1.gat",mf_clouds;
setmapflag "prontera.gat",mf_clouds;
setmapflag "prt_are01.gat",mf_clouds;
setmapflag "prt_fild00.gat",mf_clouds;
setmapflag "prt_fild01.gat",mf_clouds;
setmapflag "prt_fild02.gat",mf_clouds;
setmapflag "prt_fild03.gat",mf_clouds;
setmapflag "prt_fild04.gat",mf_clouds;
setmapflag "prt_fild05.gat",mf_clouds;
setmapflag "prt_fild06.gat",mf_clouds;
setmapflag "prt_fild07.gat",mf_clouds;
setmapflag "prt_fild08.gat",mf_clouds;
setmapflag "prt_fild09.gat",mf_clouds;
setmapflag "prt_fild10.gat",mf_clouds;
setmapflag "prt_fild11.gat",mf_clouds;
setmapflag "prt_maze01.gat",mf_clouds;
setmapflag "prt_maze02.gat",mf_clouds;
setmapflag "prt_maze03.gat",mf_clouds;
setmapflag "prt_monk.gat",mf_clouds;
setmapflag "cmd_fild01.gat",mf_clouds;
setmapflag "cmd_fild02.gat",mf_clouds;
setmapflag "cmd_fild03.gat",mf_clouds;
setmapflag "cmd_fild04.gat",mf_clouds;
setmapflag "cmd_fild05.gat",mf_clouds;
setmapflag "cmd_fild06.gat",mf_clouds;
setmapflag "cmd_fild07.gat",mf_clouds;
setmapflag "cmd_fild08.gat",mf_clouds;
setmapflag "cmd_fild09.gat",mf_clouds;
setmapflag "cmd_in01.gat",mf_clouds;
setmapflag "cmd_in02.gat",mf_clouds;
setmapflag "gef_fild12.gat",mf_clouds;
setmapflag "gef_fild13.gat",mf_clouds;
setmapflag "gef_fild14.gat",mf_clouds;
setmapflag "alde_gld.gat",mf_clouds;
setmapflag "pay_gld.gat",mf_clouds;
setmapflag "prt_gld.gat",mf_clouds;
setmapflag "alde_alche.gat",mf_clouds;
setmapflag "yuno.gat",mf_clouds;
setmapflag "yuno_fild01.gat",mf_clouds;
setmapflag "yuno_fild02.gat",mf_clouds;
setmapflag "yuno_fild03.gat",mf_clouds;
setmapflag "yuno_fild04.gat",mf_clouds;
setmapflag "ama_fild01.gat",mf_clouds;
setmapflag "ama_test.gat",mf_clouds;
setmapflag "amatsu.gat",mf_clouds;
setmapflag "gon_fild01.gat",mf_clouds;
setmapflag "gon_test.gat",mf_clouds;
setmapflag "gonryun.gat",mf_clouds;
setmapflag "umbala.gat",mf_clouds;
setmapflag "um_fild01.gat",mf_clouds;
setmapflag "um_fild02.gat",mf_clouds;
setmapflag "um_fild03.gat",mf_clouds;
setmapflag "um_fild04.gat",mf_clouds;
setmapflag "niflheim.gat",mf_clouds;
setmapflag "nif_fild01.gat",mf_clouds;
setmapflag "nif_fild02.gat",mf_clouds;
setmapflag "nif_in.gat",mf_clouds;
setmapflag "yggdrasil01.gat",mf_clouds;
setmapflag "valkyrie.gat",mf_clouds;
setmapflag "lou_fild01.gat",mf_clouds;
setmapflag "louyang.gat",mf_clouds;
setmapflag "nguild_gef.gat",mf_clouds;
setmapflag "nguild_prt.gat",mf_clouds;
setmapflag "nguild_pay.gat",mf_clouds;
setmapflag "nguild_alde.gat",mf_clouds;
setmapflag "jawaii.gat",mf_clouds;
setmapflag "jawaii_in.gat",mf_clouds;
setmapflag "gefenia01.gat",mf_clouds;
setmapflag "gefenia02.gat",mf_clouds;
setmapflag "gefenia03.gat",mf_clouds;
setmapflag "gefenia04.gat",mf_clouds;
setmapflag "payon.gat",mf_clouds;
setmapflag "ayothaya.gat",mf_clouds;
setmapflag "ayo_in01.gat",mf_clouds;
setmapflag "ayo_in02.gat",mf_clouds;
setmapflag "ayo_fild01.gat",mf_clouds;
setmapflag "ayo_fild02.gat",mf_clouds;
setmapflag "que_god01.gat",mf_clouds;
setmapflag "que_god02.gat",mf_clouds;
setmapflag "yuno_fild05.gat",mf_clouds;
setmapflag "yuno_fild07.gat",mf_clouds;
setmapflag "yuno_fild08.gat",mf_clouds;
setmapflag "yuno_fild09.gat",mf_clouds;
setmapflag "yuno_fild11.gat",mf_clouds;
setmapflag "yuno_fild12.gat",mf_clouds;
setmapflag "alde_tt02.gat",mf_clouds;
setmapflag "einbech.gat",mf_clouds;
setmapflag "einbroch.gat",mf_clouds;
setmapflag "ein_fild06.gat",mf_clouds;
setmapflag "ein_fild07.gat",mf_clouds;
setmapflag "ein_fild08.gat",mf_clouds;
setmapflag "ein_fild09.gat",mf_clouds;
setmapflag "ein_fild10.gat",mf_clouds;
setmapflag "que_sign01.gat",mf_clouds;
setmapflag "ein_fild03.gat",mf_clouds;
setmapflag "ein_fild04.gat",mf_clouds;
setmapflag "lhz_fild02.gat",mf_clouds;
setmapflag "lhz_fild03.gat",mf_clouds;
return;
}

function	script	RcloudFlag	{
removemapflag "alb2trea.gat",mf_clouds;
removemapflag "alberta.gat",mf_clouds;
removemapflag "aldebaran.gat",mf_clouds;
removemapflag "gef_fild00.gat",mf_clouds;
removemapflag "gef_fild01.gat",mf_clouds;
removemapflag "gef_fild02.gat",mf_clouds;
removemapflag "gef_fild03.gat",mf_clouds;
removemapflag "gef_fild04.gat",mf_clouds;
removemapflag "gef_fild05.gat",mf_clouds;
removemapflag "gef_fild06.gat",mf_clouds;
removemapflag "gef_fild07.gat",mf_clouds;
removemapflag "gef_fild08.gat",mf_clouds;
removemapflag "gef_fild09.gat",mf_clouds;
removemapflag "gef_fild10.gat",mf_clouds;
removemapflag "gef_fild11.gat",mf_clouds;
removemapflag "geffen.gat",mf_clouds;
removemapflag "gl_church.gat",mf_clouds;
removemapflag "gl_chyard.gat",mf_clouds;
removemapflag "gl_knt01.gat",mf_clouds;
removemapflag "gl_knt02.gat",mf_clouds;
removemapflag "gl_step.gat",mf_clouds;
removemapflag "glast_01.gat",mf_clouds;
removemapflag "hunter_1-1.gat",mf_clouds;
removemapflag "hunter_2-1.gat",mf_clouds;
removemapflag "hunter_3-1.gat",mf_clouds;
removemapflag "izlude.gat",mf_clouds;
removemapflag "mjolnir_01.gat",mf_clouds;
removemapflag "mjolnir_02.gat",mf_clouds;
removemapflag "mjolnir_03.gat",mf_clouds;
removemapflag "mjolnir_04.gat",mf_clouds;
removemapflag "mjolnir_05.gat",mf_clouds;
removemapflag "mjolnir_06.gat",mf_clouds;
removemapflag "mjolnir_07.gat",mf_clouds;
removemapflag "mjolnir_08.gat",mf_clouds;
removemapflag "mjolnir_09.gat",mf_clouds;
removemapflag "mjolnir_10.gat",mf_clouds;
removemapflag "mjolnir_11.gat",mf_clouds;
removemapflag "mjolnir_12.gat",mf_clouds;
removemapflag "moc_fild01.gat",mf_clouds;
removemapflag "moc_fild02.gat",mf_clouds;
removemapflag "moc_fild03.gat",mf_clouds;
removemapflag "moc_fild04.gat",mf_clouds;
removemapflag "moc_fild05.gat",mf_clouds;
removemapflag "moc_fild06.gat",mf_clouds;
removemapflag "moc_fild07.gat",mf_clouds;
removemapflag "moc_fild08.gat",mf_clouds;
removemapflag "moc_fild09.gat",mf_clouds;
removemapflag "moc_fild10.gat",mf_clouds;
removemapflag "moc_fild11.gat",mf_clouds;
removemapflag "moc_fild12.gat",mf_clouds;
removemapflag "moc_fild13.gat",mf_clouds;
removemapflag "moc_fild14.gat",mf_clouds;
removemapflag "moc_fild15.gat",mf_clouds;
removemapflag "moc_fild16.gat",mf_clouds;
removemapflag "moc_fild17.gat",mf_clouds;
removemapflag "moc_fild18.gat",mf_clouds;
removemapflag "moc_fild19.gat",mf_clouds;
removemapflag "moc_pryd01.gat",mf_clouds;
removemapflag "moc_pryd02.gat",mf_clouds;
removemapflag "moc_pryd03.gat",mf_clouds;
removemapflag "moc_pryd04.gat",mf_clouds;
removemapflag "moc_pryd05.gat",mf_clouds;
removemapflag "moc_pryd06.gat",mf_clouds;
removemapflag "moc_prydb1.gat",mf_clouds;
removemapflag "moc_ruins.gat",mf_clouds;
removemapflag "morocc.gat",mf_clouds;
removemapflag "new_1-1.gat",mf_clouds;
removemapflag "new_1-2.gat",mf_clouds;
removemapflag "new_1-3.gat",mf_clouds;
removemapflag "new_1-4.gat",mf_clouds;
removemapflag "pay_arche.gat",mf_clouds;
removemapflag "pay_fild01.gat",mf_clouds;
removemapflag "pay_fild02.gat",mf_clouds;
removemapflag "pay_fild03.gat",mf_clouds;
removemapflag "pay_fild04.gat",mf_clouds;
removemapflag "pay_fild05.gat",mf_clouds;
removemapflag "pay_fild06.gat",mf_clouds;
removemapflag "pay_fild07.gat",mf_clouds;
removemapflag "pay_fild08.gat",mf_clouds;
removemapflag "pay_fild09.gat",mf_clouds;
removemapflag "pay_fild10.gat",mf_clouds;
removemapflag "pay_fild11.gat",mf_clouds;
removemapflag "priest_1-1.gat",mf_clouds;
removemapflag "priest_2-1.gat",mf_clouds;
removemapflag "priest_3-1.gat",mf_clouds;
removemapflag "prontera.gat",mf_clouds;
removemapflag "prt_are01.gat",mf_clouds;
removemapflag "prt_fild00.gat",mf_clouds;
removemapflag "prt_fild01.gat",mf_clouds;
removemapflag "prt_fild02.gat",mf_clouds;
removemapflag "prt_fild03.gat",mf_clouds;
removemapflag "prt_fild04.gat",mf_clouds;
removemapflag "prt_fild05.gat",mf_clouds;
removemapflag "prt_fild06.gat",mf_clouds;
removemapflag "prt_fild07.gat",mf_clouds;
removemapflag "prt_fild08.gat",mf_clouds;
removemapflag "prt_fild09.gat",mf_clouds;
removemapflag "prt_fild10.gat",mf_clouds;
removemapflag "prt_fild11.gat",mf_clouds;
removemapflag "prt_maze01.gat",mf_clouds;
removemapflag "prt_maze02.gat",mf_clouds;
removemapflag "prt_maze03.gat",mf_clouds;
removemapflag "prt_monk.gat",mf_clouds;
removemapflag "cmd_fild01.gat",mf_clouds;
removemapflag "cmd_fild02.gat",mf_clouds;
removemapflag "cmd_fild03.gat",mf_clouds;
removemapflag "cmd_fild04.gat",mf_clouds;
removemapflag "cmd_fild05.gat",mf_clouds;
removemapflag "cmd_fild06.gat",mf_clouds;
removemapflag "cmd_fild07.gat",mf_clouds;
removemapflag "cmd_fild08.gat",mf_clouds;
removemapflag "cmd_fild09.gat",mf_clouds;
removemapflag "gef_fild12.gat",mf_clouds;
removemapflag "gef_fild13.gat",mf_clouds;
removemapflag "gef_fild14.gat",mf_clouds;
removemapflag "alde_gld.gat",mf_clouds;
removemapflag "pay_gld.gat",mf_clouds;
removemapflag "prt_gld.gat",mf_clouds;
removemapflag "alde_alche.gat",mf_clouds;
removemapflag "yuno.gat",mf_clouds;
removemapflag "yuno_fild01.gat",mf_clouds;
removemapflag "yuno_fild02.gat",mf_clouds;
removemapflag "yuno_fild03.gat",mf_clouds;
removemapflag "yuno_fild04.gat",mf_clouds;
removemapflag "ama_fild01.gat",mf_clouds;
removemapflag "ama_test.gat",mf_clouds;
removemapflag "amatsu.gat",mf_clouds;
removemapflag "gon_fild01.gat",mf_clouds;
removemapflag "gon_in.gat",mf_clouds;
removemapflag "gon_test.gat",mf_clouds;
removemapflag "gonryun.gat",mf_clouds;
removemapflag "umbala.gat",mf_clouds;
removemapflag "um_fild01.gat",mf_clouds;
removemapflag "um_fild02.gat",mf_clouds;
removemapflag "um_fild03.gat",mf_clouds;
removemapflag "um_fild04.gat",mf_clouds;
removemapflag "niflheim.gat",mf_clouds;
removemapflag "nif_fild01.gat",mf_clouds;
removemapflag "nif_fild02.gat",mf_clouds;
removemapflag "nif_in.gat",mf_clouds;
removemapflag "yggdrasil01.gat",mf_clouds;
removemapflag "valkyrie.gat",mf_clouds;
removemapflag "lou_fild01.gat",mf_clouds;
removemapflag "louyang.gat",mf_clouds;
removemapflag "nguild_gef.gat",mf_clouds;
removemapflag "nguild_prt.gat",mf_clouds;
removemapflag "nguild_pay.gat",mf_clouds;
removemapflag "nguild_alde.gat",mf_clouds;
removemapflag "jawaii.gat",mf_clouds;
removemapflag "jawaii_in.gat",mf_clouds;
removemapflag "gefenia01.gat",mf_clouds;
removemapflag "gefenia02.gat",mf_clouds;
removemapflag "gefenia03.gat",mf_clouds;
removemapflag "gefenia04.gat",mf_clouds;
removemapflag "payon.gat",mf_clouds;
removemapflag "ayothaya.gat",mf_clouds;
removemapflag "ayo_in01.gat",mf_clouds;
removemapflag "ayo_in02.gat",mf_clouds;
removemapflag "ayo_fild01.gat",mf_clouds;
removemapflag "ayo_fild02.gat",mf_clouds;
removemapflag "que_god01.gat",mf_clouds;
removemapflag "que_god02.gat",mf_clouds;
removemapflag "yuno_fild05.gat",mf_clouds;
removemapflag "yuno_fild07.gat",mf_clouds;
removemapflag "yuno_fild08.gat",mf_clouds;
removemapflag "yuno_fild09.gat",mf_clouds;
removemapflag "yuno_fild11.gat",mf_clouds;
removemapflag "yuno_fild12.gat",mf_clouds;
removemapflag "alde_tt02.gat",mf_clouds;
removemapflag "einbech.gat",mf_clouds;
removemapflag "einbroch.gat",mf_clouds;
removemapflag "ein_fild06.gat",mf_clouds;
removemapflag "ein_fild07.gat",mf_clouds;
removemapflag "ein_fild08.gat",mf_clouds;
removemapflag "ein_fild09.gat",mf_clouds;
removemapflag "ein_fild10.gat",mf_clouds;
removemapflag "que_sign01.gat",mf_clouds;
removemapflag "ein_fild03.gat",mf_clouds;
removemapflag "ein_fild04.gat",mf_clouds;
removemapflag "lhz_fild02.gat",mf_clouds;
removemapflag "lhz_fild03.gat",mf_clouds;
return;
}


