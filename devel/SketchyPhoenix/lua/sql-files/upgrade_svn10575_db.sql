ALTER TABLE `login` CHANGE `email` `email` VARCHAR( 39 ) NOT NULL;
ALTER TABLE `login` CHANGE `sex` `sex` ENUM( 'M', 'F', 'S' ) NOT NULL DEFAULT 'M';

ALTER TABLE `char` CHANGE `last_map` `last_map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `char` CHANGE `save_map` `save_map` VARCHAR( 11 ) NOT NULL;

ALTER TABLE `memo` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `memo` CHANGE `x` `x` SMALLINT( 4 ) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `memo` CHANGE `y` `y` SMALLINT( 4 ) UNSIGNED NOT NULL DEFAULT '0';

ALTER TABLE `party` CHANGE `name` `name` VARCHAR( 24 ) NOT NULL;
