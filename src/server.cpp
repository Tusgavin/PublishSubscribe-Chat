#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <algorithm>

#include "./utils.hpp"

#define BUFSIZE 500

#define ADD_CHANNEL 1
#define REMOVE_CHANNEL 2
#define BROADCAST 3
#define NON_SPECIAL_MESSAGE 4
#define EXIT_SERVER 5

struct client_data
{
   int c_sock;
   struct sockaddr_storage storage;
   std::vector<std::string> interest_topics = {};
};

std::vector<struct client_data *> clients_channels = {};

void usage()
{
   std::cout << "[!] Error executing server" << std::endl;
   std::cout << ">> Usage ./server <v4/v6> <server_port>" << std::endl;
   std::cout << ">> Usage Example: ./server v4 56560" << std::endl;
   exit(EXIT_FAILURE);
};

int check_type_of_message(std::string message)
{
   if (message == "##kill")
   {
      return EXIT_SERVER;
   }
   else if (message.at(0) == '+' && message.length() > 1)
   {
      return ADD_CHANNEL;
   }
   else if (message.at(0) == '-' && message.length() > 1)
   {
      return REMOVE_CHANNEL;
   }
   else
   {
      for (size_t i = 0; i < message.length(); ++i)
      {
         if (message.at(i) == '#' && message.length() > 1 && (i != 0))
         {
            return BROADCAST;
         }
      }
   }

   return NON_SPECIAL_MESSAGE;
}

std::vector<std::string> get_topics_to_broadcast(std::string message)
{
   std::vector<std::string> topic_strings;
   message = message + " ";

   for (size_t i = 0; i < message.length() - 1; ++i)
   {
      if (message.at(i) == '#' && message.at(i - 1) == ' ')
      {
         for (size_t j = i; j < message.length(); ++j)
         {
            if (message.at(j) == ' ')
            {
               topic_strings.push_back(message.substr(i + 1, j - i - 1));
               break;
            }
         }
      }
   }

   return topic_strings;
}

bool message_validation(std::string message)
{
   std::string symbols = ",.?!:;+-*/=@#$%()[]{} \n";
   bool valid = std::all_of(&message[0], &message[message.length() - 1], [&](char c){
      if (c >= 48 && c <= 57)
      return true;
      if (c >= 65 && c <= 90)
         return true;
      if (c >= 97 && c <= 122)
         return true;
      if (symbols.find(c) != std::string::npos)
         return true;
      return false;
   });

   return valid;
}

bool check_same_elements_vectors(std::vector<std::string> vec1, std::vector<std::string> vec2)
{
   for (std::vector<std::string>::iterator it_vec1 = vec1.begin(); it_vec1 != vec1.end(); ++it_vec1)
   {
      for (std::vector<std::string>::iterator it_vec2 = vec2.begin(); it_vec2 != vec2.end(); ++it_vec2)
      {
         if ((*it_vec1) == (*it_vec2))
         {
            return true;
         }
      }
   }

   return false;
}

void handle_add_channel(std::string message, struct client_data *c_data)
{
   std::string topic_of_interest = message.substr(1, message.length() - 1);
   int check_already_sub_count = 0;
   for (size_t i = 0; i < c_data->interest_topics.size(); ++i)
   {
      if (c_data->interest_topics.at(i) == topic_of_interest)
      {
         std::string already_sub = "Already subscribed to " + message;
         size_t count_send = send(c_data->c_sock, already_sub.c_str(), already_sub.length() + 1, 0);
         if (count_send != already_sub.length() + 1)
         {
            std::cout << "[!] Error while sending message" << std::endl;
         }
         check_already_sub_count++;
      }
   }
   if (!check_already_sub_count)
   {
      c_data->interest_topics.push_back(topic_of_interest);
      std::string new_sub = "Subscribed to " + message;
      size_t count_send = send(c_data->c_sock, new_sub.c_str(), new_sub.length() + 1, 0);
      if (count_send != new_sub.length() + 1)
      {
         std::cout << "[!] Error while sending message" << std::endl;
      }
   }
};

