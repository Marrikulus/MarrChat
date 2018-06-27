#include "Common.h"

int main(int argc, char const *argv[])
{
	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd(SERVER_PORT,0);
	RakNet::Packet *packet;

	peer->Startup(MAX_CLIENTS, &sd, 1);
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);

	std::vector<Message> messages = {};

	while (1)
	{
		for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			switch (packet->data[0])
			{
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
				{
					std::cout << "Another client has disconnected." << std::endl;
				}break;
				case ID_REMOTE_CONNECTION_LOST:
				{
					std::cout << "Another client has lost the connection." << std::endl;
				}break;
				case ID_REMOTE_NEW_INCOMING_CONNECTION:
				{
					std::cout << "Another client has connected." << std::endl;
				}break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
				{
					std::cout << "Our connection request has been accepted." << std::endl;

					RakNet::BitStream bsOut;
					bsOut.Write((RakNet::MessageID)ID_MARR_MESSAGE);
					bsOut.Write("Hello, I'm server");

					peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
				}break;
				case ID_NEW_INCOMING_CONNECTION:
				{
					std::cout << "A connection is incoming." << std::endl;
				}break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
				{
					std::cout << "The server is full." << std::endl;
				}break;
				case ID_DISCONNECTION_NOTIFICATION:
				{
					std::cout << "A client has disconnected." << std::endl;
				}break;
				case ID_CONNECTION_LOST:
				{
					std::cout << "A client lost the connection." << std::endl;
				}break;
				case ID_MARR_MESSAGE:
				{
					//Message msg = {};

					Message* msg = (Message*)packet->data;
					messages.push_back(*msg);

					peer->Send((char*)msg,sizeof(Message),HIGH_PRIORITY,RELIABLE_ORDERED,0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
					//RakNet::RakString rs;
					//RakNet::BitStream bsIn(packet->data,packet->length,false);
					//bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
					//bsIn.Read(rs);
					std::cout << "From: '" << msg->name << "'" << std::endl;
					std::cout << "text: '" << msg->text << "'" << std::endl;
				}break;
				default:
				{
					std::cout << "Message with identifier " << packet->data[0] << " has arrived." << std::endl;
				} break;
			}
		}
	}

	RakNet::RakPeerInterface::DestroyInstance(peer);
	return 0;
}