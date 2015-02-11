#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>

void  read_directory(const char * , int , pthread_mutex_t*);
void * first_line_threads  ( void *  ) ;
void sent_to_client();
void * worker_threads  ( void * );
void write_to_socket( int  , void *  , int );
void read_from_socket( int  , void *  , int );
void signal_kill(int);


 
   class Queue 
    {
    struct Node
    {
	    std :: string path;
	    int socket ,*remaining_files ;
        struct Node * next;
        pthread_mutex_t *client_lock;//ena mutex ana client
        
    } ;

        Node *first,*last;
        int size;
       
   public:
    
    Queue () ;
    
    ~Queue();
 	
    

    void  push_Queue ( std:: string & , int ,pthread_mutex_t*,int * );

    int delete_from_Queue ( int= 0   ) ;//diagrafi mono ap to telos ( to proto pou mpike)
    
    
    int get_size();
    
    Node *  get_last();
    
    pthread_mutex_t* get_mutex(); //returns mutex variable of last
    
    
    int get_sock(); //returns socket of last
    
    std :: string& get_path(); //returns path of last
    
    int *get_remaining();
    

    };

class Server
{
    
  
    pthread_t *pool ;
    int pool_size;
    int sock , port ,queue_size;
    struct sockaddr_in server , client;
    socklen_t clientlen;
    struct sockaddr * serverptr ;
    struct sockaddr * clientptr;
    Queue sending , waiting;
    int flag;
   public:
    
    Server();
    ~Server();
    
    void set_values(int ,int,int);
    
    int get_pool_size();
   
    pthread_t* get_pool_threads();
    
    int  get_sock();

    int   get_port();
    
    Queue  * get_waiting();
    
    Queue * get_sending();
    
    int max_size();
    
    
    socklen_t   get_clientlen();
    
    struct sockaddr *    get_serverptr();

    struct sockaddr *    get_clientptr() ;
    
    int manage_connections() ;
    
    int  make_socket ();
	
	int get_flag();
	
	void set_flag(int);
};
