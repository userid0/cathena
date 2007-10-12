#
# Table structure for table `mob_db2`
#

DROP TABLE IF EXISTS `mob_db2`;
CREATE TABLE `mob_db2` (
  `ID` mediumint(9) unsigned NOT NULL default '0',
  `Sprite` text NOT NULL,
  `kName` text NOT NULL,
  `iName` text NOT NULL,
  `LV` tinyint(6) unsigned NOT NULL default '0',
  `HP` int(9) unsigned NOT NULL default '0',
  `SP` mediumint(9) unsigned NOT NULL default '0',
  `EXP` mediumint(9) unsigned NOT NULL default '0',
  `JEXP` mediumint(9) unsigned NOT NULL default '0',
  `Range1` tinyint(4) unsigned NOT NULL default '0',
  `ATK1` smallint(6) unsigned NOT NULL default '0',
  `ATK2` smallint(6) unsigned NOT NULL default '0',
  `DEF` smallint(6) unsigned NOT NULL default '0',
  `MDEF` smallint(6) unsigned NOT NULL default '0',
  `STR` tinyint(4) unsigned NOT NULL default '0',
  `AGI` tinyint(4) unsigned NOT NULL default '0',
  `VIT` tinyint(4) unsigned NOT NULL default '0',
  `INT` tinyint(4) unsigned NOT NULL default '0',
  `DEX` tinyint(4) unsigned NOT NULL default '0',
  `LUK` tinyint(4) unsigned NOT NULL default '0',
  `Range2` tinyint(4) unsigned NOT NULL default '0',
  `Range3` tinyint(4) unsigned NOT NULL default '0',
  `Scale` tinyint(4) unsigned NOT NULL default '0',
  `Race` tinyint(4) unsigned NOT NULL default '0',
  `Element` tinyint(4) unsigned NOT NULL default '0',
  `Mode` smallint(6) unsigned NOT NULL default '0',
  `Speed` smallint(6) unsigned NOT NULL default '0',
  `aDelay` smallint(6) unsigned NOT NULL default '0',
  `aMotion` smallint(6) unsigned NOT NULL default '0',
  `dMotion` smallint(6) unsigned NOT NULL default '0',
  `MEXP` mediumint(9) unsigned NOT NULL default '0',
  `ExpPer` smallint(9) unsigned NOT NULL default '0',
  `MVP1id` smallint(9) unsigned NOT NULL default '0',
  `MVP1per` smallint(9) unsigned NOT NULL default '0',
  `MVP2id` smallint(9) unsigned NOT NULL default '0',
  `MVP2per` smallint(9) unsigned NOT NULL default '0',
  `MVP3id` smallint(9) unsigned NOT NULL default '0',
  `MVP3per` smallint(9) unsigned NOT NULL default '0',
  `Drop1id` smallint(9) unsigned NOT NULL default '0',
  `Drop1per` smallint(9) unsigned NOT NULL default '0',
  `Drop2id` smallint(9) unsigned NOT NULL default '0',
  `Drop2per` smallint(9) unsigned NOT NULL default '0',
  `Drop3id` smallint(9) unsigned NOT NULL default '0',
  `Drop3per` smallint(9) unsigned NOT NULL default '0',
  `Drop4id` smallint(9) unsigned NOT NULL default '0',
  `Drop4per` smallint(9) unsigned NOT NULL default '0',
  `Drop5id` smallint(9) unsigned NOT NULL default '0',
  `Drop5per` smallint(9) unsigned NOT NULL default '0',
  `Drop6id` smallint(9) unsigned NOT NULL default '0',
  `Drop6per` smallint(9) unsigned NOT NULL default '0',
  `Drop7id` smallint(9) unsigned NOT NULL default '0',
  `Drop7per` smallint(9) unsigned NOT NULL default '0',
  `Drop8id` smallint(9) unsigned NOT NULL default '0',
  `Drop8per` smallint(9) unsigned NOT NULL default '0',
  `Drop9id` smallint(9) unsigned NOT NULL default '0',
  `Drop9per` smallint(9) unsigned NOT NULL default '0',
  `DropCardid` smallint(9) unsigned NOT NULL default '0',
  `DropCardper` smallint(9) unsigned NOT NULL default '0',
  PRIMARY KEY  (`ID`)
) TYPE=MyISAM;

