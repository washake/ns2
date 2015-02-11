#ifndef _ftfp_h_
#define _ftfp_h_

#include <sys/types.h>
#include <trace/cmu-trace.h>
#include <priqueue.h>
#include <ftfp/ftfp_packet.h>
#include <ftfp/ftfp_nbtable.h>
#include <classifier/classifier-port.h>

#ifdef DEBUG
  FILE *fp = fopen("out.txt","w+");
#endif

#define FTFP_LOCAL_REPAR
#define FTFP_LINK_LAYER_DETECTION

#define FTFP_USE_LL_METRIC

class FTFP;

/*
 * Below we define some stable parameter which is set directly under AODV
 * the reason why we use this kind of parameter is that we just set it 
 * in the current test environment
 */

/*=========================================================
 * 
 * 
 * ========================================================
 */

#define MY_ROUTE_TIMEOUT        10                      	// 100 seconds
#define ACTIVE_ROUTE_TIMEOUT    10				// 50 seconds
#define REV_ROUTE_LIFE          6				// 5  seconds
#define BCAST_ID_SAVE           6				// 3 seconds
//#define 

// No. of times to do network-wide search before timing out for 
// MAX_RREQ_TIMEOUT sec. 
#define RREQ_RETRIES            3  
// timeout after doing network-wide search RREQ_RETRIES times
#define MAX_RREQ_TIMEOUT	10.0 //sec

/* Various constants used for the expanding ring search */
#define TTL_START     5
#define TTL_THRESHOLD 7
#define TTL_INCREMENT 2 

// This should be somewhat related to arp timeout
#define NODE_TRAVERSAL_TIME     0.03             // 30 ms
#define LOCAL_REPAIR_WAIT_TIME  0.15 //sec

// Should be set by the user using best guess (conservative) 
#define NETWORK_DIAMETER        30             // 30 hops

// Must be larger than the time difference between a node propagates a route 
// request and gets the route reply back.

//#define RREP_WAIT_TIME     (3 * NODE_TRAVERSAL_TIME * NETWORK_DIAMETER) // ms
//#define RREP_WAIT_TIME     (2 * REV_ROUTE_LIFE)  // seconds
#define RREP_WAIT_TIME         1.0  // sec

#define ID_NOT_FOUND    0x00
#define ID_FOUND        0x01
//#define INFINITY        0xff

// The followings are used for the forward() function. Controls pacing.
#define DELAY 1.0           // random delay
#define NO_DELAY -1.0       // no delay 

// think it should be 30 ms
#define ARP_DELAY 0.01      // fixed delay to keep arp happy


#define HELLO_INTERVAL          1               // 1000 ms
#define ALLOWED_HELLO_LOSS      3               // packets
#define BAD_LINK_LIFETIME       3               // 3000 ms
#define MaxHelloInterval        (1.25 * HELLO_INTERVAL)
#define MinHelloInterval        (0.75 * HELLO_INTERVAL)


/*
 * ========================================================
 * 
 * ========================================================
 */

class FTFP_BroadcastTimer : public Handler {
public:
	FTFP_BroadcastTimer(FTFP* a) : agent(a) {}
	void handle(Event*);
	
private:
	FTFP  *agent;
	Event intr;
};


class FTFP_HelloTimer : public Handler {
public:
	FTFP_HelloTimer(FTFP* a) : agent(a){}
	void handle(Event*);
  
private:
	//Packet *sq_number;
	FTFP *agent;
	Event intr;
};

class FTFP_NeighborTimer : public Handler {
public:
	FTFP_NeighborTimer(FTFP* a) : agent(a) {}
	void handle(Event*);
  
private:
	FTFP *agent;
	Event intr;
};

class FTFP_RouteCacheTimer : public Handler {
public:
  FTFP_RouteCacheTimer(FTFP* a) : agent(a) {}
  void handle(Event*);
  
private:
  FTFP *agent;
  Event intr;
};

class FTFP_LocalRepairTimer : public Handler {
  //friend class ftfp_rtable;
public: 
  FTFP_LocalRepairTimer(FTFP* a) : agent(a) {}
  void handle(Event*);
  
private:
  FTFP *agent;
  Event intr;
};

class FTFP_BroadcastID {
  friend class FTFP;
public:
  FTFP_BroadcastID(nsaddr_t i, u_int32_t b) { src = i; id = b; }
protected:
  LIST_ENTRY(FTFP_BroadcastID) link;
  nsaddr_t 	src;
  u_int32_t 	id;
  double 	expire;
};

LIST_HEAD(ftfp_bcache, FTFP_BroadcastID);


class FTFP : public Agent {
  
