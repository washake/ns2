#include <ftfp/ftfp.h>
#include <ftfp/ftfp_packet.h>
#include <ftfp/ftfp_nbtable.h>
#include <random.h>
#include <cmu-trace.h>
#include <ip.h>
#include <node.h>

#define max(a,b)   		( (a) > (b) ? (a) : (b))
#define CURRENT_TIME_		Scheduler::instance().clock()


//new Packet Type
int hdr_ftfp::offset_;
static class FTFPHeaderClass : public PacketHeaderClass {
public:
  FTFPHeaderClass() : PacketHeaderClass("PacketHeader/FTFP",
				    sizeof(hdr_all_ftfp)) {
		bind_offset(&hdr_ftfp::offset_);		      
		}
}class_rtProtoFTFP_hdr;


//TCL Hooks
static class FTFPclass : public TclClass {
public:
  FTFPclass() : TclClass("Agent/FTFP") {}
  TclObject* create(int argc, const char*const* argv) {
    assert(argc == 5);
    return (new FTFP((nsaddr_t) Address::instance().str2addr(argv[4])));
  }
} class_rtProtoFTFP;




int FTFP::command(int argc, const char*const* argv)
{
  if(argc == 2) {
    Tcl& tcl = Tcl::instance();
    
    if(strncasecmp(argv[1], "id", 2) == 0) {
      tcl.resultf("%d",index);
      return TCL_OK;
    }
    
    if(strncasecmp(argv[1], "start", 2) == 0) {
#ifdef DEBUG
      fprintf(fp, "[$node start] in command function.\n");
#endif
      btimer.handle((Event*) 0);
      
//#ifdef FTFP_LINK_LAYER_DETECTION
      htimer.handle((Event*) 0);
      ntimer.handle((Event*) 0);
//#endif
      
      rtimer.handle((Event*) 0);
      return TCL_OK;
    }
  }
  
  else if(argc == 3) {
    if(strcmp(argv[1], "index") == 0) {
      index = atoi(argv[2]);
      return TCL_OK;
    }
    
    else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
      logtarget = (Trace*) TclObject::lookup(argv[2]);
      if(logtarget == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    
    else if(strcmp(argv[1], "if-queue") == 0) {
      ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
      
      if(ifqueue == 0)
	return TCL_ERROR;
      return TCL_OK;
    }
    
    else if(strcmp(argv[1], "port-dmux") == 0) {
      dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
      if(dmux_ == 0) {
	fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__,
	  argv[1], argv[2]
	);
	return TCL_ERROR;
      }
      
      return TCL_OK;
    }
  }
return Agent::command(argc, argv);
}


/*
 * Constructor
 */

FTFP::FTFP(nsaddr_t id): Agent(PT_FTFP),
			 btimer(this),
			 htimer(this),
			 ntimer(this),
			 lrtimer(this),
			 rtimer(this) 
{
  
  index = id;
  seqno = 2;
  bid = 1;
  hello_sequence = 0;
  
  LIST_INIT(&nbhead);
  LIST_INIT(&bihead);
  
  logtarget = 0;
  ifqueue = 0;
  
//#ifdef DEBUG
  fp = fopen("out.txt","w+");
  //nb_print();
  //fprintf(fp, "\n======================================================\n"); 
//#endif
}


/*
 * Timers
 */