# // Monsters Additional Database
# //
# // Structure of Database :
# // ID,Sprite_Name,kROName,iROName,LV,HP,SP,EXP,JEXP,Range1,ATK1,ATK2,DEF,MDEF,STR,AGI,VIT,INT,DEX,LUK,Range2,Range3,Scale,Race,Element,Mode,Speed,aDelay,aMotion,dMotion,MEXP,ExpPer,MVP1id,MVP1per,MVP2id,MVP2per,MVP3id,MVP3per,Drop1id,Drop1per,Drop2id,Drop2per,Drop3id,Drop3per,Drop4id,Drop4per,Drop5id,Drop5per,Drop6id,Drop6per,Drop7id,Drop7per,Drop8id,Drop8per,Drop9id,Drop9per,DropCardid,DropCardper
# // Easter Event Monsters
# //1920,EASTER_EGG,Easter Egg,Easter Egg,3,300,0,4,4,0,1,2,20,20,1,1,1,1,1,20,10,12,0,0,60,128,1000,1001,1,1,0,0,0,0,0,0,0,0,1010,250,935,500,558,300,501,200,501,200,713,800,558,300,558,300,0,0,0,0
# //1921,EASTER_BUNNY,Easter Bunny,Easter Bunny,6,1800,0,60,55,1,20,26,0,40,1,36,6,1,11,80,10,10,0,2,60,181,200,1456,456,336,0,0,0,0,0,0,0,0,2250,200,515,8000,727,1200,746,1500,706,30,622,50,534,5000,0,0,0,0,4006,70
# // eAthena Dev Team
# //1900,VALARIS,Valaris,Valaris,99,668000,0,107250,37895,2,3220,4040,35,45,1,152,96,85,120,95,10,10,2,6,67,1973,100,1068,768,576,13000,5000,608,1000,750,400,923,3800,1466,200,2256,200,2607,800,714,500,617,3000,984,4300,985,5600,0,0,0,0,4147,1
# //1901,VALARIS_WORSHIPPER,Valaris's Worshipper,Valaris's Worshipper,50,8578,0,2706,1480,1,487,590,15,25,1,75,55,1,93,45,10,12,0,6,27,1685,100,868,480,120,0,0,0,0,0,0,0,0,923,500,984,63,1464,2,607,50,610,100,503,300,2405,50,0,0,0,0,4129,1
# //1902,MC_CAMERI,MC Cameri,MC Cameri,99,668000,0,107250,37895,2,3220,4040,35,45,1,152,96,85,120,95,10,10,2,6,67,1973,100,1068,768,576,13000,5000,608,1000,750,400,923,3800,1466,200,2256,200,2607,800,714,500,617,3000,984,4300,985,5600,0,0,0,0,4147,1
# //1903,POKI,Poki#3,Poki#3,99,1349000,0,4093000,1526000,9,4892,9113,22,35,1,180,39,67,193,130,10,12,1,7,64,1973,120,500,672,480,92100,7000,603,5500,617,3000,1723,1000,1228,100,1236,500,617,2500,1234,75,1237,125,1722,250,1724,100,1720,50,0,0,0,0
# //1904,SENTRY,Sentry,Sentry,99,668000,0,107250,37895,2,3220,4040,35,45,1,152,96,85,120,95,10,10,2,6,67,1973,100,1068,768,576,13000,5000,608,1000,750,400,923,3800,1466,200,2256,200,2607,800,714,500,617,3000,984,4300,985,5600,0,0,0,0,4147,1
# //Custom Hollow Poring (overrrides/collides with META_ANDRE)
# //1237,HOLLOW_PORING,Hollow Poring,Hollow Poring,1,50,0,2,1,1,7,10,0,5,1,1,1,0,6,30,10,12,1,3,21,0x83,400,1872,672,480,0,0,0,0,0,0,0,0,909,7000,1202,100,938,400,512,1000,713,1500,512,150,619,20,0,0,0,0,4001,10
# //Custom Fire Poring. Warning, Colides with META_DENIRO
# //1239,FIRE_PORING,Fire Poring,Fire Poring,1,50,0,2,1,1,7,10,0,5,1,1,1,1,6,30,10,12,1,3,21,131,400,1872,672,480,0,0,0,0,0,0,0,0,909,7000,1202,100,938,400,512,1000,713,1500,741,5,619,20,0,0,0,0,4001,20