void handle_remove_channel(std::string message, struct client_data *c_data)
{
   std::string topic_of_interest = message.substr(1, message.length() - 1);
   int check_already_sub_count = 0;
   for (size_t i = 0; i < c_data->interest_topics.size(); ++i)
   {
      if (c_data->interest_topics.at(i) == topic_of_interest)
      {
         c_data->interest_topics.erase(c_data->interest_topics.begin() + i);
         std::string already_sub = "Unsubscribe to " + message;
         size_t count_send = send(c_data->c_sock, already_sub.c_str(), already_sub.length() + 1, 0);
         if (count_send != already_sub.length() + 1)
         {
            std::cout << "[!] Error while sending message" << std::endl;
         }
         check_already_sub_count++;
      }
   }
   if (!check_already_sub_count)
   {
      std::string new_sub = "Not subscribed to " + message;
      size_t count_send = send(c_data->c_sock, new_sub.c_str(), new_sub.length() + 1, 0);
      if (count_send != new_sub.length() + 1)
      {
         std::cout << "[!] Error while sending message" << std::endl;
      }
   }
}

void handle_broadcast(std::string message)
{
   std::vector<std::string> topics = get_topics_to_broadcast(message);

      for (std::vector<struct client_data *>::iterator it = clients_channels.begin(); it != clients_channels.end(); ++it)
      {
         if (check_same_elements_vectors((*it)->interest_topics, topics))
         {
            size_t count_send = send((*it)->c_sock, message.c_str(), message.length() + 1, 0);
            if (count_send != message.length() + 1)
            {
               std::cout << "[!] Error while sending message" << std::endl;
            }
         }
      }
}

void handle_invalid_message(struct sockaddr *c_addr, struct client_data *c_data)
{
   std::cout << ">> Closing connection with: " << getAddrStr(c_addr) << " " << getAddrPort(c_addr) << " due to invalid character" << std::endl;

   std::string exit_resp = ">> Connection Closed";

   size_t count_send = send(c_data->c_sock, exit_resp.c_str(), exit_resp.length() + 1, 0);
   if (count_send != exit_resp.length() + 1)
   {
      std::cout << "[!] Error while sending message" << std::endl;
   }

   shutdown(c_data->c_sock, SHUT_RDWR);
   close(c_data->c_sock);
}

void handle_message_too_large(struct sockaddr *c_addr, struct client_data *c_data)
{
   std::cout << ">> Closing connection with: " << getAddrStr(c_addr) << " " << getAddrPort(c_addr) << " dua to message exceding limit" << std::endl;

   std::string exit_resp = ">> Connection Closed";

   size_t count_send = send(c_data->c_sock, exit_resp.c_str(), exit_resp.length() + 1, 0);
   if (count_send != exit_resp.length() + 1)
   {
      std::cout << "[!] Error while sending message" << std::endl;
   }

   shutdown(c_data->c_sock, SHUT_RDWR);
   close(c_data->c_sock);
}

