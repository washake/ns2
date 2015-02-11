#ifndef _ftfp_nbtable_h_
#define _ftfp_nbtable_h_

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>


#define CURRENT_TIME	scheduler::instance().clock
#define INFF		0xff


class FTFP_Neighbor {
  friend class FTFP;
  friend class ftfp_rt_entry;
public: 
  FTFP_Neighbor(u_int32_t a) {nb_addr = a; }
  
protected:
    LIST_ENTRY(FTFP_Neighbor) nb_link;
    nsaddr_t	nb_addr;
    double 	nb_expire;
    //int 	nb_length;
};

LIST_HEAD(ftfp_ncache, FTFP_Neighbor);

/*
 * Here is Precursor list. For each valid route maintained by a node 
 * (containing a finite Hop
   Count metric) as a routing table entry, the node also maintains a
   list of precursors that may be forwarding packets on this route.
   These precursors will receive notifications from the node in the
   event of detection of the loss of the next hop link.  The list of
   precursors in a routing table entry contains those neighboring nodes
   to which a route reply was generated or forwarded.
   
   For now, maybe i will use this class to manage the routing table based on the 
   information of triangle table 
*/

class FTFP_Precursor {
  friend class FTFP;
  friend class ftfp_rt_entry;
public:
  FTFP_Precursor(u_int32_t a) {pc_addr = a;}
  
protected:
  LIST_ENTRY(FTFP_Precursor) pc_link;
  nsaddr_t 	pc_addr;
};

LIST_HEAD(ftfp_precursors, FTFP_Precursor);

/*
 * Routing Table Entry
*/

class ftfp_rt_entry {
  friend class ftfp_rtable;
  friend class FTFP;
  friend class FTFP_LocalRepairTimer;
  
  
public:
  ftfp_rt_entry();
  ~ftfp_rt_entry();
  
  void 			nb_insert(nsaddr_t id);
  FTFP_Neighbor* 	nb_lookup(nsaddr_t id);
  int 			nb_calculate_length(FTFP_Neighbor* aa) {
				  int length =0; 
				  //FTFP_Neighbor *nb = rt_nblist.lh_first; 
				  while(aa){length++;aa = aa->nb_link.le_next;} 
				  return length;}
  int 			pc_calculate_length(FTFP_Precursor* bb) {
				  int length =0; 
				  //FTFP_Precursor *pc = rt_pclist.lh_first; 
				  while(bb){length++;
				    bb = bb->pc_link.le_next;} return length;}
  
  void 			pc_insert(nsaddr_t id);
  FTFP_Precursor*	pc_lookup(nsaddr_t id);
  void 			pc_delete(nsaddr_t id);
  bool 			pc_empty(void);
  void 			pc_delete(void);
  
  double 		rt_req_timeout;	//when i can send another req 
  u_int8_t		rt_req_cnt; 	//number of route requests
  u_int8_t		getrt_flags_() {return rt_flags;}
protected:
  LIST_ENTRY(ftfp_rt_entry) rt_link;
  
  nsaddr_t		rt_dst;
  u_int32_t		rt_seqno;
  u_int16_t		rt_hops;
  int 			rt_last_hop_count;
  nsaddr_t		rt_nexthop;
  ftfp_precursors	rt_pclist;
  double 		rt_expire;
  u_int8_t		rt_flags;
  
#define RTF_DOWN 0
#define RTF_UP 1
#define RTF_IN_REPAIR 2
  
#define MAX_HISTORY 3
  double 		rt_disc_latency[MAX_HISTORY];
  char 			hist_indx;
  int 			rt_req_last_ttl;
  
  /*
   *  a list of neighbors that are using this route.
   */
  
  ftfp_ncache		rt_nblist;
  
};

class ftfp_rtable {
public:
  ftfp_rtable() {LIST_INIT(&rthead);}

  ftfp_rt_entry*	head() { return rthead.lh_first; }
  ftfp_rt_entry*	rt_add(nsaddr_t id);
  void 			rt_delete(nsaddr_t id);
  ftfp_rt_entry*	rt_lookup(nsaddr_t id);
  
private:
  LIST_HEAD(ftfp_rthead, ftfp_rt_entry) rthead;
};

#endif 