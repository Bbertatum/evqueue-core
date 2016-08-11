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

#include <NotificationTypes.h>
#include <NotificationType.h>
#include <DB.h>
#include <Exception.h>
#include <Logger.h>
#include <SocketQuerySAX2Handler.h>
#include <QueryResponse.h>
#include <sha1.h>

#include <string.h>

#include <xqilla/xqilla-dom3.hpp>

NotificationTypes *NotificationTypes::instance = 0;

using namespace std;
using namespace xercesc;

NotificationTypes::NotificationTypes()
{
	instance = this;
	
	pthread_mutex_init(&lock, NULL);
	
	Reload();
}

NotificationTypes::~NotificationTypes()
{
	// Clean current tasks
	for(auto it=notifications_name.begin();it!=notifications_name.end();++it)
		delete it->second;
	
	notifications_name.clear();
	notifications_id.clear();
}

void NotificationTypes::Reload(void)
{
	Logger::Log(LOG_NOTICE,"[ NotificationTypes ] Reloading configuration from database");
	
	pthread_mutex_lock(&lock);
	
	// Clean current tasks
	for(auto it=notifications_name.begin();it!=notifications_name.end();++it)
		delete it->second;
	
	notifications_name.clear();
	notifications_id.clear();
	
	// Update
	DB db;
	DB db2(&db);
	db.Query("SELECT notification_type_id,notification_type_name FROM t_notification_type");
	
	while(db.FetchRow())
	{
		NotificationType *notification_type = new NotificationType(&db2,db.GetFieldInt(0));
		std::string notification_type_name(db.GetField(1));
		
		notifications_name[notification_type_name] = notification_type;
		notifications_id[db.GetFieldInt(0)] = notification_type;
	}
	
	pthread_mutex_unlock(&lock);
}

bool NotificationTypes::Exists(unsigned int id)
{
	pthread_mutex_lock(&lock);
	
	auto it = notifications_id.find(id);
	if(it==notifications_id.end())
	{
		pthread_mutex_unlock(&lock);
		
		return false;
	}
	
	pthread_mutex_unlock(&lock);
	
	return true;
}

void NotificationTypes::SyncBinaries(void)
{
	Logger::Log(LOG_NOTICE,"[ NotificationTypes ] Syncing binaries");
	
	pthread_mutex_lock(&lock);
	
	DB db;
	
	// Load tasks from database
	db.Query("SELECT notification_type_binary, notification_type_binary_content FROM t_notification_type WHERE notification_type_binary_content IS NOT NULL");
	
	struct sha1_ctx ctx;
	char db_hash[20];
	string file_hash;
	
	while(db.FetchRow())
	{
		// Compute database SHA1 hash
		sha1_init_ctx(&ctx);
		sha1_process_bytes(db.GetField(1),db.GetFieldLength(1),&ctx);
		sha1_finish_ctx(&ctx,db_hash);
		
		// Compute file SHA1 hash
		try
		{
			NotificationType::GetFileHash(db.GetField(0),file_hash);
		}
		catch(Exception &e)
		{
			Logger::Log(LOG_NOTICE,"[ NotificationTypes ] Task %s was not found creating it",db.GetField(0));
			
			NotificationType::PutFile(db.GetField(0),string(db.GetField(1),db.GetFieldLength(1)),false);
			continue;
		}
		
		if(memcmp(file_hash.c_str(),db_hash,20)==0)
		{
			Logger::Log(LOG_NOTICE,"[ NotificationTypes ] Task %s hash matches DB, skipping",db.GetField(0));
			continue;
		}
		
		Logger::Log(LOG_NOTICE,"[ NotificationTypes ] Task %s hash does not match DB, replacing",db.GetField(0));
		
		NotificationType::RemoveFile(db.GetField(0));
		NotificationType::PutFile(db.GetField(0),string(db.GetField(1),db.GetFieldLength(1)),false);
	}
	
	pthread_mutex_unlock(&lock);
}

NotificationType NotificationTypes::GetNotificationType(unsigned int id)
{
	pthread_mutex_lock(&lock);
	
	auto it = notifications_id.find(id);
	if(it==notifications_id.end())
	{
		pthread_mutex_unlock(&lock);
		
		throw Exception("NotificationTypes","Unable to find notification type");
	}
	
	NotificationType notification_type = *it->second;
	
	pthread_mutex_unlock(&lock);
	
	return notification_type;
}

NotificationType NotificationTypes::GetNotificationType(const string &name)
{
	pthread_mutex_lock(&lock);
	
	auto it = notifications_name.find(name);
	if(it==notifications_name.end())
	{
		pthread_mutex_unlock(&lock);
		
		throw Exception("NotificationTypes","Unable to find notification type");
	}
	
	NotificationType notification_type = *it->second;
	
	pthread_mutex_unlock(&lock);
	
	return notification_type;
}

bool NotificationTypes::HandleQuery(SocketQuerySAX2Handler *saxh, QueryResponse *response)
{
	NotificationTypes *notification_types = NotificationTypes::GetInstance();
	
	const std::map<std::string,std::string> attrs = saxh->GetRootAttributes();
	
	auto it_action = attrs.find("action");
	if(it_action==attrs.end())
		return false;
	
	if(it_action->second=="list")
	{
		pthread_mutex_lock(&notification_types->lock);
		
		for(auto it = notification_types->notifications_name.begin(); it!=notification_types->notifications_name.end(); it++)
		{
			NotificationType notification_type = *it->second;
			DOMElement *node = (DOMElement *)response->AppendXML("<notification_type />");
			node->setAttribute(X("id"),X(std::to_string(notification_type.GetID()).c_str()));
			node->setAttribute(X("name"),X(notification_type.GetName().c_str()));
			node->setAttribute(X("description"),X(notification_type.GetDescription().c_str()));
			node->setAttribute(X("binary"),X(notification_type.GetBinary().c_str()));
		}
		
		pthread_mutex_unlock(&notification_types->lock);
		
		return true;
	}
	
	return false;
}