void FTFP_BroadcastTimer::handle(Event*)
{
  agent->id_purge();
  Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

void FTFP_HelloTimer::handle(Event*)
{
  if(agent->get_hello_seqno() == 0) {
    agent->sendHello(); 
    agent->set_hello_seqno(agent->get_hello_seqno()+1);
  }
  else if (agent->get_hello_seqno() < 4) {
    agent->sendTRIHello();
    agent->set_hello_seqno(agent->get_hello_seqno() + 1);
  }
  else
    agent->set_hello_seqno(0);
  
  double interval = MinHelloInterval +
		  ((MaxHelloInterval - MinHelloInterval) * Random::uniform());
  assert(interval >= 0);
  Scheduler::instance().schedule(this, &intr, interval);
}


void FTFP_RouteCacheTimer::handle(Event*)
{
  agent->rt_purge();
#define FREQUENCY 0.5 // sec
  Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

void FTFP_NeighborTimer::handle(Event*)
{
    agent->nb_purge();
    Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

void FTFP_LocalRepairTimer::handle(Event* p)
{
 ftfp_rt_entry *rt;
 //ftfp_rt_entry *fa;
 struct hdr_ip *ih = HDR_IP( (Packet *)p);
 //fa = agent->rtable.head();
 //agent->rtable.rt_delete(ih->daddr());
 rt = agent->rtable.rt_lookup(ih->daddr());
 if (rt && rt->getrt_flags_() != RTF_UP) {
   agent->rt_down(rt);
 
#ifdef DEBUG
 fprintf(stderr, "Dst - %d, failed local repair\n", rt->rt_dst);
#endif
 }
 Packet::free((Packet *)p);
}


void FTFP::nb_insert(nsaddr_t id)
{
  FTFP_Neighbor *nb = new FTFP_Neighbor(id);
  //nb->nb_expire
  assert(nb);
  nb->nb_expire = CURRENT_TIME_ + (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);

  LIST_INSERT_HEAD(&nbhead, nb, nb_link);
  //nb_print();
  //seqno += 2;
  //assert((seqno%2) == 0);
}

FTFP_Neighbor* FTFP::nb_lookup(nsaddr_t id) 
{
  FTFP_Neighbor *nb = nbhead.lh_first;
  
  for(; nb; nb = nb->nb_link.le_next) {
    if(nb->nb_addr == id) break;
  }
  
  return nb;
}

void FTFP::nb_delete(nsaddr_t id)
{
  FTFP_Neighbor *nb = nbhead.lh_first;
  
  for(; nb; nb = nb->nb_link.le_next) {
    if(nb->nb_addr ==id) {
      LIST_REMOVE(nb, nb_link);
      delete nb;
      break;
    }
  }
  
  handle_link_failure(id);
}

void FTFP::nb_purge() {
FTFP_Neighbor *nb = nbhead.lh_first;
FTFP_Neighbor *nbn;
double now = CURRENT_TIME_;

 for(; nb; nb = nbn) {
   nbn = nb->nb_link.le_next;
   if(nb->nb_expire <= now) {
     nb_delete(nb->nb_addr);
   }
 }

}


void FTFP::nb_print(void)
{
#ifdef DEBUG
  FTFP_Neighbor *nb = nbhead.lh_first;
  fprintf(stderr, "\n======================================================\n");
  fprintf(stderr, "------------ this node is %d---------------\n", index);
  for(;nb;nb=nb->nb_link.le_next) {
    fprintf(stderr, "Neighbor_Adress: %d\n", nb->nb_addr);
  }
  fprintf(stderr, "\n======================================================\n");
#endif
  //neighbor_list_node *node;
  //fprintf(fp,"\n================= We want to show all the node and their Neighbor_Adress========================\n");
  //for (;node;node = node->next) {
  //  fprintf(fp,"node : %d", node->nodeid);
  //}
  FTFP_Neighbor *nb = nbhead.lh_first;
  fprintf(fp, "\n======================================================\n");
  fprintf(fp, "------------ this node is %d---------------\n", index);
  for(;nb;nb=nb->nb_link.le_next) {
    fprintf(fp, "Neighbor_Adress: %d\n", nb->nb_addr);
    //fprintf(fp,"test what is it: %d\n", ftfp_ncache.)
  }
  fprintf(fp, "\n======================================================\n");
  
  
//#endif
}


void FTFP::id_purge(void)
{
  FTFP_BroadcastID *b = bihead.lh_first;
  FTFP_BroadcastID *bn;
  double now = CURRENT_TIME_;
  
  for(; b; b = bn) {
    bn = b->link.le_next;
    if(b->expire <= now) {
      LIST_REMOVE(b,link);
      delete b;
    }
  }
}

void FTFP::rt_purge(void)
{
  
}

void FTFP::handle_link_failure(nsaddr_t id)
{

}


void FTFP::sendHello(void)
{
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_ftfp_hello *rh = HDR_FTFP_HELLO(p);
  
#ifdef DEBUG
  fprintf(stderr, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
#endif
  fprintf(fp, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
  
  rh->fp_type = FTFPTYPE_HELLO;
  rh->fp_hello_sequence = get_hello_seqno();
  rh->fp_dst = index;
  rh->fp_dst_seqno = seqno;
  rh->fp_lifetime = (1+ALLOWED_HELLO_LOSS)*HELLO_INTERVAL;
  
  ch->ptype() = PT_FTFP;
  ch->size() = IP_HDR_LEN + rh->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index;
  
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;
  
  Scheduler::instance().schedule(target_, p, 0.0);
  nb_print();
}

void FTFP::sendTRIHello(void)
{
    Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_ftfp_hello *rh = HDR_FTFP_HELLO(p);
  
#ifdef DEBUG
  fprintf(stderr, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
#endif
  fprintf(fp, "sending TRIHello from %d at %.2f\n", index, Scheduler::instance().clock());
  
  rh->fp_type = FTFPTYPE_HELLO;
  rh->fp_hello_sequence = get_hello_seqno();
  rh->fp_dst = TRI_random();
  rh->fp_dst_seqno = seqno;
  rh->fp_lifetime = (1+ALLOWED_HELLO_LOSS)*HELLO_INTERVAL;
  
  ch->ptype() = PT_FTFP;
  ch->size() = IP_HDR_LEN + rh->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index;
  
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;
  
  Scheduler::instance().schedule(target_, p, 0.0);
  nb_print();
  //set_hello_seqno(rh->fp_hello_sequence);
}

void FTFP::send_Hello_ACK(u_int8_t seq, nsaddr_t dstad, u_int32_t dstad_sq, nsaddr_t srcad, u_int32_t srcad_sq)
{
    Packet *p = Packet::alloc();
    struct hdr_cmn *ch = HDR_CMN(p);
    struct hdr_ip *ih = HDR_IP(p);
    struct hdr_ftfp_hello_ACK *rh = HDR_FTFP_HELLO_ACK(p);
    
    fprintf(fp, "sending Hello_ACK from %d ---> %d at %.2F\n", srcad,dstad,Scheduler::instance().clock() );
    
    rh->fack_type = FTFPTYPE_HELLO_ACK;
    rh->fack_hello_sequence = seq;
    rh->fack_src = srcad;
    rh->fack_src_seqno = srcad_sq;
    rh->fack_dst = dstad;
    rh->fack_dst_seqno = dstad_sq;
    rh->fack_lifetime = MY_ROUTE_TIMEOUT;
    
    ch->ptype() = PT_FTFP;
    ch->size() = IP_HDR_LEN + rh->size();
    ch->iface() = -2;
    ch->error() = 0;
    ch->addr_type() = NS_AF_INET;
    ch->next_hop_ = 0;
    ch->prev_hop_ = index;
    ch->direction() = hdr_cmn::DOWN;
    
    ih->saddr() = index;
    ih->daddr() = dstad;
    ih->sport() = RT_PORT;
    ih->dport() = RT_PORT;
    ih->ttl_ = NETWORK_DIAMETER;
    
    Scheduler::instance().schedule(target_, p, 0.);
}



/*
 *  The reason we dont wait for the neighbor list is established integrately, is that, 
 *  we assume the topoly of the whole map is moving, when we receive the others' hello message, the 'neigbor' may be move away next time.
 *  So we just random send the TRI_HELLO randomly to the current nodes in the neighbor list.
 */



nsaddr_t FTFP::TRI_random()
{
  FTFP_Neighbor *nb = nbhead.lh_first;
  FTFP_Precursor *pb = pentry.lh_first;
  //ftfp_rt_entry *rt;
  int i = 1;
  int loop = 0;
  int rd = (int)(Random::uniform(rentry.nb_calculate_length(nb)) + 0.5);
  if (rd == 0) {
    return nb->nb_addr;
  }
  else {
  while(i<rd) {
    nb = nb->nb_link.le_next;
    i++;
  }
  if (nb == 0) {
    
    nb = nb->nb_link.le_next;
    //return nb->nb_addr;
  }
  if (rentry.pc_calculate_length(pb))
    rentry.pc_insert(nb->nb_addr);
  else {
    while(!(rentry.pc_lookup(nb->nb_addr)) && loop<30) {
    i = 1;
    rd = (int)(Random::uniform(rentry.nb_calculate_length(nb)) + 0.5);
    while(i<rd) {
      nb = nb->nb_link.le_next;
      i++;
    }
    if (nb == 0) {
    
      nb = *(nb->nb_link.le_prev);
    //return nb->nb_addr;
    }
    loop++;
    }
  }
    return nb->nb_addr;
  }
    //return nb->nb_addr;
}




void FTFP::recvHello(Packet* p)
{
  struct hdr_ftfp_hello *rh = HDR_FTFP_HELLO(p);
  FTFP_Neighbor *nb;
  
  if (rh->fp_src == index) {
    fprintf(stderr, "Got my own Hello Message\n");
    Packet::free(p);
    return;
  }
  else if (rh->fp_dst == index) {
    fprintf(stderr, "Got TRIHello who dont want to see me ;_;\n");
    Packet::free(p);
    return;
  }
  else {
  
  if(rh->fp_hello_sequence == 0) {
    
    nb = nb_lookup(rh->fp_dst);
  //nb_print();
    if(nb == 0) {
      nb_insert(rh->fp_dst);
    }
    else {
      nb->nb_expire = CURRENT_TIME_ + (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
    }
    
    Packet::free(p);
    return;
  }
  
  else {
    
    nb = nb_lookup(rh->fp_dst);
    if(nb == 0) {
      Packet::free(p);
      return;
    }
    else {
      send_Hello_ACK(rh->fp_hello_sequence,
		     rh->fp_dst,
		     rh->fp_dst_seqno,
		     index,
		     seqno
		    );
      Packet::free(p);
      return;
    }
  }
  
  }
  
}


void FTFP::recvHello_ACK(Packet* p)
{
  //struct hdr_ip *ih = HDR_IP(p);
  struct hdr_ftfp_hello_ACK *rh = HDR_FTFP_HELLO_ACK(p);
  //FTFP_Neighbor *nb = nbhead.lh_first;
  FTFP_Precursor *pb = pentry.lh_first;
  if(rh->fack_src == index) {
    fprintf(fp, "Got my own HELLO_ACK");
    Packet::free(p);
    return;
  }
  // if the address in the precursor list is equal to the hello sequence number
  // we have receive one hello_ack and set the address from that ACK into the precursor list
  // if the address in the precursor list is small to the hello sequence number,
  // we receive the ACK and insert the address directly to the precursor list
  // if the address in the precusor list is larger that the hello sequence number
  // we wait because the precursor list will be refresh in a minutes
  
  if(rentry.pc_calculate_length(pb) == get_hello_seqno()) {
    fprintf(fp," \nThe node %d receive the hello_ACK from %d, the hello_sequence_number in the \
    HELLO_ACK is %d, the hello_sequence_number in the node is %d\n", 
    index, 
    rh->fack_src,
    rh->fack_hello_sequence,
    get_hello_seqno()
    );
    Packet::free(p);
    return;
  }
  else if(rentry.pc_calculate_length(pb) < get_hello_seqno()) {
    
    rentry.pc_insert(rh->fack_src);
    fprintf(fp,"\nLet's see the precursor list\n");
    FTFP_Precursor *pn = pentry.lh_first;
    for (;pn;pn = pn->pc_link.le_next)
      fprintf(fp," Precursor address in %d Address %d", index, pn->pc_addr);
    Packet::free(p);
  }
  else {
    Packet::free(p);
    return;
  }
  
}





void
FTFP::rt_down(ftfp_rt_entry *rt) {
  /*
   *  Make sure that you don't "down" a route more than once.
   */

  if(rt->rt_flags == RTF_DOWN) {
    return;
  }

  // assert (rt->rt_seqno%2); // is the seqno odd?
  rt->rt_last_hop_count = rt->rt_hops;
  rt->rt_hops = INFF;
  rt->rt_flags = RTF_DOWN;
  rt->rt_nexthop = 0;
  rt->rt_expire = 0;

}


void FTFP::recv(Packet* p, Handler*)
{
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip  *ih = HDR_IP(p);
  
  assert(initialized());
  
  if(ch->ptype() == PT_FTFP) {
    ih->ttl_ -= 1;
    recvFTFP(p);
    return;
  }
  
}  
  
void FTFP::recvFTFP(Packet* p)
{
   struct hdr_ftfp *fh = HDR_FTFP(p);
   
   assert(HDR_IP(p)->sport() == RT_PORT);
   assert(HDR_IP(p)->dport() == RT_PORT);
   
  
#ifdef DEBUG
  fprintf(fp, "Invalid FTFP type (%x)\n", dh->ff_type);
#endif
  switch(fh->ff_type) {
    case FTFPTYPE_HELLO:
      recvHello(p);
      break;
    case FTFPTYPE_HELLO_ACK:
      recvHello_ACK(p);
      break;
    default:
      fprintf(stderr, "Invalid FTFP type (%x)\n", fh->ff_type);
      exit(1);
  }
}
  