void * client_handling_thread(void *data)
{
   struct client_data *c_data = (struct client_data *)data;
   struct sockaddr *c_addr = (struct sockaddr *)(&c_data->storage);

   while(1)
   {
      char r_buffer[BUFSIZE];
      memset(r_buffer, 0, BUFSIZE);
      size_t count = recv(c_data->c_sock, r_buffer, BUFSIZE, 0);

      if (count > 0)
      {
         std::cout << "Received: " << std::string(r_buffer) << std::endl;
      }

      if (std::string(r_buffer).length() > 500)
      {
         handle_message_too_large(c_addr, c_data);
         pthread_exit(EXIT_SUCCESS);
      }

      std::string pure_message = std::string(r_buffer).substr(0, std::string(r_buffer).length() - 1);

      if (!message_validation(std::string(r_buffer)))
      {
         handle_invalid_message(c_addr, c_data);
         pthread_exit(EXIT_SUCCESS);
      }

      int type_of_message = check_type_of_message(pure_message);

      if (type_of_message == ADD_CHANNEL)
      {
         handle_add_channel(pure_message, c_data);
      }
      else if (type_of_message == REMOVE_CHANNEL)
      {
         handle_remove_channel(pure_message, c_data);
      }
      else if (type_of_message == BROADCAST)
      {
         handle_broadcast(pure_message);
      }
      else if (type_of_message == NON_SPECIAL_MESSAGE)
      {
         
      }
      else if (type_of_message == EXIT_SERVER)
      {
         exit(EXIT_SUCCESS);
      }
   }

   pthread_exit(EXIT_SUCCESS);
};

int main(int argc, char *argv[])
{
   if (argc < 2)
   {
      usage();
   }

   /* Get port and type of IP */
   int port = atoi(argv[1]);
   std::string type_of_IP = "v4";

   /* Create server socket */
   struct sockaddr_storage storage;
   memset(&storage, 0, sizeof(storage));

   if (type_of_IP == "v4")
   {
      struct sockaddr_in *addr4 = (struct sockaddr_in *)(&storage);
      addr4->sin_family = AF_INET;
      addr4->sin_addr.s_addr = INADDR_ANY;
      addr4->sin_port = htons(port);

      std::cout << ">> Server 32-bit IP address (IPv4)" << std::endl;
   } 
   else if (type_of_IP == "v6")
   {
      struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)(&storage);
      addr6->sin6_family = AF_INET6;
      addr6->sin6_port = htons(port);
      addr6->sin6_addr = in6addr_any;

      std::cout << ">> 128-bit IP address (IPv6)" << std::endl;
   } 
   else
   {
      usage();
   }

   int s_socket = socket(storage.ss_family,  SOCK_STREAM, 0);
   if (s_socket == -1)
   {
      std::cout << "[!] Error while creating socket" << std::endl;
      exit(EXIT_FAILURE);
   }

   int enable = 1;
   if (0 != setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
   {
      std::cout << "[!] Error reusing port " << port << std::endl;
      exit(EXIT_FAILURE);
   }

   /* Bind */
   if (0 != bind(s_socket, (struct sockaddr *)(&storage), sizeof(storage)))
   {
      std::cout << "[!] Error bind" << std::endl;
      exit(EXIT_FAILURE);
   }

   /* Listen */
   if (0 != listen(s_socket, 10))
   {
      std::cout << "[!] Error listen" << std::endl;
      exit(EXIT_FAILURE);
   }

   std::cout << ">> Server waiting for connections..." << std::endl;

   /* Accept */
   while(1)
   {
      struct sockaddr_storage client_storage;
      socklen_t caddrlen = sizeof(client_storage);
      int c_sock = accept(s_socket, (struct sockaddr *)(&client_storage), &caddrlen);
      std::cout << ">> Client connected from: " << getAddrStr((struct sockaddr *)(&client_storage)) << " " << getAddrPort((struct sockaddr *)(&client_storage)) << std::endl;

      if (c_sock == -1)
      {
         std::cout << "[!] Error accept" << std::endl;
         exit(EXIT_FAILURE);
      }

      struct client_data *client_data_thread = (struct client_data *)malloc(sizeof(*client_data_thread));
      if (!client_data_thread)
      {
         std::cout << "[!] Error allocating memory" << std::endl;
         exit(EXIT_FAILURE);
      }

      client_data_thread->c_sock = c_sock;
      memcpy(&(client_data_thread->storage), &client_storage, sizeof(client_storage));

      clients_channels.push_back(client_data_thread);

      pthread_t tid;
      pthread_create(&tid, NULL, client_handling_thread, client_data_thread);
   }
   
   exit(EXIT_SUCCESS);
}
