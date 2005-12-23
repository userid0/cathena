// $Id: battle.h,v 1.6 2004/09/29 21:08:17 Akitasha Exp $
#ifndef _BATTLE_H_
#define _BATTLE_H_

// �_���[�W
struct Damage {
	int damage;
	int damage2;
	int type;
	int div_;
	int amotion;
	int dmotion;
	int blewcount;
	int flag;
	int dmg_lv;	//�͂܂ꌸ�Z�v�Z�p�@0:�X�L���U�� ATK_LUCKY,ATK_FLEE,ATK_DEF
};

// �����\�i�ǂݍ��݂�pc.c�Abattle_attr_fix�Ŏg�p�j
extern int attr_fix_table[4][10][10];

struct map_session_data;
struct mob_data;
struct block_list;

// �_���[�W�v�Z

struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_weapon_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);

// �����C���v�Z
int battle_attr_fix(int damage,int atk_elem,int def_elem);

// �_���[�W�ŏI�v�Z
int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,short skill_lv,int flag);
enum {	// �ŏI�v�Z�̃t���O
	BF_WEAPON	= 0x0001,
	BF_MAGIC	= 0x0002,
	BF_MISC		= 0x0004,
	BF_SHORT	= 0x0010,
	BF_LONG		= 0x0040,
	BF_SKILL	= 0x0100,
	BF_NORMAL	= 0x0200,
	BF_WEAPONMASK=0x000f,
	BF_RANGEMASK= 0x00f0,
	BF_SKILLMASK= 0x0f00,
};

// ���ۂ�HP�𑝌�
int battle_delay_damage(unsigned long tick, struct block_list &src, struct block_list &target, int damage, int flag);
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag);
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag);

// �U����ړ����~�߂�
int battle_stopattack(struct block_list *bl);
int battle_stopwalking(struct block_list *bl,int type);

// �ʏ�U�������܂Ƃ�
int battle_weapon_attack( struct block_list *bl,struct block_list *target,unsigned long tick,int flag);

// �e��p�����[�^�𓾂�
unsigned int battle_counttargeted(struct block_list &bl,struct block_list *src, unsigned short target_lv);
struct block_list* battle_gettargeted(struct block_list &target);


enum {
	BCT_NOONE	=0x000000,
	BCT_SELF	=0x010000,
	BCT_ENEMY	=0x020000,
	BCT_NOENEMY	=0x1d0000,//(~BCT_ENEMY&BCT_ALL)
	BCT_PARTY	=0x050000,//Party includes self (0x04|0x01)
	BCT_NOPARTY	=0x1b0000,//(~BCT_PARTY&BCT_ALL)
	BCT_GUILD	=0x090000,//Guild includes self (0x08|0x01)
	BCT_NOGUILD	=0x170000,//(~BCT_GUILD&BCT_ALL)
	BCT_ALL		=0x1f0000,
	BCT_NEUTRAL =0x100000
};


bool battle_check_undead(int race,int element);
int battle_check_target(struct block_list *src, struct block_list *target,int flag);
bool battle_check_range(struct block_list *src,struct block_list *bl,unsigned int range);


