#ifndef _FTFP_PACKET_H_
#define _FTFP_PACKET_H_

#include <config.h>
#define FTFP_MAX_ERRORS 100

/*=================================================================
  Packet Format
  =================================================================*/
#define FTFPTYPE_HELLO         0x01
#define FTFPTYPE_HELLO_ACK     0x02
#define FTFPTYPE_RREQ          0x03
#define FTFPTYPE_RREP	       0x04
#define FTFPTYPE_RERR	       0x08
#define FTFPTYPE_RREP_ACK      0x10


/*
 *  FTFP Routing Protocol Header Macro
 */
#define HDR_FTFP(p)	((struct hdr_ftfp*)hdr_ftfp::access(p))
#define HDR_FTFP_HELLO(p)	((struct hdr_ftfp_hello*)hdr_ftfp::access(p))
#define HDR_FTFP_REQUEST(p)	((struct hdr_ftfp_request*)hdr_ftfp::access(p))
#define HDR_FTFP_REPLY(p)	((struct hdr_ftfp_reply*)hdr_ftfp::access(p))
#define HDR_FTFP_ERROR(p)	((struct hdr_ftfp_error*)hdr_ftfp::access(p))
#define HDR_FTFP_RREP_ACK(p)	((struct hdr_ftfp_rrep_ack*)hdr_ftfp::access(p))
#define HDR_FTFP_HELLO_ACK(p)	((struct hdr_ftfp_hello_ACK*)hdr_ftfp::access(p))
/*
 * General FTFP Header - shared by all formats
 */
struct hdr_ftfp{
  u_int8_t    ff_type;
  static int offset_;
  inline static int& offset() {return offset_;}
  inline static hdr_ftfp* access(const Packet* p) {
      return (hdr_ftfp*) p->access(offset_);
  }
};

struct hdr_ftfp_hello {
        u_int8_t        fp_type;        // Packet Type
        u_int8_t      	reserved[2];
        u_int8_t        fp_hello_sequence;           // Hello Sequence Count
        nsaddr_t        fp_dst;                 // Destination IP Address
        u_int32_t       fp_dst_seqno;           // Destination Sequence Number
        nsaddr_t        fp_src;                 // Source IP Address
        nsaddr_t 	fq_forward_node;
        double	        fp_lifetime;            // Lifetime

        double          fp_timestamp;           // when corresponding REQ sent;
						// used to compute route discovery latency
						
  inline int size() { 
  int sz = 0;
  
  sz = sizeof(u_int32_t)		// rp_type
	     + 3*sizeof(u_int8_t) 	// rp_flags + reserved
	     //+ 3*sizeof(u_int8_t)		
	     + 2*sizeof(double)		
	     + 3*sizeof(nsaddr_t);		
	     //+ sizeof(u_int32_t)	
	     //+ sizeof(nsaddr_t)		
	     //+ sizeof(u_int32_t);	
  
  	//sz = 6*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
  }
};

struct hdr_ftfp_hello_ACK {
  
  u_int8_t 	fack_type;
  u_int8_t	reserved[2];
  u_int8_t 	fack_hello_sequence;
  nsaddr_t	fack_src;
  u_int32_t	fack_src_seqno;
  nsaddr_t	fack_dst;
  u_int32_t	fack_dst_seqno;
  double 	fack_lifetime;
  
  inline int size() {
    int sz = 0;
    
    sz = 3*sizeof(u_int8_t)
	 + 2*sizeof(ns_addr_t)
	 + 2*sizeof(u_int32_t)
	 +sizeof(double);
    assert(sz);
    return sz;
  }
};

struct hdr_ftfp_request {
  u_int8_t 	fq_type;
  u_int8_t      reserved[2];
  
  u_int8_t 	fq_hop_count;		// hop count 
  u_int32_t     fq_bcast_id; 		//broudcast ID
  
  nsaddr_t      fq_dst;
  u_int32_t	fq_dst_seqno;
  nsaddr_t 	fq_src;
  u_int32_t	fq_src_seqno;
  nsaddr_t 	fq_forward_node;
  u_int32_t	fq_forward_seqno;
  double 	fq_timestamp;
  
  inline int size() {
    int sz = 0;
    
    sz = sizeof(u_int8_t)
	+ 2*sizeof(u_int8_t)
	//+ sizeof(u_int8_t)
	+ sizeof(double)
	+ 4*sizeof(u_int32_t)
	+ 3*sizeof(nsaddr_t);

    assert(sz >= 0);
    return sz;
  }
};

struct hdr_ftfp_reply {
        u_int8_t        fp_type;        // Packet Type
        u_int8_t      	reserved[2];
        u_int8_t        fp_hop_count;           // Hop Count
        nsaddr_t        fp_dst;                 // Destination IP Address
        u_int32_t       fp_dst_seqno;           // Destination Sequence Number
        nsaddr_t        fp_src;                 // Source IP Address
        nsaddr_t 	fq_forward_node;
        double	        fp_lifetime;            // Lifetime

        double          fp_timestamp;           // when corresponding REQ sent;
						// used to compute route discovery latency
						
  inline int size() { 
  int sz = 0;
  
  sz = sizeof(u_int32_t)		// rp_type
	     + 3*sizeof(u_int8_t) 	// rp_flags + reserved
	     //+ 3*sizeof(u_int8_t)		
	     + 2*sizeof(double)		
	     + 3*sizeof(nsaddr_t);		
	     //+ sizeof(u_int32_t)	
	     //+ sizeof(nsaddr_t)		
	     //+ sizeof(u_int32_t);	
  
  	//sz = 6*sizeof(u_int32_t);
  	assert (sz >= 0);
	return sz;
  }

};

struct hdr_ftfp_error {
        u_int8_t        re_type;                // Type
        u_int8_t        reserved[2];            // Reserved
        u_int8_t        DestCount;                 // DestCount
        // List of Unreachable destination IP addresses and sequence numbers
        nsaddr_t        unreachable_dst[FTFP_MAX_ERRORS];   
        u_int32_t       unreachable_dst_seqno[FTFP_MAX_ERRORS];   

  inline int size() { 
  int sz = 0;
  /*
  	sz = sizeof(u_int8_t)		// type
	     + 2*sizeof(u_int8_t) 	// reserved
	     + sizeof(u_int8_t)		// length
	     + length*sizeof(nsaddr_t); // unreachable destinations
  */
  	sz = (DestCount*2 + 1)*sizeof(u_int32_t);
	assert(sz);
        return sz;
  }

};

struct hdr_ftfp_rrep_ack {
	u_int8_t	rpack_type;
	u_int8_t	reserved;
};

union hdr_all_ftfp {
  hdr_ftfp          	fh;
  hdr_ftfp_hello	fhel;
  hdr_ftfp_request  	freq;
  hdr_ftfp_reply    	frep;
  hdr_ftfp_error    	ferr;
  hdr_ftfp_rrep_ack 	frep_ack;
  hdr_ftfp_hello_ACK  	fhack;
};

#endif // FTFP_PACKET_H_