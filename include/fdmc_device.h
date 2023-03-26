#ifndef _FDMC_DEVICE_INCLUDE
#define _FDMC_DEVICE_INCLUDE

#include "fdmc_global.h"
#include "fdmc_process.h"
#ifndef HANDLER_SPECIFIC
#define HANDLER_SPECIFIC void *data
#endif
// al connection descriptor
typedef struct _FDMC_DEVICE
{
	char *name; // Name of device
	struct _FDMC_PROCESS *process;
	FDMC_THREAD *device_thread; // Thread for active devices
	FDMC_LOGSTREAM *device_stream;
	int (*event_handler)(struct _FDMC_DEVICE*, FDMC_DEV_EVENT); // Event handler for device
	int package_number;
	void *data; // User data
	char ip_addr[18]; // ip address of device
	int ip_port; // ip port of device
	char device_instit[FDMC_DEV_LENGTH+1];
	int sleeptime; // Timeout in seconds before next connection procedure
	int natt; // Number of connection attemts (zero if forever)
	int header_size; // Number of bytes in header
	int header_type; // Type of header
	FDMC_SOCK *link; // Data socket of device
	FDMC_SOCK *accepted; // For listeners when accept connection
	struct _FDMC_DEVICE *parent; // For accepted sockets - listener which accepted connection
	FDMC_LOGSTREAM *stream; // Output file of device
	FDMC_DEVROLE role; // Device role in list
	FDMC_DEVSTAT stat; // Status of device
	FDMC_DEV_EVENT dev_event; // Event code to process by devproc()
	int dev_wait; // Wait until handler ends event procedure
	char device_buf[DEFAULT_BUFSIZE]; // Buffer for storing exchange data
	//char *device_buf;
	int buflen; // Capacity of buffer
	int msglen; // Last received message length
	FDMC_SOCKPAIR sockpair;
	FDMC_THREAD_LOCK *lock;
} FDMC_DEVICE;

// Connection thread handler descriptor
typedef struct _FDMC_HANDLER
{
	FDMC_THREAD *handler_thread; // Descriptor of handler thread
	int (*event_handler)(struct _FDMC_HANDLER*, FDMC_DEV_EVENT); // Event handler for handler
	struct _FDMC_PROCESS *process; // Parent process descriptor
	struct _FDMC_DEVICE dev_copy; // Copy of current served device (if nessesary)
	struct _FDMC_DEVICE *dev_ref; // Pointer to current served device (mandatory)
	int permanent; // Do not terminate on timeout
	FDMC_HNDL_STAT hndl_stat; // Status - valid or deleted
	int number; // ID of handler (for debug information)
	FDMC_SOCKPAIR sockpair;
	FDMC_THREAD_LOCK *lock; // Handler mutex
	FDMC_SQL_SESSION *db_session;
	HANDLER_SPECIFIC; // Any additional data associated with handler
} FDMC_HANDLER;

// void fdmc_handler_assign(FDMC_DEVICE *pdev, FDMC_PROCESS *pproc, FDMC_EXCEPTION *err);
 int fdmc_device_select(FDMC_PROCESS *pptr);

 FDMC_PROCESS *fdmc_process_create(
	char *name,
	FDMC_THREAD_FUNC start_proc, // Thread function for process loop
	char *proc_file, // Output stream for process
	int file_mode, // Reopen mode for stream (1 - recreate output, 0 - reopen output)
	int proc_stack_size, // Stack size for process thread
	int sockport // io port for process - handler data exchange
	);

 int fdmc_process_stop(FDMC_PROCESS *p_proc);
 int fdmc_process_start(FDMC_PROCESS *p_proc);

// Common function for handler thread loop
 FDMC_THREAD_TYPE __thread_call fdmc_handler_thread(void *p);
// Another one, using process handler stack
 FDMC_THREAD_TYPE __thread_call fdmc_handler_thread_s(void *p_hndl);

 // Allocate memory for device and device fields
 FDMC_DEVICE* fdmc_device_alloc(FDMC_PROCESS* p_proc, FDMC_EXCEPTION *err);

 // Create device with specified parameters
 FDMC_DEVICE *fdmc_device_create(
									   char *name,
									   FDMC_PROCESS *pproc, // Process owner of device
									   int p_headsize, // Header size of messages
									   char *devfile, int devfilemode, // Device file (for log output)
									   char *ip_address, // ip address of device (NULL - listener created)
									   int ip_port, // ip port of device
									   int sleeptime, // timeout before next connection
									   int attempts, // number of attempts for connection
									   FDMC_EXCEPTION *err // error handler
	);

 int fdmc_device_delete(FDMC_DEVICE *pdev);
int fdmc_device_events(FDMC_DEVICE *p_dev, FDMC_DEV_EVENT p_ev);
 FDMC_HANDLER *fdmc_handler_create(
	FDMC_PROCESS *pproc, // Process context for handler
	FDMC_EXCEPTION *err // error handler
	);
 int fdmc_handler_delete(FDMC_HANDLER *p, FDMC_EXCEPTION *err);

 int fdmc_sockpair_create_t(FDMC_PROCESS *p_proc, FDMC_SOCKPAIR *p_pair, FDMC_EXCEPTION *err);
 int fdmc_sockpair_close(FDMC_SOCKPAIR *p_pair);

 int fdmc_device_link(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err);
 int fdmc_device_disconnect(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err);
 int fdmc_device_ready(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err);
 int fdmc_device_accept(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err);
 int fdmc_device_receive(FDMC_DEVICE *pdev, char *buffer, int maxbytes, FDMC_EXCEPTION *err);
 int fdmc_device_listen(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err);
 int fdmc_device_connect(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err);
 int fdmc_device_send(FDMC_DEVICE *pdev, char *buffer, int maxbytes, FDMC_EXCEPTION *err);
 int fdmc_device_continue(FDMC_HANDLER *p_hndl, FDMC_EXCEPTION *err);
 FDMC_HANDLER* fdmc_handler_get(FDMC_PROCESS *p, FDMC_EXCEPTION *err);
// Handler selection using fast stack
 FDMC_HANDLER *fdmc_handler_pop(FDMC_PROCESS *p_proc, FDMC_EXCEPTION *err);
 int fdmc_handler_push(FDMC_HANDLER *p_hndl, FDMC_EXCEPTION *err);
// Thread to create socket pairs for threads data exchange 
 FDMC_THREAD_TYPE __thread_call fdmc_sockio_thread(void *p);

// Creating devices when start process
 int fdmc_process_links_create(FDMC_PROCESS *p, FDMC_EXCEPTION *err); // Passive devices
 int fdmc_process_connects_create(FDMC_PROCESS *p, FDMC_EXCEPTION *err); // Active devices

 FDMC_THREAD_TYPE __thread_call fdmc_handler_lookup(void *p);
 int fdmc_process_sock_add(FDMC_PROCESS *p, FDMC_SOCK *s);

 FDMC_THREAD_TYPE __thread_call fdmc_device_thread(void *p_par);
// Loops for active devices
 int fdmc_device_parent(FDMC_DEVICE *p_dev, FDMC_EXCEPTION *err);
 int fdmc_device_child(FDMC_DEVICE *p_dev, FDMC_EXCEPTION *err);
 int fdmc_device_initiator(FDMC_DEVICE *p_dev, FDMC_EXCEPTION *err);

#define FDMC_HANDLE_DATA 5
#define FDMC_HANDLE_DELETE_OK 6
#define FDMC_HANDLE_DELETE_REQ 7

#endif