// �ݒ�
struct Battle_Config
{
	uint32 agi_penalty_count;
	uint32 agi_penalty_count_lv;
	uint32 agi_penalty_num;
	uint32 agi_penalty_type;
	uint32 alchemist_summon_reward;
	uint32 allow_atcommand_when_mute; // [celest]
	uint32 any_warp_GM_min_level; // added by [Yor]
	uint32 area_size; // added by [MouseJstr]
	uint32 arrow_decrement;
	uint32 atc_gmonly;
	uint32 atc_spawn_quantity_limit;
	uint32 attr_recover;
	uint32 backstab_bow_penalty;
	uint32 ban_bot;
	uint32 ban_hack_trade; // added by [Yor]
	uint32 ban_spoof_namer; // added by [Yor]
	uint32 base_exp_rate;
	uint32 basic_skill_check;
	uint32 battle_log;
	uint32 berserk_cancels_buffs;
	uint32 bone_drop;
	uint32 boss_spawn_delay;
	uint32 buyer_name;
	uint32 cardillust_read_grffile;
	uint32 cast_rate;
	uint32 castle_defense_rate;
	uint32 castrate_dex_scale; // added by [MouseJstr]
	uint32 character_size; // if riders have size=2, and baby class riders size=1 [Lupus]
	uint32 chat_warpportal;
	uint32 combo_delay_rate;
	uint32 copyskill_restrict;
	uint32 day_duration; // added by [Yor]
	uint32 dead_branch_active;
	uint32 death_penalty_base;
	uint32 death_penalty_job;
	uint32 death_penalty_type;
	uint32 defnotenemy;
	uint32 delay_battle_damage;
	uint32 delay_dependon_dex;
	uint32 delay_rate;
	uint32 devotion_level_difference;
	uint32 disp_experience;
	uint32 disp_hpmeter;
	uint32 display_delay_skill_fail;
	uint32 display_hallucination;
	uint32 display_snatcher_skill_fail;
	uint32 display_version;
	uint32 drop_rate0item;
	uint32 drop_rare_announce;
	uint32 drops_by_luk;
	uint32 dynamic_mobs;
	uint32 enemy_critical;
	uint32 enemy_critical_rate;
	uint32 enemy_perfect_flee;
	uint32 enemy_str;
	uint32 equip_natural_break_rate;
	uint32 equip_self_break_rate;
	uint32 equip_skill_break_rate;
	uint32 error_log;
	uint32 etc_log;
	uint32 exp_calc_type;
	uint32 finding_ore_rate; // orn
	uint32 finger_offensive_type;
	uint32 flooritem_lifetime;
	uint32 gm_allequip;
	uint32 gm_allskill;
	uint32 gm_allskill_addabra;
	uint32 gm_can_drop_lv;
	uint32 gm_join_chat;
	uint32 gm_kick_chat;
	uint32 gm_skilluncond;
	uint32 gtb_pvp_only;  // [MouseJstr]
	uint32 guild_emperium_check;
	uint32 guild_exp_limit;
	uint32 guild_max_castles;
	uint32 gvg_eliminate_time;
	uint32 gvg_long_damage_rate;
	uint32 gvg_magic_damage_rate;
	uint32 gvg_misc_damage_rate;
	uint32 gvg_short_damage_rate;
	uint32 gvg_weapon_damage_rate;
	uint32 gx_allhit;
	uint32 gx_cardfix;
	uint32 gx_disptype;
	uint32 gx_dupele;
	uint32 hack_info_GM_level; // added by [Yor]
	uint32 headset_block_music; // do headsets block Frost Joke, etc [Lupus]
	uint32 heal_exp;
	uint32 hide_GM_session;
	uint32 holywater_name_input;
	uint32 hp_rate;
	uint32 idle_no_share;
	uint32 ignore_items_gender; //[Lupus]
	uint32 indoors_override_grffile;
	uint32 invite_request_check;
	uint32 item_auto_get;
	uint32 item_check;
	uint32 item_drop_card_max;
	uint32 item_drop_card_min;
	uint32 item_drop_common_max;
	uint32 item_drop_common_min;
	uint32 item_drop_equip_max;
	uint32 item_drop_equip_min;
	uint32 item_drop_heal_max;
	uint32 item_drop_heal_min;
	uint32 item_drop_mvp_max;
	uint32 item_drop_mvp_min;
	uint32 item_drop_use_max;
	uint32 item_drop_use_min;
	uint32 item_equip_override_grffile;
	uint32 item_first_get_time;
	uint32 item_name_override_grffile;
	uint32 item_rate_card;
	uint32 item_rate_common;
	uint32 item_rate_equip;
	uint32 item_rate_heal;
	uint32 item_rate_use;
	uint32 item_second_get_time;
	uint32 item_slots_override_grffile;
	uint32 item_third_get_time;
	uint32 item_use_interval;
	uint32 job_exp_rate;
	uint32 left_cardfix_to_right;
	uint32 magic_defense_type;
	uint32 making_arrow_name_input;
	uint32 max_adv_level;
	uint32 max_aspd;
	uint32 max_aspd_interval; // not writable
	uint32 max_base_level;
	uint32 max_cart_weight;
	uint32 max_cloth_color; // added by [MouseJstr]
	uint32 max_hair_color; // added by [MouseJstr]
	uint32 max_hair_style; // added by [MouseJstr]
	uint32 max_hitrate;
	uint32 max_hp;
	uint32 max_job_level;
	uint32 max_parameter;
	uint32 max_sn_level;
	uint32 max_sp;
	uint32 max_walk_speed;
	uint32 maximum_level;
	uint32 min_cloth_color; // added by [MouseJstr]
	uint32 min_hair_color; // added by [MouseJstr]
	uint32 min_hair_style; // added by [MouseJstr]
	uint32 min_hitrate;
	uint32 min_skill_delay_limit;
	uint32 mob_attack_attr_none;
	uint32 mob_changetarget_byskill;
	uint32 mob_clear_delay;
	uint32 mob_count_rate;
	uint32 mob_ghostring_fix;
	uint32 mob_remove_damaged;
	uint32 mob_remove_delay;
	uint32 mob_skill_add_range;
	uint32 mob_skill_delay;
	uint32 mob_skill_log;
	uint32 mob_skill_rate;
	uint32 mob_slaves_inherit_speed;
	uint32 mob_spawn_delay;
	uint32 mob_warpportal;
	uint32 mobs_level_up; // [Valaris]
	uint32 monster_active_enable;
	uint32 monster_attack_direction_change;
	uint32 monster_auto_counter_type;
	uint32 monster_class_change_full_recover;
	uint32 monster_cloak_check_type;
	uint32 monster_damage_delay;
	uint32 monster_damage_delay_rate;
	uint32 monster_defense_type;
	uint32 monster_hp_rate;
	uint32 monster_land_skill_limit;
	uint32 monster_loot_type;
	uint32 monster_max_aspd;
	uint32 monster_max_aspd_interval;// not writable, 
	uint32 monster_skill_nofootset;
	uint32 monster_skill_reiteration;
	uint32 monsters_ignore_gm;
	uint32 motd_type; // [celest]
	uint32 multi_level_up;
	uint32 muting_players; // added by [PoW]
	uint32 mvp_exp_rate;
	uint32 mvp_hp_rate;
	uint32 mvp_item_first_get_time;
	uint32 mvp_item_rate;
	uint32 mvp_item_second_get_time;
	uint32 mvp_item_third_get_time;
	uint32 natural_heal_skill_interval;
	uint32 natural_heal_weight_rate;
	uint32 natural_healhp_interval;
	uint32 natural_healsp_interval;
	uint32 new_attack_function; //For testing purposes [Skotlex]
	uint32 night_at_start; // added by [Yor]
	uint32 night_darkness_level; // [celest]
	uint32 night_duration; // added by [Yor]
	uint32 packet_ver_flag; // added by [Yor]
	uint32 party_bonus;
	uint32 party_share_mode;
	uint32 party_skill_penalty;
	uint32 pc_attack_attr_none;
	uint32 pc_attack_direction_change;
	uint32 pc_auto_counter_type;
	uint32 pc_cloak_check_type;
	uint32 pc_damage_delay;
	uint32 pc_damage_delay_rate;
	uint32 pc_invincible_time;
	uint32 pc_land_skill_limit;
	uint32 pc_skill_add_range;
	uint32 pc_skill_log;
	uint32 pc_skill_nofootset;
	uint32 pc_skill_reiteration;
	uint32 pet_attack_attr_none;
	uint32 pet_attack_exp_rate;
	uint32 pet_attack_exp_to_master;
	uint32 pet_attack_support;
	uint32 pet_catch_rate;
	uint32 pet_damage_support;
	uint32 pet_defense_type;
	uint32 pet_equip_required;
	uint32 pet_friendly_rate;
	uint32 pet_hair_style; // added by [Skotlex]
	uint32 pet_hungry_delay_rate;
	uint32 pet_hungry_friendly_decrease;
	uint32 pet_lv_rate; //[Skotlex]
	uint32 pet_max_atk1; //[Skotlex]
	uint32 pet_max_atk2; //[Skotlex]
	uint32 pet_max_stats; //[Skotlex]
	uint32 pet_no_gvg; //Disables pets in gvg. [Skotlex]
	uint32 pet_random_move;
	uint32 pet_rename;
	uint32 pet_status_support;
	uint32 pet_str;
	uint32 pet_support_min_friendly;
	uint32 pet_support_rate;
	uint32 pk_min_level; // [celest]
	uint32 pk_mode;
	uint32 plant_spawn_delay;
	uint32 player_defense_type;
	uint32 player_skill_partner_check;
	uint32 pp_rate;
	uint32 prevent_logout;
	uint32 produce_item_name_input;
	uint32 produce_potion_name_input;
	uint32 pvp_exp;  // [MouseJstr]
	uint32 quest_skill_learn;
	uint32 quest_skill_reset;
	uint32 rainy_waterball;
	uint32 random_monster_checklv;
	uint32 require_glory_guild;
	uint32 restart_hp_rate;
	uint32 restart_sp_rate;
	uint32 resurrection_exp;
	uint32 save_clothcolor;
	uint32 save_log;
	uint32 sdelay_attack_enable;
	uint32 shop_exp;
	uint32 show_hp_sp_drain;
	uint32 show_hp_sp_gain;
	uint32 show_mob_hp;  // end additions [Valaris]
	uint32 show_steal_in_same_party;
	uint32 skill_min_damage;
	uint32 skill_out_range_consume;
	uint32 skill_removetrap_type;
	uint32 skill_sp_override_grffile;
	uint32 skill_steal_rate; // [celest]
	uint32 skill_steal_type; // [celest]
	uint32 skillfree;
	uint32 skillup_limit;
	uint32 sp_rate;
	uint32 undead_detect_type;
	uint32 unit_movement_type;
	uint32 use_statpoint_table;
	uint32 vending_max_value;
	uint32 vit_penalty_count;
	uint32 vit_penalty_count_lv;
	uint32 vit_penalty_num;
	uint32 vit_penalty_type;
	uint32 warp_point_debug;
	uint32 wedding_ignorepalette;
	uint32 wedding_modifydisplay;
	uint32 who_display_aid;
	uint32 wp_rate;
	uint32 zeny_from_mobs; // [Valaris]
	uint32 zeny_penalty;
};

extern struct Battle_Config battle_config;

int battle_config_read(const char *cfgName);
void battle_validate_conf();
void battle_set_defaults();
int battle_set_value(const char *w1, const char *w2);

#endif
