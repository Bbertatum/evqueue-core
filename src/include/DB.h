/*
 * This file is part of evQueue
 * 
 * evQueue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * evQueue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with evQueue. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Thibault Kummer <bob@coldsource.net>
 */

#ifndef __DB_H__
#define __DB_H__

#include <mysql/mysql.h>

#include <string>
#include <vector>

class DB
{
	MYSQL *mysql;
	MYSQL_RES *res;
	MYSQL_ROW row;
	unsigned long *row_field_length;
	
	bool transaction_started;
	
	bool is_connected;
	bool is_copy;
	
public:
	DB(DB *db);
	DB(void);
	~DB(void);
	
	DB *Clone(void);
	
	void Ping(void);
	void Query(const char *query);
	void QueryPrintfC(const char *query,...);
	void QueryPrintf(const std::string &query,...);
	void QueryVsPrintf(const std::string &query,const std::vector<void *> &args);
	void EscapeString(const char *string, char *escaped_string);
	int InsertID(void);
	
	bool FetchRow(void);
	void Seek(int offset);
	void Free(void);
	int NumRows(void);
	int AffectedRows(void);
	
	void StartTransaction();
	void CommitTransaction();
	void RollbackTransaction();
	
	bool GetFieldIsNULL(int n);
	std::string GetField(int n);
	int GetFieldInt(int n);
	double GetFieldDouble(int n);
	unsigned long GetFieldLength(int n);
	
	void Disconnect();
	
private:
	void connect();
};

#endif
