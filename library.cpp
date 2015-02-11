#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <dirent.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fstream>


#include "library.h"



//run -p 12500 -s 10 -q 5

using namespace std;

Server serv;
int fl =0;

pthread_mutex_t lock_read=PTHREAD_MUTEX_INITIALIZER;      //mutex for queue access 
pthread_cond_t condition_read= PTHREAD_COND_INITIALIZER;	//condition for writting to queue/(while reading from client)
pthread_cond_t condition_write= PTHREAD_COND_INITIALIZER;	//condition for reading from queue / ( while responding to client)

void signal_kill (int signo )            //signal handler for SIGINT
{										 //signal is received via Cntrl -c , client request  catalog is equal to "exit" 
    serv.set_flag(1);
    pthread_cond_broadcast(&condition_write); //so that all threads are informed
    close(serv.get_sock());					  //closing server-side socket
	
}


void write_to_socket( int socket , void * ptr , int size ) //write all
{
		if(size == 0 )
			return;
		int counter=0;
		while ( counter< size ) 
		{ 
			while(1)
			{ 
				
				int sent ,n;
				for(sent =0 ;   sent < size ; sent +=n )
				{
				 	if (( n= write(socket , ptr , size  -sent )) == -1  )
					{	
						perror("write" );
						
						n=0;
					}
					else
					{   
  						counter +=n;
  						
					}	
				}

				break;		
			
			}

		}
		
}

void read_from_socket ( int socket , void * ptr , int size) //read_all
{
	int counter = 0, n , count;
	while( (counter < size ))
        {
            while(1)
			{
				
				for(count =0 ;   count <size ; count +=n )
				{
					if (  ( n= read(socket , ptr , size -count) ) ==-1 )
					{
						perror("read");
						
					}
					else
					
					counter+=n;
				}

				break;		
			
			}         
        
        }
	
}

void  read_directory(const char * buf, int sock , pthread_mutex_t * lock , int * ptr)  //reading from directory requested and adding to queue
{
    DIR * directory;
    int x;
    struct dirent *dp;
    struct stat st;
    if( (directory = opendir( buf) ) == NULL ) //check if file exists 
    { 
        cout<< "cannot access file "<< buf <<endl;
        
        int size= -1;
   		int tmp=htonl(size);
   		write_to_socket(sock , &tmp , sizeof(int));
   		return;
    }
    dp = readdir(directory);
    while(dp!=NULL)
    {
        
        if( ( !strcmp(dp->d_name,".") ) || (!strcmp(dp->d_name,"..")) )
        {  
            dp = readdir(directory); // do not copy . and ..   
            continue;
        }
        string path(buf);
        x=path.length();
        if(path[x -1]!='/')               //add name of file||directory to path in order to check it's status
            path=path+'/';
        path=path+dp->d_name;
        stat(path.c_str(), &st);

       
        if(S_ISDIR(st.st_mode))
        {
          	
            read_directory(path.c_str() , sock ,lock ,  ptr);        //open directories recursively
            	    
        }
        else
         {  
           
            serv.get_waiting()-> push_Queue (path ,sock ,lock , ptr); //one path is added to queue each time
           

            (*ptr)++; 
            
         }      
        dp = readdir(directory);   
    }
    closedir(directory);
	return;
}




void add_to_client()
{
	 	   pthread_mutex_lock (&lock_read ) ;
		   while(serv.get_sending()->get_size() ==  serv.max_size()  )
		   {
		   		pthread_cond_wait(&condition_read , &lock_read);
		   
		   }
		   if(serv.get_waiting()->get_size() >0 )
		   {
		   		int sock, *remaining;
		   		string path;
		   		pthread_mutex_t * lock2;
		   		sock = serv.get_waiting()->get_sock();
				remaining =	serv.get_waiting()->get_remaining();	
		   		path= serv.get_waiting()->get_path();
		   		lock2 = serv.get_waiting()->get_mutex();
		   		
		   		serv.get_sending()->push_Queue(path , sock , lock2 , remaining );
				serv.get_waiting()->delete_from_Queue(1);
		   	}    
		    pthread_mutex_unlock (&lock_read ) ;
		  return ;  


}

void * first_line_threads  ( void * ptr )   // starting function of first line threads
{
        
        
        int size , *newsock = (int * )ptr;
        
        
        read_from_socket(* newsock , &size ,sizeof(int));       //reading size of requested path
     
        size=ntohl(size);
        

        char * buf= new char[size +1];
        read_from_socket(* newsock , buf ,size); 
        buf[size]= '\0';
        
        if(!strcmp(buf,"exit"))
        {
        	delete[] buf;
        	kill(getpid() ,SIGINT); 
        	
        }
        else
        {
		    pthread_mutex_t * lock = new pthread_mutex_t(); //creating lock for each client
		    
		    int *remaining_files = new int(0);
		    
		    pthread_mutex_init( lock , NULL ) ;
		    
		    pthread_mutex_lock(lock);       //sending pagesize to client
		    int sz=sysconf(_SC_PAGESIZE);
       	    sz =htonl(sz);
       	    write_to_socket( *newsock ,& sz , sizeof(int) );  
		    pthread_mutex_unlock(lock);
		    
		    read_directory(buf,*newsock, lock ,remaining_files ); //remaining files ( pointer )  informs client/server about the number of files to be sent
		    while(serv.get_waiting()->get_size () > 0 )
			{
				
				add_to_client();
				pthread_cond_broadcast(&condition_write); 
			}

		  delete[] buf;
		 buf = NULL;
        }
        
        pthread_exit( NULL ) ;
		
		
}




