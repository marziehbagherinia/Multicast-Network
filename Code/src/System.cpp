#include "System.h"

System::System(int pipeFd, string sys_name)
{
	int n;
	char buff[MAX_BUFF];
	
	n = read(pipeFd, buff, MAX_BUFF);
	buff[n] = '\0';
	

	pipe_fd = pipeFd;
	name = sys_name;

	fifoPath = Tools::getNamedFifoName(to_string(getpid()));

	if (n == -1 && errno == EAGAIN)
	{
		send_message_LB("F");
		close(pipeFd);
    }
	else
	{
		send_message_LB("S");
		wait_for_command();
	}
}

string System::wait_for_command()
{
	int n, fd;
	string str_buff;
	vector <string> v;
	char buff[MAX_BUFF];

	while (waited)
	{
		n = read(pipe_fd, buff, MAX_BUFF);
		buff[n] = '\0';	

		if (n != -1)
		{
			str_buff = buff;
			command_handler(Tools::splitSpace(str_buff));
		}
		check_pipe();
	}

	close(pipe_fd);
	return str_buff;
}

void System::command_handler(vector <string> command)
{
	string message;
	
	if (command.size() < 1)
	{
		cerr << "Failed connection!" << endl;
		return; 
	}
	
	if (command[0] == "exit")
	{
		waited = false;
	}
	else if(command[0] == "Connect")
	{
		if (command.size() == 4)
		{
			create_pipe(command);
			send_message_LB("S");
		}
		else
		{
			send_message_LB("F");
		}
	}
	else if(command[0] == SEND)
	{
		if (command.size() == 4)
		{
			file_content = read_file(command[3]);

			if(file_content.length() == 0)
			{
				cout << "The file you requested does not exist." << endl;
			}
			else
			{
				while (file_content.length() > 0)
				{
					sleep(1);
					message = prepare_message_send(command);
					send_message(message);
					cout << "The system " << name << " sent the <Send> message." << endl;
				}
			}
		}
		else
		{
			cout << "The <Send> Message not sent." << endl;
		}	
	}
	else if(command[0] == RECV)
	{
		if (command.size() == 4)
		{
			message = prepare_message_recv(command);
			send_message(message);	
			cout << "The system " << name << " sent the <Recv> message." << endl;
		}
		else
		{
			cout << "The <Recv> Message not sent." << endl;
		}	
	}
	else if (command[0] == "Join_group" )
	{
		if (command.size() != 3)
		{
			cerr << "Bad request!" << endl;
		}
		else
		{
			groups.push_back(command[2]);
			cout << "Client joins " << command[2] << " Group successfully!" << endl;
		}
	}
	else if (command[0] == "Sync")
	{
		vector <string> msg;
		msg.push_back(RECV);
		msg.push_back("");

		for (int i = 1; i < command.size(); i++)
			if (clients.find(command[i]) == clients.end() && command[i] != name)
				clients[command[i]] = "";
		
		for (auto i : clients)
		{
			msg[1] = i.first;
			message = prepare_message_recv(msg);
			send_message(message);
		}
	}
}

void System::send_message_LB(string message)
{
	int fd = open(fifoPath.c_str(), O_WRONLY | O_CREAT, 0666);
	write(fd, message.c_str(), message.length());
	close(fd);
}

void System::create_pipe(vector <string> command)
{
	fds.push_back(stoi(command[1]));
	fds.push_back(stoi(command[2]));
	fds.push_back(stoi(command[3]));
	connected = true;
}

void System::send_message(string message)
{
	write(fds[WRITE], message.c_str(), message.length());
}

