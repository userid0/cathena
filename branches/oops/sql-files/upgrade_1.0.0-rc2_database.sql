# Upgrade database tables for eAthena 1.0 RC2

ALTER TABLE `item_db` MODIFY `name_english` varchar(28); 
ALTER TABLE `item_db` MODIFY `name_japanese` varchar(28);
