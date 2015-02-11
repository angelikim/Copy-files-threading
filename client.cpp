#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <cstdlib>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>

#include "library.h"

using namespace std;




int main (int argc , char ** argv)
{
	int counter=0 ,flag=0;
    int server_port, sock ;
    char * server_ip, *directory ;
    struct sockaddr_in server;
    struct sockaddr * serverptr = (struct sockaddr * ) & server;
    struct hostent *rem;
    
    if(( argc != 7 )  )
    {
        cout<<"Wrong arguments ,try again ..."<<endl;
        return 1;
    }

    		
    for(int i= 1 ; i < 6 ; i=i+2)
    {
        if(!strcmp(argv[i],"-i"))
        {
            server_ip=(argv[i+1]);
            cout<<"server_ip : " <<server_ip <<endl;    
        }
        else if(!strcmp(argv[i],"-p"))
        {
            server_port=atoi(argv[i+1]);
            cout<<"server_port : " <<server_port <<endl;
        }
        else if(!strcmp(argv[i],"-d"))
        {  
        	
            directory= argv[i+1];
            if(!strcmp (directory , "exit"))                    //if client sends "exit" , server terminates
            	flag=1;
            
            
            cout<<"directory : " <<directory <<endl;
        }
        
    }
    
    if (( sock = socket (PF_INET,SOCK_STREAM , 0 )) < 0 )          //establishing connention
        perror ("error in socket");
   
    if (( rem = gethostbyname(server_ip)) == NULL )
        herror("error in gethostbyname");
        
    server.sin_family=AF_INET;
    memcpy(&server.sin_addr , rem->h_addr ,rem->h_length);
    server.sin_port=htons(server_port);
    
    if( connect(sock ,serverptr , sizeof(server)) <0 )
        perror("error in connect");
        
    cout<< "Connecting to socket " << endl;
	
    
	
    
    int size= strlen(directory);
    int tmp=htonl(size);

   write_to_socket( sock ,& tmp , sizeof(int) );                      //sending size of "directory " ( path size )  argument to server
   write_to_socket( sock ,directory , size );						  //sending actuall path
    
    if(flag==1)														 //if "exist" , terminate client , gracefully closing the socket end
   	{
		shutdown(sock, SHUT_WR);
		return 0;
    }
    
	int page_size;													// receiving page size of server		
	read_from_socket(sock , &page_size , sizeof(int));
	
	page_size= htonl(page_size);
   
   
   while(1)
   {	
   		shutdown(sock, SHUT_WR);
   		
   		size=0;    
   		
   		read_from_socket(sock , &size , sizeof(int));			//receiving path size from server
       	size=ntohl(size);
       
       	if( size == -1 )										//if size =  -1 , either the requested file does not exist , or server has sent all files
			break;
   	 	 
   		
	    char * buff = new char [size+1];
	
		read_from_socket(sock , buff , size);                   //receiving actuall path

		
        buff[size]='\0';
        DIR * dir;
        if(directory[0] == '/' )                             //absolute path
        {
        	
        	if((dir = opendir(buff) ) == NULL )
			{
				string part1(buff),str,path;
		    	istringstream str_stream(part1);
		        int i=0;
		        int count =0;
		        while(buff[i] != '\0'  )
		        {  
		            if(part1[i] == '/')
		                count++;
		            i++;
		        }
		        char ** part = new   char * [count ]  ;
		        for(i= 0 ; i  < count ; i ++ )
		            part[i]= new char[15];
		        
		        i=0;
		        path.clear();
		        while(getline(str_stream,str,'/'))
		        {
		        	if(i== ( count ) || ( i == 0 ) )
		        		break;
		            strcpy(part[i-1],str.c_str());
		           
		            path= path + part[i];
		            mkdir(path.c_str() , 0777 );
		             path=path + "/" ;
		           
		            i++;
		        }
				for(i= 0 ; i  < count ; i ++ )
				{  
					
					delete[] part[i];
		  		}
		  		delete[] part;
		  		
		  		if((dir = opendir(directory) ) == NULL )
		  			cout << " unaible to create folder " <<endl;
			}        	
       
       
        }
        else
        {
        	
        	if((dir = opendir(buff) ) == NULL )
			{  
				string part1(buff),str,path;
		    	istringstream str_stream(part1);
		        int i=0;
		        int count =0;
		        while(buff[i] != '\0'  )
		        {  
		            if(part1[i] == '/')
		                count++;
		            i++;
		        }
		        char ** part = new   char * [count ]  ;
		        for(i= 0 ; i  < count ; i ++ )
		            part[i]= new char[15];
		        
		        i=0;
		        path.clear();
		        while(getline(str_stream,str,'/'))
		        {
		        	if(i== ( count ))
		        		break;
		            strcpy(part[i],str.c_str());
		           
		            path= path + part[i];
		            mkdir(path.c_str() , 0777 );
		             path=path + "/" ;
		            i++;
		        }
				for(i= 0 ; i  < count ; i ++ )
				{  
					
					delete[] part[i];
		  		}
		  		delete[] part;
		  		
		  		if((dir = opendir(directory) ) == NULL )
		  			cout << " unaible to create folder " <<endl;
			}
       	
        }
      
        ofstream file( buff , std::ios::binary  | ios::trunc );
   		if(file == NULL)
   			cout << " unable to open file  " <<endl;
   		
        read_from_socket( sock  , &size  , sizeof(int) );            //size of file
  

       	size=ntohl(size);

		
        char *buffer = new char [page_size ];
        int count = size /page_size ;
        if( (size % page_size  ) > 0 )
        	count++;
        counter=size;
        for(int i=0 ; i < count ; i ++ )
        {
        	if(counter >= page_size  )
        	{
        		read_from_socket(sock , buffer , page_size  );
            	file.write(buffer,page_size );
            }
            else
            {	
            	read_from_socket(sock , buffer , counter );
            	file.write(buffer,counter);
            }
            counter -=page_size ;
            
        }
        file.close();
  		delete[] buffer;
        
        

        
  		
	}
     
  shutdown(sock, SHUT_WR);
  return 0;
}
