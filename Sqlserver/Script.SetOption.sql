/*
 预先部署脚本模板							
--------------------------------------------------------------------------------------
 此文件包含将在生成脚本之前执行的 SQL 语句。	
 使用 SQLCMD 语法将文件包含在预先部署脚本中。			
 示例:      :r .\myfile.sql								
 使用 SQLCMD 语法引用预先部署脚本中的变量。		
 示例:      :setvar TableName MyTable							
               SELECT * FROM [$(TableName)]					
--------------------------------------------------------------------------------------
*/
Use [$(DatabaseName)]

select * from sys.configurations where name='clr enabled';
go
sp_configure 'show advanced options',1
GO
RECONFIGURE
GO
sp_configure 'clr enabled',1
GO
RECONFIGURE
GO
alter database [$(DatabaseName)] set trustworthy on

exec sp_changedbowner 'sa'