string System::prepare_message_send(vector <string> command)
{
	int remaind_char;
	string temp, message;
	
	message  = SEND;
	message += SPACE + name;
	message += SPACE + command[1];
	message += SPACE + command[2];
	message += SPACE + command[3];
	
	remaind_char = MAX_BUFF - message.length() - 2;

	if (file_content.length() < remaind_char)
	{
		message += SPACE + file_content;
		file_content.erase();
	}
	else
	{
		temp = file_content.substr(0, remaind_char);
		file_content.erase(file_content.begin(), file_content.begin() + remaind_char);
		message += SPACE + temp;
	}

	return message;
}

string System::prepare_message_recv(vector <string> command)
{
	int remaind_char;
	string temp;
	string message = command[0];
	message += SPACE + name;
	message += SPACE + name;
	message += SPACE + command[1];
	message += SPACE + name;

	return message;
}

void System::handle_message(vector <string> str, string message_content)
{
	string message;
	
	if (str.size() < 1)
	{
		cerr << "Failed connection!" << endl;
		return; 
	}
	else if(str[0] == SEND)
	{
		cout << "The system " << name << " from Group " << str[3] << " recieved the <Send> message." << endl;

		if (files.find(name + "_" + str[4]) == files.end())
		{
			ofstream fout;
			string filename = "./Test_Files/";
			filename += name + "_" + str[4];

			fout.open(filename);

			fout << message_content << endl;
		
			fout.close();
			files[name + "_" + str[4]] = 1;
		}
		else
		{
			ofstream fout;
			string filename = "./Test_Files/";
			filename += name + "_" + str[4];
			fout.open(filename, ios::app);

			fout << message_content << endl;
		
			fout.close();
		}
	}
	else if(str[0] == "Send_sync")
	{
		vector <string> msg;

		if (str.size() == 5)
		{
			msg = Tools::splitComma(str[4]);

			if (clients.find(msg[msg.size() - 1]) != clients.end() && clients[msg[msg.size() - 1]] == "")
			{
				for (int i = 0; i < msg.size(); i++)
				{
					if (msg[i] == name && i != msg.size() - 1)
					{
						clients[msg[msg.size() - 1]] = msg[i + 1];
						break;
					}	
				}
				
			}
			
			//for (auto i : clients)
			//	cout << "The System " << name <<   " is " << i.first << " : " << i.second << endl;
		}
		
	}
	else if(str[0] == RECV)
	{
		vector <string> command;

		if (str.size() == 5)
		{
			string message;
	
			message  = "Send_sync";
			message += SPACE + name;
			message += SPACE + str[3];
			message += SPACE + str[2];
			message += SPACE + str[4] + COMMA + name;
				
			send_message(message);
			
			//cout << "The system " << name << " send the <Recv> message." << endl;
		}
		else
		{
			cout << "<Recv> Message's format is not correct." << endl;
		}	
	}
}

void System::check_pipe()
{
	int fd, n, pos;
	string str_buff;
	char buff[MAX_BUFF];
	vector <string> str;

	if(connected == true)
	{
		n = read(fds[2], buff, MAX_BUFF);
		buff[n] = '\0';
		if (n != -1)
		{
			str_buff = buff;
			str = Tools::splitSpace(str_buff);

			if(is_my_message(str[3]) || str[3] == name)
			{
				//cout << "System: " << str[0] << " " << str_buff.length() << endl;

				for (int i = 0; i < 5; i++)
				{
					pos = str_buff.find(str[i]);
					if (pos != string::npos)
						str_buff.erase(pos, str[i].length() + 1);
				}

				handle_message(str, str_buff);
			}
		}
	}
}

bool System::is_my_message(string dest)
{
	for (int i = 0; i < groups.size(); i++)
		if (groups[i] == dest)
			return true;
	
	return false;
}

string System::read_file(string filename)
{
	string line, file_content;

	file_content = "";
	ifstream myfile(filename.c_str());
	
	if(myfile.is_open())
	{
		while (getline(myfile, line))
			file_content += line + "\n";
	
		file_content.erase(file_content.end() - 1, file_content.end());
	}

	return file_content;
}

System::~System()
{
	exit(0);
}