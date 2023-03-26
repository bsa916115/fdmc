
#ifndef _FDMC_PROCESS_INCLUDE
#define _FDMC_PROCESS_INCLUDE

#include "fdmc_global.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"
#include "fdmc_stack.h"
#include "fdmc_thread.h"

#ifdef FDMC_USE_SQL
#include "fdmc_oci.h"
#endif

#define DEFAULT_BUFSIZE 1024*8
#define DEFAULT_NAMESIZE 32

typedef enum _FDMC_DEVROLE
{
	FDMC_DEV_INITIATOR,
	FDMC_DEV_PARENT,
	FDMC_DEV_CHILD
} FDMC_DEVROLE;

typedef enum _FDMC_DEVSTAT
{
	FDMC_DEV_IDLE = 0,
	FDMC_DEV_FREE,
	FDMC_DEV_BISY,
	FDMC_DEV_SERVED,
	FDMC_DEV_DELETED
} FDMC_DEVSTAT;

typedef enum _FDMC_DEV_EVENT
{
	// Device is in idle state
	FDMC_DEV_EVIDLE = 0, 
	// After Device was created, usually after this create link
	FDMC_DEV_EVCREATE = 1, 
	// After Device connection established,
	// for initiator after connect, for child after accept
	FDMC_DEV_EVCONNECTED = 2, 
	// Device has to accept connection, when device is ready
	FDMC_DEV_EVACCEPT = 3, 
	// Device has to listen port
	FDMC_DEV_EVLISTEN = 4,
	// Device is ready for exchange. Read data for connected device
	// or accept connection for listen device
	FDMC_DEV_EVREADY = 5, 
	// Device has to be deleted
	FDMC_DEV_EVDELETE = 6, 
	// After device connection lost
	FDMC_DEV_EVDISCONNECT = 7, 
	// Device has to receive data into buffer
	FDMC_DEV_EVRECEIVE = 8, 
	// Device has data in buffer
	FDMC_DEV_EVDATA = 9, 
	// Device data was sent to peer
	FDMC_DEV_EVSENT = 11,
	// Device received control message
	FDMC_DEV_EVCTRL = 10,
	// Device going to shutdown
	FDMC_DEV_EVSHUTDOWN = 12,
	// Device lost child connection
	FDMC_DEV_EVCHILDLOST = 14,
	// Error occurred while op processing
	FDMC_DEV_EVERROR = -1
} FDMC_DEV_EVENT;

struct _FDMC_PROCESS;
struct _FDMC_DEVICE;
struct _FDMC_HANDLER;

typedef enum _FDMC_HNDL_STAT
{
	FDMC_HNDL_ACTIVE,
	FDMC_HNDL_DELETE_REQ,
	FDMC_HNDL_DELETED,
	FDMC_HNDL_BISY
} FDMC_HNDL_STAT;

#define FDMC_HNDL_IDLE FDMC_HNDL_ACTIVE

typedef struct _FDMC_SOCKPAIR
{
	FDMC_SOCK *his_mail; // Other processes will send and receive data from this socket
	FDMC_SOCK *my_mail; // I will read and write data from this sockket
} FDMC_SOCKPAIR;

// External connection and programme handler manager
typedef struct _FDMC_PROCESS
{
	char *name; // Name of process in application
	FDMC_THREAD *pthr; // Thread of process
	FDMC_LIST *device_list; // List of actual devices for select loockup
	FDMC_LIST *handler_list;  // List of active communication handlers
	FDMC_LIST *sleeping_handlers; // List of sleeping handlers
	FDMC_HASH_TABLE *device_hash; // Hash table of devices for fast access
	FDMC_LIST *free_handlers; // Stack to accelerate handle selection
	FDMC_THREAD_FUNC start_proc; // Start procedure for process tread
	FDMC_THREAD_FUNC handler_proc; // Start procedure for communication handler threads
	FDMC_THREAD_FUNC device_proc; // Start procedure for devices
	int hndl_stack_size; // Stack size for handler thread
	int dev_stack_size; // Stack size for active devices
	int (*event_handler)(struct _FDMC_PROCESS*, FDMC_DEV_EVENT); // Event handler for process
	int (*handler_event)(struct _FDMC_HANDLER*, FDMC_DEV_EVENT); // Default event handler for handlers
	int (*device_event)(struct _FDMC_DEVICE*, FDMC_DEV_EVENT); // Default event handler for devices
	int wait_sec; // Time in seconds for waiting socket
	int sock_port; // Port for exchange between handlers and devices
	SOCKET sock_max; // Maximum number of device socket (use for select call)
	SOCKET sock_min; // Minimum number of device socket
	int sock_count; // Number of active sockets
	fd_set sock_set; // Device set for select call
	FDMC_SOCKPAIR sockpair; // Comunication pair
	FDMC_THREAD_LOCK *lock; //Process mutex
#ifdef FDMC_USE_SQL
	FDMC_SQL_SESSION *db_session;
#endif
	void *data; // Some additional data associated with process
} FDMC_PROCESS;


#endif