  friend class ftfp_rt_entry;
  friend class ftfp_rtable;
  friend class FTFP_BroadcastTimer;
  friend class FTFP_HelloTimer;
  friend class FTFP_NeighborTimer;
  friend class FTFP_RouteCacheTimer;
  friend class FTFP_LocalRepairTimer;
  
public:
  FTFP(nsaddr_t id);
  void 		recv(Packet *p, Handler *);
  u_int8_t	get_hello_seqno() {return hello_sequence;};
  void 		set_hello_seqno(u_int8_t a) {hello_sequence = a;}
protected:
  int 	command(int, const char*const*);
  int 	initialized() {return 1 && target_;}
  
  /*
   * Route Table Mangement
   */
  
  void 		rt_resolve(Packet *p);
  void 		rt_update(ftfp_rt_entry *rt, u_int32_t seqnum,
			  u_int16_t metric, nsaddr_t nexthop,
			  double expire_time
 			);
  
  void 		rt_down(ftfp_rt_entry *rt);
  //void 		local_rt_repair(ftfp_rt_entry *rt, Packet *p);
  
public:
  //void 		rt_ll_failed(Packet *p);
  void 		handle_link_failure(nsaddr_t id);
protected:
  void 		rt_purge(void);
  
  //void 		enque(ftfp_rt_entry *rt, Packet *p);
  //Packet*	deque(ftfp_rt_entry *rt);
  
  /*
   * Neighbor Mangement
   */
  
  void 		nb_insert(nsaddr_t id);
  FTFP_Neighbor* 	nb_lookup(nsaddr_t id);
  void 		nb_delete(nsaddr_t id);
  void 		nb_purge(void);
  void 		nb_print(void);
  
  /*
   * Brodcast ID Mangement
   */
  
  //void 		id_insert(nsaddr_t id, u_int32_t bid);
  //bool 		id_lookup(nsaddr_t id, u_int32_t bid);
  void 		id_purge(void);
  
  /*
   * Packet TX Routines
   */
  
  //void forward(ftfp_rt_entry *rt, Packet *p, double delay);
  void sendHello(void);
  
  //used for Triangle Precursor list Establishment
  void sendTRIHello(void);
  void send_Hello_ACK(u_int8_t seq, nsaddr_t dstad, u_int32_t dstad_sq,nsaddr_t srcad, u_int32_t srcad_sq);
  //void sendRequest(nsaddr_t dst);
  
  /*void sendReply(nsaddr_t ipdst, u_int32_t hop_count,
		 nsaddr_t rpdst, u_int32_t rpseq,
		 u_int32_t lifetime, double timestamp
		  );
		  */
  //void sendError(Packet *p, bool jitter = true);
  
  /*
   * Packet RX Routies
   */
  void recvFTFP(Packet *p);
  void recvHello(Packet *p);
  void recvTRIHello(Packet *p);
  void recvHello_ACK(Packet *p);
  
  //void recvRequest(Packet *p);
  //void recvReply(Packet *p);
  //void recvError(Packet *p);
  
  /*
   * History Mangement
   */
  
  double 	PerHopTime(ftfp_rt_entry *rt);
  nsaddr_t	TRI_random();
  //bool 		Check_in_Precursor(nsaddr_t id);
  nsaddr_t 	index;
  u_int32_t 	seqno;
  int 		bid;
  u_int8_t	hello_sequence;
  
  /*
   *  Routing Table 
   *  Neighbor Cache 
   *  Broudcast ID ?? (i dont konw what is this)
   */
  
  
  ftfp_rtable	rthead;
  ftfp_ncache	nbhead;
  ftfp_bcache 	bihead;
  
  /*
   * Timers
   * 
   * The difference between broadcast and hello is 
   * Hello can be broadcast or muticast or even send directly to 
   * the particular node, but broadcast message can be heard by everyone 
   * which is in the transmission range
   * 
   * we can broadcast a hello, a request or something else
   * 
   *
   */
  FTFP_BroadcastTimer 		btimer;
  FTFP_HelloTimer 		htimer;
  FTFP_NeighborTimer 		ntimer;
  FTFP_LocalRepairTimer		lrtimer;
  FTFP_RouteCacheTimer 		rtimer;

  
  
  /*
   * Routing Table
   */
  ftfp_rtable		rtable;
  
  //
  ftfp_rt_entry		rentry;
  ftfp_precursors	pentry;
  
  //A mechanism for logging the contents of the routing 
  Trace			*logtarget;
  NsObject		*uptarget_;
  NsObject		*port_dmux_;
  
  PriQueue		*ifqueue;
  
  
  //void 		log_link_del(nsaddr_t dst);
  //void 		log_link_broke(Packet *p);
  //void 		log_link_kept(nsaddr_t dst);
  
  PortClassifier	*dmux_;
  
//#ifdef DEBUG
  FILE *fp;
//#endif
  
};

#endif