void  send_to_client  (  )
{
	int sz=sysconf(_SC_PAGESIZE);
	
		pthread_mutex_lock (&lock_read ) ;
		while(serv.get_sending()->get_size() <= 0 )
		{
			pthread_cond_wait(&condition_write ,&lock_read);
			if(serv.get_flag() == 1 )                           //means that server received SIGINT , exiting from loop
			{	
				pthread_mutex_unlock (&lock_read ) ;
				return;
			}	
		}
		
		
		
		pthread_mutex_t *mutex =  serv.get_sending()->get_mutex();     // getting info about the node/path that is going to be removed from queue
		pthread_mutex_lock(mutex);                            //only one thread can write to client
		string path=serv.get_sending()->get_path();
	    int  socket=serv.get_sending()->get_sock();

		int flag =serv.get_sending()->delete_from_Queue();
		pthread_mutex_unlock(&lock_read);  
	    int size =0 ;
	    
		
		
		
		size=strlen(path.c_str() );                           //size of path
		int tmp=htonl(size);
		write_to_socket( socket ,& tmp , sizeof(int) );
			
			
		const char * buff =path.c_str();
		char *buf = new char[size+1];
		strcpy(buf,buff);

		write_to_socket(  socket , buf ,  size );         //path
		delete[] buf;
		buf = NULL;
	    
	    ifstream file (path.c_str(), std::ifstream::binary);         //open file 
	   
	    
	    file.seekg(0,file.end);
	    size=file.tellg();                                           //size of file
	    file.seekg(0,file.beg);
		 tmp=htonl(size);

		write_to_socket(  socket ,( void * ) &tmp,  sizeof(int) );
		
	    //char * buffer = new char [sz];                                                                               //blepe akribos apo kato
	    char * buffer = new char [size];
	    file.read( buffer, size );                                                                                    //copying bytes to buffer
	    file.close();
	    int i=size , position=0;
		
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
		while(1)                                                          
		{   
			if(position >= size )
				break;
			if(i >=sz )
				write_to_socket( socket ,& buffer[position] , sz );                                                    // I diafora tou sxoliasmenou while me to
			else																									   //mi sxoliasmeno einai oti to proto diabazei 	
				write_to_socket( socket , &buffer[position] , i  );													   //olokliro to arxeio se buffer kai to stelnei ana page_size
			i-=sz;																									   //eno to deutero diabazei to arxeio ana page size , to stelnei ,
			position +=sz;																							   // ksanadiabazei kok. Par olo pou protimotero einai to deutero 		
		}																											   //kathos den exo enan terastio buffer , den ginete na to xrismopoiiso	
																													   //giati ston client an ena arxeio iparxei prepei na diagrafe me apotelesma
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////	   //na apotigxanei to epanaliptiko gemisma tou buffer 
		/*while ( file ) 
		{ 
			                                            
			file.read( buffer, sz );
			int i = file.gcount();
			write_to_socket( socket , buffer , i );


		}*/

		delete[] buffer;
		buffer = NULL;
		
		
	
			
			

			if(flag ==0)                                   //this means that the server has more files to sent
				pthread_mutex_unlock(mutex);
			else
			{ 
				size=-1;                                   //ending communication
				int tmp=htonl(size);
		
				write_to_socket( socket ,& tmp , sizeof(int) );
				delete mutex;
				mutex= NULL;
			    close(socket);
			}
			

	    
	
	
	return;
}

