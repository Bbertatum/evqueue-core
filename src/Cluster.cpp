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

#include <Cluster.h>
#include <Configuration.h>
#include <Exception.h>
#include <SocketResponseSAX2Handler.h>
#include <SocketSAX2Handler.h>
#include <Sockets.h>
#include <Logger.h>
#include <Client.h>
#include <sha1.h>

#include <sstream>
#include <iomanip>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>

using namespace std;

Cluster *Cluster::instance = 0;

void Cluster::ParseConfiguration(const string &conf)
{
	instance = this;
	
	notify = Configuration::GetInstance()->GetBool("cluster.notify");
	
	istringstream split(conf);
	for (std::string each; std::getline(split, each, ','); nodes.push_back(each));
	
	if(notify && nodes.size())
		Logger::Log(LOG_NOTICE,"Cluster notifications enabled");
	
	user = Configuration::GetInstance()->Get("cluster.notify.user");
	password = Configuration::GetInstance()->Get("cluster.notify.password");
	
	// Prepare hashed password
	sha1_ctx ctx;
	char c_hash[20];
	
	sha1_init_ctx(&ctx);
	sha1_process_bytes(password.c_str(),password.length(),&ctx);
	sha1_finish_ctx(&ctx,c_hash);
	
	// Format HEX result
	stringstream sstream;
	sstream << hex;
	for(int i=0;i<20;i++)
		sstream << std::setw(2) << setfill('0') << (int)(c_hash[i]&0xFF);
	password = sstream.str();
	
	cnx_timeout = Configuration::GetInstance()->GetInt("cluster.cnx.timeout");
	snd_timeout = Configuration::GetInstance()->GetInt("cluster.snd.timeout");
	rcv_timeout = Configuration::GetInstance()->GetInt("cluster.rcv.timeout");
}

void Cluster::Notify(const string &command)
{
	if(!notify)
		return; // Cluster notifications are disabled
	
	ExecuteCommand(command);
}

bool Cluster::ExecuteCommand(const string &command, DOMDocument *response )
{
	if(nodes.size()==0)
		return false; // Cluster is not configured
	
	Logger::Log(LOG_NOTICE, "Executing cluster command "+command);
	
	if(response)
		response->appendChild(response->createElement("cluster-response"));
	
	for(int i=0;i<nodes.size();i++)
	{
		try
		{
			Client client(nodes.at(i),user,password);
			client.SetTimeouts(cnx_timeout,snd_timeout,rcv_timeout);
			
			// Check if we are not self connecting
			const string node = client.Connect();
			if(node==Configuration::GetInstance()->Get("cluster.node.name"))
			{
				client.Disconnect();
				Logger::Log(LOG_INFO, "Skipping current node in cluster notification");
				continue;
			}
			
			if(response)
				client.Exec(command, true);
			else
				client.Exec(command);
			
			if(response)
				response->getDocumentElement().appendChild(response->importNode(client.GetResponseDOM()->getDocumentElement(),true));
		}
		catch(Exception &e)
		{
			Logger::Log(LOG_ERR, "Error executing cluster command on node "+nodes.at(i)+" : "+e.error);
		}
	}
	
	return true;
}

std::vector<std::string> Cluster::Ping(void)
{
	Logger::Log(LOG_INFO, "Discovering cluster");
	
	vector<string> res;
	
	for(int i=0;i<nodes.size();i++)
	{
		try
		{
			Client client(nodes.at(i),user,password);
			client.SetTimeouts(cnx_timeout,snd_timeout,rcv_timeout);
			
			res.push_back(client.Connect());
			client.Disconnect();
		}
		catch(Exception &e)
		{
			// Errors are ignored in discovery mode
		}
	}
	
	return res;
}