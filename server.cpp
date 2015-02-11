#include <iostream>
#include <cstdlib>
#include <cstring>
#include <signal.h>

#include "library.h"

using namespace std;


extern Server  serv;



int main( int argc , char ** argv)
{


	static struct sigaction act ;
    
    act.sa_handler=signal_kill;
    sigfillset(&(act.sa_mask));
    act.sa_flags=SA_RESTART;
    
    sigaction(SIGINT , &act , NULL);
    
    
    int port,thread_pool_size,queue_size;
    if(argc != 7 )                // check input arguments, if false return
    {
        cout<<"Wrong arguments ,try again ..."<<endl;
        return 1;
    }
    for(int i= 1 ; i < 6 ; i=i+2)
    {
        if(!strcmp(argv[i],"-p"))
        {
            port=atoi(argv[i+1]);
            cout<<"port number : " <<port <<endl;
        }
        else if(!strcmp(argv[i],"-s"))
        {
            thread_pool_size=atoi(argv[i+1]);
            cout<<"thread_pool_size : " <<thread_pool_size <<endl;
        }
        else if(!strcmp(argv[i],"-q"))
        {
            queue_size=atoi(argv[i+1]);
            cout<<"queue_size : " <<queue_size <<endl;
        }
        
    }
    
    serv.set_values(thread_pool_size,port ,queue_size);
    int socket_flg=serv.make_socket();
    if(!socket_flg)
        return 1;
    cout<< "Listening for connections ....." <<endl;
    serv.manage_connections();
    
    return 0;

}