void * worker_threads  ( void * )
{
	
		while( 1 )
		{
			send_to_client();
			if(serv.get_flag() == 1 )                           //means that server received SIGINT , exiting from loop
			{	
				break;
			}
			pthread_cond_signal(&condition_read); 
		}
	
	
	pthread_exit( NULL ) ;
	

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Queue :: Queue()
{
    first=NULL;
    last=NULL;
}

Queue :: ~Queue()
{
    Node* t ;
		while(size > 0)
		{	
		    t=first;
			first=first->next;
			delete(t);
			t= NULL; 
			//pthread_mutex_lock(&size_lock);
			size--;
			//pthread_mutex_unlock(&size_lock);
		}
}

void Queue ::   push_Queue ( string& path_var, int socket_var ,pthread_mutex_t* mutex, int * ptr)
{
	
    if( size == 0)		//  pushing in an empty list ( first element)
    {	
        first = new Node;
		first->next=NULL;
		first->path=path_var;
		first->socket=socket_var;
		first->client_lock=mutex; 
		first->remaining_files = ptr;
		last=first;
		size++;	
	}
	else   
	{
		Node *t;
		t=first;      //mpenoun sto first
		first= new Node;    // ara |first|-> |next| -> ...... -> |last| ( to proto pou mpike )
		first->next=t;
		first->path=path_var;
		first->socket=socket_var;
		first->client_lock=mutex;
		first->remaining_files = ptr;
		size++;
	}
		return;
}

int Queue ::  delete_from_Queue ( int option )
{
	int flag1=0; 
                                            // auto
	Node* t = first ;
		if((size == 0 ) )
			return  flag1;			//if list empty or wrong pointer ,error
	    if((size > 1)  )	//if list has more than one nodes
		{	
			while(1)
			{
		        if(t->next==last)
			        break;
			    t=t->next;
			}
			if(option == 0 )
            {
            	 int x(*get_remaining());
           
          
          		  if(x == 1 )          //it means that server has no more files to sent
          	 	  {
          	 		pthread_mutex_destroy(last->client_lock);
          		  	//delete last->client_lock;
       			  //last->client_lock =NULL;
       		   	  	delete last->remaining_files;
        	    	last->remaining_files= NULL;
        	    	flag1 = 1;	
        		    }
        		else
        		
            	(*get_remaining())--;
           }
		    delete  last;
		    last = NULL;
			last=t;
			    
	    }
		else                                    // if deleting the only node
		{
			if(option == 0 )                               //carefull that queue may be empty but client may not have gotten a whole asnwer(  some paths will be written in queue later ) 
		    {
		   		int x(*get_remaining());
            	if(x == 1 )              
            	{
            	
		    	    pthread_mutex_destroy(last->client_lock);
					//delete last->client_lock;
		    	    delete last->remaining_files;
		    	   // last->client_lock =NULL;
            	  	last->remaining_files= NULL;
		    	    flag1=1;
           		 }
            	else
            
               	(*get_remaining())--;
          	}
            
			delete last;
			first=NULL;
			last=NULL;
			
	    }
	    size--;
	    return flag1;
	 
}    



int Queue ::  get_size() 
{

	return size;

}  




pthread_mutex_t * Queue ::   get_mutex()
{
	return last->client_lock;
}


int Queue :: get_sock()
{
	return last->socket;
}
    
std :: string& Queue ::  get_path()
{
	return last->path;
}

int *  Queue ::  get_remaining()
{
	return last->remaining_files;
}


Queue :: Node *   Queue ::   get_last() 
{

	return last;

}  

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Server :: Server()
{
    flag=0;
    clientlen= sizeof(client);
    serverptr =( struct sockaddr *) & server ;
    clientptr =( struct sockaddr *) & client ;

}


void Server :: set_values(int poolSize, int port_num , int queue_size_num)
{
	pool_size=poolSize;
    pool = new pthread_t[pool_size];
    port=port_num;
    queue_size=queue_size_num;
    for(int i=0 ; i < pool_size ; i++)
    {
    	pthread_create (&pool[i] , 0, worker_threads , NULL) ;

	}
	
}

Server :: ~Server()
{
	
	for(int i=0 ; i < pool_size ; i++)    //waiting for threads to terminate
		pthread_join(pool[i], NULL);
    delete[] pool;
	cout << endl<<"Exiting... " <<endl;
}


Queue  *   Server :: get_waiting()
{
	return &waiting;
}
    
Queue *    Server :: get_sending()
{
	return &sending;
}


int Server :: get_pool_size()
{
    return pool_size;
}


pthread_t*  Server :: get_pool_threads()
{
    return pool;
}


int Server :: get_sock()
{
    return sock;

}

int Server :: get_port()
{
    return port;

}

int Server :: max_size()
{
    return queue_size;

}

socklen_t Server :: get_clientlen()
{
    return clientlen;
}

struct sockaddr *  Server :: get_serverptr()
{
    return serverptr;
}


struct sockaddr *  Server :: get_clientptr()
{
    return clientptr;
}


int Server :: get_flag()
{
	return flag;
}
	
void Server :: set_flag(int fg)
{
	flag=fg;
	return;
}

int Server :: make_socket ()
{
    if (( sock = socket (PF_INET , SOCK_STREAM , 0 )) == -1 )
        perror( "Socket creation failed " );
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr=htonl( INADDR_ANY );
    server.sin_port = htons(port);
    if(bind(sock , serverptr , sizeof (server ) ) < 0 )
    {
      perror("error in  bind" );
      return 0;   
    }   
    if ( listen ( sock , 10) < 0 )
    {
        perror("error in listen" );
        return 0;
    }
     
    return 1;

}

int Server :: manage_connections()
{
    int newsock;
    while(1)
    {   

    	if (( newsock = accept (sock ,clientptr , &clientlen) ) < 0 ) 
        {
            break;
        }
        cout<< "Connection accepted ..." <<endl;
        
        pthread_t *first_line_thread = new pthread_t() ;
        
        pthread_create (first_line_thread , 0, first_line_threads , (void * ) & newsock) ;
        if(serv.get_flag() == 1 )                       //signal has been received
        	break;
        
    }


    return 0;

}
