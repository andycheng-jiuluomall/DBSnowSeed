#CREATE FUNCTION SnowSeed RETURNS INTEGER SONAME 'DBSnowSeed.MySQL.dll';
#DROP FUNCTION SnowSeed
#select 1 a, SnowSeed(0,0) b
#union
#select 2 a, SnowSeed(0,0) b